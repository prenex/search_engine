# search_engine.h

A single-header library (also there is a .c file) written in regular C for
best compatibility that you can hook in anywhere in case you need to do some
kind of text search.

Is it a small text editor or IDE for expanding words you wish? Good. Is it a
GUI application where you want to search in some in-memory data structures?
Also good! Are you building a web-app where you want search? Excellent!

This little thing all gets you covered if you use it well!

# Features

* Easy to add to any project: just single header and implementation file in C!
* Exact, prefix and (if configured) substring match results!
* Multimap-like working: you can add multiple values to same key!
* Likely fast enough for all purposes. Stored as cache-friendly trie-like data.
* Small memory footprint. O(n) for the number of characters in stored keys.
* You can run multiple engines at the same time easily.
* This also means you can create multi-threaded engines that delegate to us!
* Or even multi-node engines if you wish on multiple machines for big stuffz!
* Free and open source - very enabling licence I think but contact me if not.
* "suckless.org" philosophy: no huge dependency chains just "KISS" everywhere!

# Details

What substring matching is? Not that the search term and one stored key shares
"some substring somewhere", but practically that the search term can be just a
substring of the key if this is turned on and we still get the result. You can
however use suffix trees to generate multiple searches like this and it will
be likely fast enough if you really want that other kind of working that you
get a result even if some substring is shared between stored keys and terms...

Even in substring search being turned on, you can still expect good (cool)
performance. It is of course slightly slower on big data sets, but further
analysis will be needed how much slower. Compacting the tree would help maybe
but it is hard to tell how much because practically all data we will look at
have a possibility to be a substring you search for and thus MUST be looked...

# About resource usage

It is about 20 bytes in full mode and around 8-14 bytes generally per key char.
Then you also pay a constant price with some 1-2 kilobytes for metadata - most
of which is sized like that because of pointers.

Also code footprint is pretty small too. In case you are in "so much embedded"
situations that this is too heavy for you, consider just doing linear search,
but in a more cache-optimized way. Something really simple like what dmenu do!

Searching for exact key and search-term fit would be O(m) where m is the number
of characters you have in the search term. For prefix searches, the time is
practically O(m + k) I think. Where m is same as above, k is the number of the
matches you will get in your results.

Searching for substrings is practically O(l * (m + k)), where the m and k is
the same as above and l means something like "at this many places we need to
check if a substring might start to correspond your search term". This "l" is
however surely asymptotically bound by "n" (the number of all letters in all
the keys you give). That is the "n" value you see at memory usage too. So all
in all, to me it seems practically both memory usage and runtime is O(n) where
this "n" value is at most the number of characters in all keys added together.

If you are doing really-big data - like for example you have dreams of redoing
what google does in your fairy dreams with scraping websites and all the stuff,
then consider running this multiple cores, multiple nodes and spread all the
data between nodes - then at searches just ask all of them to answer and you
can then unify the data. As "value" is just a void* you can add "scores" and
such if you really want to and then sort your results based on that. With the
separate nodes approach, you can actually do sort per node, then merge sort,
as a whole between nodes and get results in the order you wish them to be. Also
likely you would have some "web crawlers" and data would be not immediately
updated, but nodes would rebuild their data after new packages are sent to the
given node. When that happens, you can ensure that the node / thread gets a
sorted (by key) data set on rebuilding / updating based of scraping and that
will nicely affect search performance with a bit of extra (little) boost.

Also source code is there. It is simple and no extra stuff is done, just very
basic optimizations to try ensuring at least what I call cache locality basics.

# Licence

You can also use this for closes source or commercial stuff, but need mention.
See details in the files themselves.

# TODO

* More sophisticated writeup of LICENCE
* Maybe ability to delete from the engine? Currently you rebuild all for now.
* Maybe I could add "suffix tree" generation for searching any common substr?
* search_engine_2 data structures and functions workin on many engines!

The search_engine_2 is a good idea, because we can then split maybe the whole
database into 16 number of search_engines which likely all work in the fast
paths always and with better cache optimality, so likely with a little bit of
an overhead of having multiple engines for book-keeping you get a much better
performance out of the whole when storing really much data. I think this is a
good thing to write. OR EVEN BETTER: maybe we should somehow organize the code
to be able to use both search_engines and search_engine2s in search_engine2s
which in turn gives us practically "endless" scalability.
