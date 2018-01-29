#ifndef CITRUS_TRIE_H_
#define CITRUS_TRIE_H_

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * A multilevel table structure to facilitate ku/ten <-> Unicode mapping.
 */
#ifndef VALUE_TYPE
# define VALUE_TYPE int32_t
# define STRINGIZE(s) #s
# define VALUE_TYPE_STRING STRINGIZE(VALUE_TYPE) /* "int32_t" */
#endif
#define INVALID_VALUE 0xffffffff

typedef uint64_t citrus_trie_key_t;

typedef struct citrus_trie_header {
	unsigned int th_flags;
	size_t th_bitwidth;
	size_t th_bitmask;
	size_t th_off;
	size_t th_level;
#ifdef DEBUG_TRIE
	size_t th_size; /* How much memory is consumed by this trie */
#endif
	struct citrus_trie_node *th_root;
} *citrus_trie_header_t;

typedef struct citrus_trie_node {
	size_t tr_len;
	union citrus_trie_node_union {
		struct citrus_trie_node **u_child;
		VALUE_TYPE      *u_value;
	} tr_u;
} *citrus_trie_node_t;

#ifdef DEBUG_TRIE
# define citrus_trie_malloc(h, x) (h->th_size += (x), malloc(x))
#else
# define citrus_trie_malloc(h, x) malloc(x)
#endif

citrus_trie_header_t citrus_trie_create(unsigned int, size_t, size_t, size_t, size_t);
citrus_trie_node_t citrus_trie_node_create(citrus_trie_header_t, size_t, size_t len);
int citrus_trie_node_insert(citrus_trie_header_t, citrus_trie_node_t, size_t, citrus_trie_key_t, VALUE_TYPE);
int citrus_trie_insert(citrus_trie_header_t, citrus_trie_key_t, VALUE_TYPE);
VALUE_TYPE citrus_trie_node_lookup(citrus_trie_header_t, citrus_trie_node_t, size_t, citrus_trie_key_t);
VALUE_TYPE citrus_trie_lookup(citrus_trie_header_t, citrus_trie_key_t);
citrus_trie_header_t citrus_trie_create_from_flat(VALUE_TYPE *, size_t, size_t);
void citrus_trie_node_destroy(citrus_trie_node_t, size_t);
void citrus_trie_destroy(citrus_trie_header_t);

void citrus_trie_init(citrus_trie_header_t, size_t *, VALUE_TYPE *);
void citrus_trie_dump(citrus_trie_header_t, char *, char *, int);
citrus_trie_header_t citrus_trie_load(char *);
#endif /* CITRUS_TRIE_H_ */
