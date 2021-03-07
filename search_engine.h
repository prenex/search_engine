#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H
/*
 * search_engine.h version 0.2
 *
 * Simple single-header and single-c search engine library
 * which lets you use prefix/infix/suffix (partial) search
 * on keys for which you provide values.
 *
 * This file contains smallest possible header for compile.
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

#ifndef NO_MODERN_TYPEZ
/* DEFAULT: Modern compiler - cleaner: use stdint.h */

#include "stdint.h"
#define SE_UCHAR uint8_t
#define SE_CHAR int8_t
#define SE_WORD uint16_t
#define SE_DWORD uint32_t

#else

/* OPTIONAL: Non-modern compiler - do not use uint8_t and such! */
#define SE_UCHAR unsigned char
#define SE_UCHAR char
#define SE_WORD unsigned short
#define SE_DWORD unsigned int

#endif /* NO_MODERN_TYPEZ */

typedef enum { se_false, se_true } SE_BOOL;

/** Result of a search */
struct search_engine_result {
	/** Number of exact results */
	unsigned int exact_result_no;
	/** Array of exact results */
	void **exact_results;
	/** Number of prefix results */
	unsigned int prefix_result_no;
	/** Array of prefix results */
	void **prefix_results;
	/** Number of substring results - if supported */
	unsigned int substring_result_no;
	/** Array of substring results - if supported */
	void **substring_results;
};

/**
 * Return a handle to a new, empty search engine.
 *
 * Support for substring matching has some price in
 * runtime and memory overhead so can be turned off!
 */
void *search_engine_init(SE_BOOL substr_support);

/** Close down the given search engine handl handle */
void search_engine_shutdown(void *handle);

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
		const SE_CHAR *key,
		unsigned int value_count,
	       	void **values);

/**
 * The search function: search keys in a given search-engine.
 *
 * As you can see, multiple "values" can exist for the very same search
 * term even in case of exact results. This is because of the "multimap-ish"
 * workings that you can also see above at addition.
 *
 * As you can see, you can turn off substr_support here too even if you
 * enabled that for the data structure. In that case you only pay a smaller
 * overhead in memory usage and of course related cache locality penalty,
 * but not longer search by looking at various parts a word can start.
 *
 * Rem.: The try_ignore_case parameter is named like that consciously!
 *       I really cannot guarantee you ignoring it for UTF-8, just for
 *       normal ASCII characters so it will likely only "kind of work"
 *       for non-english alphabet letters sometimes not ignorecased.
 */
search_engine_result search_engine(
		void *handle,
		const SE_CHAR *key,
	       	SE_BOOL substr_support,
	       	SE_BOOL try_ignore_case);

#endif /* SEARCH_ENGINE_H */
