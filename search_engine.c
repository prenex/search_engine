/*
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

#include "stdint.h"

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
	uint8_t kar;
	uint8_t meta;
	uint8_t lo_child;
	uint8_t lo_next;
};

/** 
 * The full state of the search engine.
 *
 * I choose this way so there can be multiple active
 * engines in the same application - for different
 * purposes and such stuff. This is for what we return
 * a pointer in the search_engine_init(..) function.
 */
struct search_engine_state {
// TODO
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
