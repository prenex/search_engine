/*
 * search_engine.c version 1.0
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
      return (void *) (2*sizeof(int)); // try to force a NULL pointer exception later
   }
}
#endif // STB_STRETCHY_BUFFER_H_INCLUDED

/* END: stretchy_buffer.h */
/* ********************** */

/* **** */
/* DATA */
/* **** */

/**
 * Used in my prefix-tree data structures.
 *
 * We do "relative indices" with signed 8 bits of -127..127
 * for the child indices to keep everything in the strechy
 * buffer for cache locality instead of spreading all around.
 *
 * When indices grow too big, the special value of -128 will
 * mean that index is stored as a WHOLE METACHAR (4 bytes),
 * right after this metachar.
 *
 * TODO:  I will figure it out that if I am using 16 bits of
 * that for each index is better or two full 4 bytes is...
 */
struct metachar {
	/** The stored byte (character) */
	unsigned char kar;
	/**
	 * Defines if a "word" (key) ends here or not. If non-zero, a word end.
	 *
	 * In case of word ends the value here is an index to stored "values"
	 * lists so you can just look up what values are stored for the key
	 * that ends by this character. In the special case of the value  255,
	 * You need to do a pointer based lookup however! Then there were so
	 * many "values" stored in the system that there are more than 0..254
	 * and the lookup will happen based on hashing into buckets and
	 * searching the corresponding value-list through simple 254-way hash.
	 *
	 * Examples:
	 *
	 * - Value of 0 means no keys end at this position. Even if you would
	 *   find a full match until now, you only found a stored key being a
	 *   prefix of the search term if you get to this point.
	 * - A value of 3 means that you can grab all value_set.fastvals
	 * - A value of 255 means that you need to generate a hash from the
	 *   pointer / location of this metachar in the prefix_tree and use
	 *   this pointer to hash into 0..255 buckets where strechy buffers
	 *   contain N number of pointers. A (linear?) search is to be done on
	 *   the pointers and if found the pointer of the metachar, in question
	 *   the other ptr buffer near the pointer - so buckets have two
	 *   pointers per ptr: the pointer to compare and the one as strechbuf
	 *   - will contain the streachy-buffer of all the values for this key.
	 *
	 */
	unsigned char meta;
	/**
	 * Relative index for a child node (-127..127).
	 * - key continuation can be found in this direction.
	 *
	 * Here special -128 means next two metachar is two 32 bit absolute
	 * index in the stretchy_buffer of metachars!
	 *
	 * Also the special value of 0 (zero) means no childs.
	 */
	signed char lo_child;
	/**
	 * Relative index to the next sibling node (-127..127).
	 * - leads to keys with positional alternative.
	 *
	 * Here special -128 means next two metachar is two 32 bit absolute
	 * index in the stretchy_buffer of metachars!
	 *
	 * Also the special value of 0 (zero) means no (further) siblings.
	 */
	signed char lo_next;
};

/** The internal prefix tree with substring lookup support */
struct prefix_tree {
	/**
	 * Pointers to metachar-strechybuffers for each starting char / byte.
	 *
	 * 256 pointers to starts of keys practically.
	 *
	 * Practically the start-characters of all texts from beginning.
	 * Here NULL (nullptr) means no key starts like that char / byte.
	 *
	 * For example top_levels['a'] points to the top-level prefix-tree/trie
	 * node where all keys are hooked up that start with an 'a' char...
	 */
	metachar *top_levels[256] = { 0 }; // zero-init all ptrs

	/**
	 * 256 pointers to "lists" (strechy_buffers) of pointers for each
	 * possible char start for substring support. This is 256 lists and
	 * all lists contains their character occurences in the prefix-tree.
	 *
	 * For example substr_starts['a'] is a stretchy_buffer of metachar*
	 * pointers and all of those pointers point to places that are part of
	 * keys with 'a' at the pointed location. Using this, we can find all
	 * substring locations that we should look for when searching substrs.
	 *
	 * Just empty when unused: nullptrs and no heap usage when we do not
	 * get initialized with substr support so you pay pretty much no price.
	 *
	 * Here NULL means that no key in the whole "database" has that given
	 * byte / char ANYWHERE in them currently.
	 */
	metachar **substr_starts[256] = { 0 }; // zero-init all ptrs
};

/** Metachar ptr for the key endings and the corresponding value stretchbuf */
struct ptr_valuesbuf_pair {
	/** Compare this with metachar addresses having meta==255 to fit */
	metachar *key_end_ptr;
	/** In case there is a fit, this is strechybuf of result values */
	void **valuesbuf;
};

/** The values for the keys are stored here - support few-value optimization */
struct value_set {
	/** Fastaccess to first 254 values. Better be the most common ones! */
	void *fastvals[254];

	/*
	 * TODO: maybe having this simple hashing as 64-way and doing more
	 * linear searching is actually better because of cache efficiency?
	 */
	/* TODO: Maybe make it possible to sort lists (key_end_ptr binsearc) */
	/**
	 * Slower access to non-first values.
	 *
	 * This is basically a really-really simple 256-way hash and you need
	 * to hash the address of the key-ending metachar down to a single
	 * unsigned byte then index into this array to get the corresponding
	 * bucket. If you are there, you need a linear search to compare the
	 * buckets
	 */
	ptr_valuesbuf_pair *slowvals[256];

	/** Number of currently stored values - used for few-value opt */
	unsigned int valueNum;
};

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

/** Close down the given search engine handl handle */
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
