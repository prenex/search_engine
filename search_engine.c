/*
 * search_engine.c version 0.2
 *
 * Simple single-header and single-c search engine library
 * which lets you use prefix/infix/suffix (partial) search
 * on keys for which you provide values.
 *
 * This uses my specific prefix-tree-like algorithms.
 *
 * Licence currently is modified CC3 so you must name me
 * Richard Thier and my company MagosIT Kft not in the
 * source code, but at the usage scenario at some way.
 * Easiest to fulfill what I want is a link to my company
 * and my blog where I write details about this algo.
 *
 * Likely there will be also a blog post about the way
 * this technically works on ballmerpeak devblog ;-)
 * 
 * Header tried to be kept as small as possible to keep
 * compilation times fast, but all you need to do is to
 * add a single c file. This should work with C++ too!
 *
 * Richard Thier
 * Owner, CEO - MagosIT Kft (Hungary)
 */

#include "search_engine.h"
#include <stdint.h>

/* ************************ */
/* BEGIN: stretchy_buffer.h */

/* from: https://github.com/nothings/stb/blob/master/stretchy_buffer.h */
/* stretchy_buffer.h - v1.04 - public domain - nothings.org/stb */
/* a vector<>-like dynamic array for C */
/* */
/* version history: */
/*      1.04 -  fix warning */
/*      1.03 -  compile as C++ maybe */
/*      1.02 -  tweaks to syntax for no good reason */
/*      1.01 -  added a "common uses" documentation section */
/*      1.0  -  fixed bug in the version I posted prematurely */
/*      0.9  -  rewrite to try to avoid strict-aliasing optimization */
/*              issues, but won't compile as C++ */
/* */
/* Will probably not work correctly with strict-aliasing optimizations. */

#ifndef STB_STRETCHY_BUFFER_H_INCLUDED
#define STB_STRETCHY_BUFFER_H_INCLUDED

#ifndef NO_STRETCHY_BUFFER_SHORT_NAMES
#define sb_free   stb_sb_free
#define sb_push   stb_sb_push
#define sb_count  stb_sb_count
#define sb_add    stb_sb_add
#define sb_last   stb_sb_last
#endif

#define stb_sb_free(a)         ((a) ? free(stb__sbraw(a)),0 : 0)
#define stb_sb_push(a,v)       (stb__sbmaybegrow(a,1), (a)[stb__sbn(a)++] = (v))
#define stb_sb_count(a)        ((a) ? stb__sbn(a) : 0)
#define stb_sb_add(a,n)        (stb__sbmaybegrow(a,n), stb__sbn(a)+=(n), &(a)[stb__sbn(a)-(n)])
#define stb_sb_last(a)         ((a)[stb__sbn(a)-1])

#define stb__sbraw(a) ((int *) (void *) (a) - 2)
#define stb__sbm(a)   stb__sbraw(a)[0]
#define stb__sbn(a)   stb__sbraw(a)[1]

#define stb__sbneedgrow(a,n)  ((a)==0 || stb__sbn(a)+(n) >= stb__sbm(a))
#define stb__sbmaybegrow(a,n) (stb__sbneedgrow(a,(n)) ? stb__sbgrow(a,n) : 0)
#define stb__sbgrow(a,n)      (*((void **)&(a)) = stb__sbgrowf((a), (n), sizeof(*(a))))

#include <stdlib.h>

static void * stb__sbgrowf(void *arr, int increment, int itemsize) {
   int dbl_cur = arr ? 2*stb__sbm(arr) : 0;
   int min_needed = stb_sb_count(arr) + increment;
   int m = dbl_cur > min_needed ? dbl_cur : min_needed;
   int *p = (int *) realloc(arr ? stb__sbraw(arr) : 0, itemsize * m + sizeof(int)*2);
   if (p) {
      if (!arr)
         p[1] = 0;
      p[0] = m;
      return p+2;
   } else {
      #ifdef STRETCHY_BUFFER_OUT_OF_MEMORY
      STRETCHY_BUFFER_OUT_OF_MEMORY ;
      #endif
      return (void *) (2*sizeof(int)); /* try to force a NULL ptr ex. later */
   }
}
#endif /* STB_STRETCHY_BUFFER_H_INCLUDED */

/* END: stretchy_buffer.h */
/* ********************** */

/* **** */
/* DATA */
/* **** */

/**
 * Stores a character, related meta-data and trie parent-sibling indices used
 * in the prefix-tree data structure.
 *
 * This is 32 bits with the following bit fields.
 *
 * kkkkkkkk ssmmmmmm cccccccc nnnnnnnn
 *
 * - k is the bits of the encoded character.
 * - s are sign bits of 'c' and 'n' (defines if we do sub or add)
 * - m is [1..63] "meta" index for finding the linked values. 0 == no word end!
 * - c is UNSIGNED relative offset of (first) child. When zero no child.
 * - n is UNSIGNED relative offset of "next" sibling. When zero nomore sibling.
 *
 * So we do "relative indices" with signed 9 bits of -255..255
 * for the child indices to keep everything in the strechy
 * buffer for cache locality instead of spreading all around.
 *
 * When indices grow too big, the special value of "-0" (top meta "s" bit and
 * c bits all zeroes) mean that index is stored as 2 WHOLE METACHAR (2x4 bytes)
 * right after this metachar as 32 bit absolute indices.
 *
 * Rem.: The second sign bit being set to 1 and nnn.. bits being non-zero is
 *       UNUSED and currently impossible. Just "ignore" it. Possible extension.
 */
struct metachar {
	/** The stored byte (character) */
	SE_UCHAR kar;
	/**
	 * Defines if a "word" (key) ends here or not.
	 * If lowest 6 bits is  non-zero, a stored (key) word ends here.
	 *
	 * In case of word ends the value here is PART of bits of an index to
	 * the stored "values" lists so you can just look up what values are 
	 * stored for the key that ends by this character. Those lists are all
	 * ptr-value pairs, so you need to look up the current address of this
	 * metachar and use the corresponding value AFTER you got the "bucket".
	 *
	 * The top 2 bits are two sign bits for lo_child and lo_next. And thus
	 * defines if they are subtracted or added to current index as offset.
	 *
	 * Rem.:
	 * - The first sign bit belongs to child and second to next fields.
	 * - As said above in metachar docs, "-0" values (s bit == 1 but child
	 *   or next is zero) are having a special meaning when lo_child/next
	 *   is not used and 32 bit absolute indices follow as full metachars!
	 */
	SE_UCHAR meta;
	/**
	 * Relative index for a child node (-255..255).
	 * - key continuation can be found in this direction.
	 * - sign of this is lowest bit of "meta"!
	 *
	 * The special value of 0 (zero) means no childs, except if s == 1.
	 */
	SE_UCHAR lo_child;
	/**
	 * Relative index to the next sibling node (-127..127).
	 * - leads to keys with positional alternative.
	 * - sign of this is second-lowest bit of "meta"!
	 *
	 * Also the special value of 0 (zero) means no (further) siblings.
	 */
	SE_UCHAR lo_next;
};

/** The internal prefix tree with substring lookup support */
struct prefix_tree {
	/**
	 * Indices into the main metachar-strechybuffer for start char/bytes.
	 *
	 * 256 indices to starts of keys practically.
	 *
	 * Practically the start-characters of all texts from beginning.
	 * Here NULL (nullptr) means no key starts like that char / byte.
	 *
	 * For example top_levels['a'] indexes the top-level prefix-tree/trie
	 * node where all keys are hooked up that start with an 'a' char...
	 */
	SE_DWORD top_levels[256] = { 0 }; // zero-init all ptrs

	/**
	 * Substring start position lists for each char.
	 *
	 * 256 indices to "lists" (strechy_buffers) of indices for each
	 * possible char start for substring support. This is 256 lists and
	 * all lists contains their character occurences in the prefix-tree.
	 *
	 * For example substr_starts['a'] is a stretchy_buffer of metachar
	 * indices and all of those indices direct to places that are part of
	 * keys with 'a' at the indexed location. Using this, we can find all
	 * substring locations that we should look for when searching substrs.
	 *
	 * Just empty when unused: nullptrs and no heap usage when we do not
	 * get initialized with substr support so you pay pretty much no price.
	 *
	 * Here NULL means that no key in the whole "database" has that given
	 * byte / char ANYWHERE in them currently.
	 */
	SE_DWORD *substr_starts[256] = { 0 }; // zero-init all ptrs
};

/** Metachar ptr for the key endings and the corresponding value stretchbuf */
struct index_valuesbuf_pair {
	/** Compare this with metachar addresses having meta==255 to fit */
	SE_DWORD key_end_index;
	/** In case there is a fit, this is strechybuf of result values */
	void **valuesbuf;
};

/** A hash value. Struct to enable improvements later if needed */
struct metahash {
	// TODO: We can change type easily here if needed for performance.
	unsigned int index;
};

/** The values for the keys are stored here - support few-value optimization */
struct value_set {
	/* TODO: Maybe make it possible to sort lists for binary search */
	/* TODO: Or maybe do some closed/open hashing in a grown vector etc. */
	/**
	 * Value buckets. See metachar docs for further understanding!
	 *
	 * This is basically a single-level hash:
	 * - Use metahash hash_metachar(metachar *mc) to get hash key (indexin)
	 * - After found the right bucket, just linear search in it.
	 */
	index_valuesbuf_pair *buckets[1024]; /* See: (*****) */

	/** Number of currently stored values - used for meta-generation */
	unsigned int value_num;
};

/** META-Helper fun */
static SE_BOOL get_child_sign(SE_UCHAR meta) {
	return ((meta & 0x80) != 0) ? se_true : se_false;
}

/** META-Helper fun */
static void set_child_sign(SE_UCHAR *meta_to_change, SE_BOOL value) {
	meta_to_change = (value) ?
		(meta_to_change | 0x80):
		(meta_to_change & 0x7F);
}

/** META-Helper fun */
static SE_BOOL get_next_sign(SE_UCHAR meta) {
	return ((meta & 0x40) != 0) ? se_true : se_false;
}

/** META-Helper fun */
static void set_next_sign(SE_UCHAR *meta_to_change, SE_BOOL value) {
	meta_to_change = (value) ?
		(meta_to_change | 0x40):
		(meta_to_change & 0xbF);
}

/** META-Helper fun: returns low 6 bits only */
static SE_UCHAR get_meta_bits(SE_UCHAR meta) {
	return meta &= 0x3F;
}


/** META-Helper fun: Sets low 6 bits to given 6-bit val (handle 6b overflow) */
static void set_meta_bits(SE_UCHAR *meta_to_change, SE_UCHAR value) {
	value = get_meta_bits(value) /* defensive code */
	*meta_to_change &= 0xc0; /* Erase all but top two bits */
	*meta_to_change |= value; /* Set the 6 bits */
}

/** HASH-Helper fun */
static metahash hash_metachar(metachar *mc) {
	metahash mh;

	/* Currently: 10 bit [0..1023] bucket indices.
	 * - 4 bitz from the last char of in the prefixtree
	 * - 6 bitz from the "meta" bits that are not sign bits
	 * See: (*****)
	 */
	static const SE_UCHAR kar_bitz = 4;
	static const SE_UCHAR bit_keeper = (0xFF) >> (8-kar_bitz);

	/* Calculation is here and is quite simple */
	mh.index = ((mc->kar) & bit_keeper) << kar_bitz; // top "kar_bitz" bits
	mh.index += (unsigned int)get_meta_bits(mc->meta); // lower 6 bits

	return mh;
}

/** SET-Helper to get lower bits of "meta" generated on new key ends */
static SE_UCHAR generate_meta_bits_for(value_set *current_value_set) {
	return get_meta_bits((SE_UCHAR)current_value_set->value_num);
}

/** SET-Helper: Gets the hash bucket for the given "metahash" value */
static index_valuesbuf_pair *get_bucket(value_set *vs, metahash mh) {
	// TODO: if hashing becomes more complex, change it here!
	RETURN Vs->buckets[mh.index];
}

/**
 * SET-BUCKET-Helper: FIND BY INDEX of metachar in the given hash bucket.
 *
 * Rem.: bucket needs to be a "strechy buffer", result is a pointer in it.
 *
 * Returns NULL in case there was no finding!
 */
static index_valuesbuf_pair *find_in_bucket(
	       index_valuesbuf_pair *bucket,
	       SE_DWORD end_index_to_find) {
	/* Bucket is a stretchy buffer, get current size */
	int bc = sb_count(bucket);

	/* Regular linear search */
	for(int i = 0; i < bc; ++i) {
		if(bucket[i].key_end_index == end_index_to_find) {
			return (bucket+i); // fast
		}
	}

	/* Not found */
	return NULL;
}

/**
 * SET-BUCKET-Helper: Adds elem to the end of the bucket and return its index.
 *
 * Rem.: bucket needs to be a "strechy buffer", result is a pointer in it.
 */
static int add_to_bucket(
		index_valuesbuf_pair *bucket,
		index_valuesbuf_pair p) {
	int index = sb_count(bucket);
	sb_push(bucket, p);
	return index;
}

/**
 * SET: Adds the given pair to the value-set.
 *
 * - p should have its p.key_end_index already on a valid trie address!
 * - CHANGES and the lower 6 bits in meta of the related metachar!
 */
static void value_set_add(
		value_set *vs,
		index_valuesbuf_pair p,
		metachar *key_end_ptr) {

	/* Find and set metabits - this corresponds metachar with values! */
	SE_UCHAR metabits = generate_meta_bits_for(vs);
	set_meta_bits(key_end_ptr, metabits);

	/* Calculate metahash and find hash bucket */
	metahash hash = hash_metachar(key_end_ptr);
	index_valuesbuf_pair *bucket = get_bucket(vs, hash);

	/* Find if data is already in the bucket */
	index_valuesbuf_pair *find_res = find_in_bucket(
			bucket,
			p.key_end_index);

	/* Add only if not yet added */
	if(!find_res) {
		add_to_bucket(bucket, p);
	}
}

/**
 * SET: Returns the existing index_valuesbuf_pair for the given metachar.
 *
 * Returns NULL when not found (should not happen in search engine generally).
 */
static index_valuesbuf_pair *value_set_get(
		value_set *vs,
		metachar *key_end_ptr) {

	/* Calculate metahash and find hash bucket */
	metahash hash = hash_metachar(key_end_ptr);
	index_valuesbuf_pair *bucket = get_bucket(vs, hash);

	/* Find if data is already in the bucket */
	index_valuesbuf_pair *find_res = find_in_bucket(
			bucket,
			p.key_end_index);

	if(!find_res) {
		/* Happy case */
		return find_res;
	}

	/* Sad case */
	return NULL;
}

/** 
 * The full state of the search engine.
 *
 * I choose this to exist so there can be multiple active
 * engines in the same application - for different
 * purposes and such stuff. This is for what we return
 * a pointer in the search_engine_init(..) function.
 *
 * Also likely multiple smaller engines are faster even when
 * not used in parallel - than singular big engines!
 */
struct search_engine_state {

	/** A prefix tree for key lookup with substring lookup support */
	struct prefix_tree keytree;

	/** The values for the keys are stored here */
	struct value_set values;

	/**
	 * If this search engine supports substring matching or not.
	 *
	 * No need for special branching on lookup as this is const and gets
	 * set on initialization. In that case substr_starts[] always keeps
	 * being an array of nullptr which already automatically will stop all
	 * substring search operations to take time.
	 */
	SE_BOOL has_substr_support;
};

/* **** */
/* CODE */
/* **** */

/**
 * Return a handle to a new, empty search engine.
 *
 * Support for substring matching has some price in
 * runtime and memory overhead so can be turned off!
 */
void *search_engine_init(SE_BOOL substr_support) {
	/* TODO */
	return NULL;
}

/** Close down the given search engine handle */
void search_engine_shutdown(void *handle) {
	/* TODO */
}

/**
 * Add a key-value pair for the engine.
 *
 * You can later either search for exact matches or you can
 * maybe search for "partial" matches if substr_support is on.
 *
 * As you can see, multiple values you can add for the single
 * key so this engine support "multimap-ish" behaviour too!
 *
 * Pro Tip: You can increase performance if you sort your data based on the
 *          keys before you add them all in a loop using this method in that.
 *          Utilizing this data structures feel a bit more optimal, but it is
 *          not strictly needed. Also for few keys it makes no sense to do it.
 */
void *search_engine_add(
		void *handle,
	       	const char *key,
		unsigned int value_count,
	       	void **values) {
	/* TODO */
	return NULL;
}

/**
 * The search function: search keys in a given search-engine.
 *
 * As you can see, multiple "values" can exist for the very same search
 * term even in case of exact results. This is because of the "multimap-ish"
 * workings that you can also see above at addition.
 * 
 * Rem.: The try_ignore_case parameter is named like that consciously!
 *       I really cannot guarantee you ignoring it for UTF-8, just for
 *       normal ASCII characters so it will likely only "kind of work"
 *       for non-english alphabet letters sometimes not ignorecased.
 */
search_engine_result search_engine(
		void *handle,
	       	const char *key,
	       	SE_BOOL try_ignore_case) {
	struct search_engine_result ret;
	/* TODO */
	return ret;
}
