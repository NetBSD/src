/* sl_malloc.c - malloc routines using a per-thread slab */
/* $OpenLDAP: pkg/ldap/servers/slapd/sl_malloc.c,v 1.39.2.6 2008/02/11 23:34:15 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2008 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"

static struct slab_object * slap_replenish_sopool(struct slab_heap* sh);
#ifdef SLAPD_UNUSED
static void print_slheap(int level, void *ctx);
#endif

void
slap_sl_mem_destroy(
	void *key,
	void *data
)
{
	struct slab_heap *sh = data;
	int pad = 2*sizeof(int)-1, pad_shift;
	int order_start = -1, i;
	struct slab_object *so;

	if (sh->sh_stack) {
		ber_memfree_x(sh->sh_base, NULL);
		ber_memfree_x(sh, NULL);
	} else {
		pad_shift = pad - 1;
		do {
			order_start++;
		} while (pad_shift >>= 1);

		for (i = 0; i <= sh->sh_maxorder - order_start; i++) {
			so = LDAP_LIST_FIRST(&sh->sh_free[i]);
			while (so) {
				struct slab_object *so_tmp = so;
				so = LDAP_LIST_NEXT(so, so_link);
				LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, so_tmp, so_link);
			}
			ch_free(sh->sh_map[i]);
		}
		ch_free(sh->sh_free);
		ch_free(sh->sh_map);

		so = LDAP_LIST_FIRST(&sh->sh_sopool);
		while (so) {
			struct slab_object *so_tmp = so;
			so = LDAP_LIST_NEXT(so, so_link);
			if (!so_tmp->so_blockhead) {
				LDAP_LIST_REMOVE(so_tmp, so_link);
			}
		}
		so = LDAP_LIST_FIRST(&sh->sh_sopool);
		while (so) {
			struct slab_object *so_tmp = so;
			so = LDAP_LIST_NEXT(so, so_link);
			ch_free(so_tmp);
		}
		ber_memfree_x(sh->sh_base, NULL);
		ber_memfree_x(sh, NULL);
	}
}

BerMemoryFunctions slap_sl_mfuncs =
	{ slap_sl_malloc, slap_sl_calloc, slap_sl_realloc, slap_sl_free };

void
slap_sl_mem_init()
{
	ber_set_option( NULL, LBER_OPT_MEMORY_FNS, &slap_sl_mfuncs );
}

#ifdef NO_THREADS
static struct slab_heap *slheap;
#endif

void *
slap_sl_mem_create(
	ber_len_t size,
	int stack,
	void *ctx,
	int new
)
{
	struct slab_heap *sh;
	ber_len_t size_shift;
	int pad = 2*sizeof(int)-1, pad_shift;
	int order = -1, order_start = -1, order_end = -1;
	int i;
	struct slab_object *so;

#ifdef NO_THREADS
	sh = slheap;
#else
	void *sh_tmp = NULL;
	ldap_pvt_thread_pool_getkey(
		ctx, (void *)slap_sl_mem_init, &sh_tmp, NULL );
	sh = sh_tmp;
#endif

	if ( !new )
		return sh;

	/* round up to doubleword boundary */
	size += pad;
	size &= ~pad;

	if (stack) {
		if (!sh) {
			sh = ch_malloc(sizeof(struct slab_heap));
			sh->sh_base = ch_malloc(size);
#ifdef NO_THREADS
			slheap = sh;
#else
			ldap_pvt_thread_pool_setkey(ctx, (void *)slap_sl_mem_init,
				(void *)sh, slap_sl_mem_destroy, NULL, NULL);
#endif
		} else if ( size > (char *)sh->sh_end - (char *)sh->sh_base ) {
			void	*newptr;

			newptr = ch_realloc( sh->sh_base, size );
			if ( newptr == NULL ) return NULL;
			sh->sh_base = newptr;
		}
		sh->sh_last = sh->sh_base;
		sh->sh_end = (char *) sh->sh_base + size;
		sh->sh_stack = stack;
		return sh;
	} else {
		size_shift = size - 1;
		do {
			order_end++;
		} while (size_shift >>= 1);

		pad_shift = pad - 1;
		do {
			order_start++;
		} while (pad_shift >>= 1);

		order = order_end - order_start + 1;

		if (!sh) {
			sh = (struct slab_heap *) ch_malloc(sizeof(struct slab_heap));
			sh->sh_base = ch_malloc(size);
#ifdef NO_THREADS
			slheap = sh;
#else
			ldap_pvt_thread_pool_setkey(ctx, (void *)slap_sl_mem_init,
				(void *)sh, slap_sl_mem_destroy, NULL, NULL);
#endif
		} else {
			for (i = 0; i <= sh->sh_maxorder - order_start; i++) {
				so = LDAP_LIST_FIRST(&sh->sh_free[i]);
				while (so) {
					struct slab_object *so_tmp = so;
					so = LDAP_LIST_NEXT(so, so_link);
					LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, so_tmp, so_link);
				}
				ch_free(sh->sh_map[i]);
			}
			ch_free(sh->sh_free);
			ch_free(sh->sh_map);

			so = LDAP_LIST_FIRST(&sh->sh_sopool);
			while (so) {
				struct slab_object *so_tmp = so;
				so = LDAP_LIST_NEXT(so, so_link);
				if (!so_tmp->so_blockhead) {
					LDAP_LIST_REMOVE(so_tmp, so_link);
				}
			}
			so = LDAP_LIST_FIRST(&sh->sh_sopool);
			while (so) {
				struct slab_object *so_tmp = so;
				so = LDAP_LIST_NEXT(so, so_link);
				ch_free(so_tmp);
			}

			if (size > (char *)sh->sh_end - (char *)sh->sh_base) {
				void	*newptr;

				newptr = realloc( sh->sh_base, size );
				if ( newptr == NULL ) return NULL;
				sh->sh_base = newptr;
			}
		}
		sh->sh_end = (char *)sh->sh_base + size;
		sh->sh_maxorder = order_end;

		sh->sh_free = (struct sh_freelist *)
						ch_malloc(order * sizeof(struct sh_freelist));
		for (i = 0; i < order; i++) {
			LDAP_LIST_INIT(&sh->sh_free[i]);
		}

		LDAP_LIST_INIT(&sh->sh_sopool);

		if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
			slap_replenish_sopool(sh);
		}
		so = LDAP_LIST_FIRST(&sh->sh_sopool);
		LDAP_LIST_REMOVE(so, so_link);
		so->so_ptr = sh->sh_base;

		LDAP_LIST_INSERT_HEAD(&sh->sh_free[order-1], so, so_link);

		sh->sh_map = (unsigned char **)
					ch_malloc(order * sizeof(unsigned char *));
		for (i = 0; i < order; i++) {
			int shiftamt = order_start + 1 + i;
			int nummaps = size >> shiftamt;
			assert(nummaps);
			nummaps >>= 3;
			if (!nummaps) nummaps = 1;
			sh->sh_map[i] = (unsigned char *) ch_malloc(nummaps);
			memset(sh->sh_map[i], 0, nummaps);
		}
		sh->sh_stack = stack;
		return sh;
	}
}

void
slap_sl_mem_detach(
	void *ctx,
	void *memctx
)
{
#ifdef NO_THREADS
	slheap = NULL;
#else
	/* separate from context */
	ldap_pvt_thread_pool_setkey( ctx, (void *)slap_sl_mem_init,
		NULL, 0, NULL, NULL );
#endif
}

void *
slap_sl_malloc(
    ber_len_t	size,
    void *ctx
)
{
	struct slab_heap *sh = ctx;
	ber_len_t size_shift;
	int pad = 2*sizeof(int)-1, pad_shift;
	int order = -1, order_start = -1;
	struct slab_object *so_new, *so_left, *so_right;
	ber_len_t *ptr, *newptr;
	unsigned long diff;
	int i, j;

#ifdef SLAP_NO_SL_MALLOC
	return ber_memalloc_x( size, NULL );
#endif

	/* ber_set_option calls us like this */
	if (!ctx) return ber_memalloc_x(size, NULL);

	/* round up to doubleword boundary */
	size += sizeof(ber_len_t) + pad;
	size &= ~pad;

	if (sh->sh_stack) {
		if ((char *)sh->sh_last + size >= (char *)sh->sh_end) {
			Debug(LDAP_DEBUG_TRACE,
				"slap_sl_malloc of %lu bytes failed, using ch_malloc\n",
				(long)size, 0, 0);
			return ch_malloc(size);
		}
		newptr = sh->sh_last;
		*newptr++ = size - sizeof(ber_len_t);
		sh->sh_last = (char *) sh->sh_last + size;
		return( (void *)newptr );
	} else {
		size_shift = size - 1;
		do {
			order++;
		} while (size_shift >>= 1);

		pad_shift = pad - 1;
		do {
			order_start++;
		} while (pad_shift >>= 1);

		for (i = order; i <= sh->sh_maxorder &&
				LDAP_LIST_EMPTY(&sh->sh_free[i-order_start]); i++);

		if (i == order) {
			so_new = LDAP_LIST_FIRST(&sh->sh_free[i-order_start]);
			LDAP_LIST_REMOVE(so_new, so_link);
			ptr = so_new->so_ptr;
			diff = (unsigned long)((char*)ptr -
					(char*)sh->sh_base) >> (order + 1);
			sh->sh_map[order-order_start][diff>>3] |= (1 << (diff & 0x7));
			*ptr++ = size - sizeof(ber_len_t);
			LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, so_new, so_link);
			return((void*)ptr);
		} else if (i <= sh->sh_maxorder) {
			for (j = i; j > order; j--) {
				so_left = LDAP_LIST_FIRST(&sh->sh_free[j-order_start]);
				LDAP_LIST_REMOVE(so_left, so_link);
				if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
					slap_replenish_sopool(sh);
				}
				so_right = LDAP_LIST_FIRST(&sh->sh_sopool);
				LDAP_LIST_REMOVE(so_right, so_link);
				so_right->so_ptr = (void *)((char *)so_left->so_ptr + (1 << j));
				if (j == order + 1) {
					ptr = so_left->so_ptr;
					diff = (unsigned long)((char*)ptr -
							(char*)sh->sh_base) >> (order+1);
					sh->sh_map[order-order_start][diff>>3] |=
							(1 << (diff & 0x7));
					*ptr++ = size - sizeof(ber_len_t);
					LDAP_LIST_INSERT_HEAD(
							&sh->sh_free[j-1-order_start], so_right, so_link);
					LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, so_left, so_link);
					return((void*)ptr);
				} else {
					LDAP_LIST_INSERT_HEAD(
							&sh->sh_free[j-1-order_start], so_right, so_link);
					LDAP_LIST_INSERT_HEAD(
							&sh->sh_free[j-1-order_start], so_left, so_link);
				}
			}
		} else {
			Debug( LDAP_DEBUG_TRACE,
				"slap_sl_malloc of %lu bytes failed, using ch_malloc\n",
				(long)size, 0, 0);
			return (void*)ch_malloc(size);
		}
	}

	/* FIXME: missing return; guessing... */
	return NULL;
}

void *
slap_sl_calloc( ber_len_t n, ber_len_t size, void *ctx )
{
	void *newptr;

	newptr = slap_sl_malloc( n*size, ctx );
	if ( newptr ) {
		memset( newptr, 0, n*size );
	}
	return newptr;
}

void *
slap_sl_realloc(void *ptr, ber_len_t size, void *ctx)
{
	struct slab_heap *sh = ctx;
	int pad = 2*sizeof(int) -1;
	ber_len_t *p = (ber_len_t *)ptr, *newptr;

	if (ptr == NULL)
		return slap_sl_malloc(size, ctx);

#ifdef SLAP_NO_SL_MALLOC
	return ber_memrealloc_x( ptr, size, NULL );
#endif

	/* Not our memory? */
	if (!sh || ptr < sh->sh_base || ptr >= sh->sh_end) {
		/* duplicate of realloc behavior, oh well */
		newptr = ber_memrealloc_x(ptr, size, NULL);
		if (newptr) {
			return newptr;
		}
		Debug(LDAP_DEBUG_ANY, "ch_realloc of %lu bytes failed\n",
				(long) size, 0, 0);
		assert(0);
		exit( EXIT_FAILURE );
	}

	if (size == 0) {
		slap_sl_free(ptr, ctx);
		return NULL;
	}

	if (sh->sh_stack) {
		/* round up to doubleword boundary */
		size += pad + sizeof( ber_len_t );
		size &= ~pad;

		/* Never shrink blocks */
		if (size <= p[-1]) {
			newptr = p;
	
		/* If reallocing the last block, we can grow it */
		} else if ((char *)ptr + p[-1] == sh->sh_last &&
			(char *)ptr + size < (char *)sh->sh_end ) {
			newptr = p;
			sh->sh_last = (char *)sh->sh_last + size - p[-1];
			p[-1] = size;
	
		/* Nowhere to grow, need to alloc and copy */
		} else {
			newptr = slap_sl_malloc(size, ctx);
			AC_MEMCPY(newptr, ptr, p[-1]);
		}
		return newptr;
	} else {
		void *newptr2;

		newptr2 = slap_sl_malloc(size, ctx);
		if (size < p[-1]) {
			AC_MEMCPY(newptr2, ptr, size);
		} else {
			AC_MEMCPY(newptr2, ptr, p[-1]);
		}
		slap_sl_free(ptr, ctx);
		return newptr2;
	}
}

void
slap_sl_free(void *ptr, void *ctx)
{
	struct slab_heap *sh = ctx;
	int size, size_shift, order_size;
	int pad = 2*sizeof(int)-1, pad_shift;
	ber_len_t *p = (ber_len_t *)ptr, *tmpp;
	int order_start = -1, order = -1;
	struct slab_object *so;
	unsigned long diff;
	int i, inserted = 0;

	if (!ptr)
		return;

#ifdef SLAP_NO_SL_MALLOC
	ber_memfree_x( ptr, NULL );
	return;
#endif

	if (!sh || ptr < sh->sh_base || ptr >= sh->sh_end) {
		ber_memfree_x(ptr, NULL);
	} else if (sh->sh_stack && (char *)ptr + p[-1] == sh->sh_last) {
		p--;
		sh->sh_last = p;
	} else if (!sh->sh_stack) {
		size = *(--p);
		size_shift = size + sizeof(ber_len_t) - 1;
		do {
			order++;
		} while (size_shift >>= 1);

		pad_shift = pad - 1;
		do {
			order_start++;
		} while (pad_shift >>= 1);

		for (i = order, tmpp = p; i <= sh->sh_maxorder; i++) {
			order_size = 1 << (i+1);
			diff = (unsigned long)((char*)tmpp - (char*)sh->sh_base) >> (i+1);
			sh->sh_map[i-order_start][diff>>3] &= (~(1 << (diff & 0x7)));
			if (diff == ((diff>>1)<<1)) {
				if (!(sh->sh_map[i-order_start][(diff+1)>>3] &
						(1<<((diff+1)&0x7)))) {
					so = LDAP_LIST_FIRST(&sh->sh_free[i-order_start]);
					while (so) {
						if ((char*)so->so_ptr == (char*)tmpp) {
							LDAP_LIST_REMOVE( so, so_link );
						} else if ((char*)so->so_ptr ==
								(char*)tmpp + order_size) {
							LDAP_LIST_REMOVE(so, so_link);
							break;
						}
						so = LDAP_LIST_NEXT(so, so_link);
					}
					if (so) {
						if (i < sh->sh_maxorder) {
							inserted = 1;
							so->so_ptr = tmpp;
							LDAP_LIST_INSERT_HEAD(&sh->sh_free[i-order_start+1],
									so, so_link);
						}
						continue;
					} else {
						if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
							slap_replenish_sopool(sh);
						}
						so = LDAP_LIST_FIRST(&sh->sh_sopool);
						LDAP_LIST_REMOVE(so, so_link);
						so->so_ptr = tmpp;
						LDAP_LIST_INSERT_HEAD(&sh->sh_free[i-order_start],
								so, so_link);
						break;

						Debug(LDAP_DEBUG_TRACE, "slap_sl_free: "
							"free object not found while bit is clear.\n",
							0, 0, 0);
						assert(so != NULL);

					}
				} else {
					if (!inserted) {
						if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
							slap_replenish_sopool(sh);
						}
						so = LDAP_LIST_FIRST(&sh->sh_sopool);
						LDAP_LIST_REMOVE(so, so_link);
						so->so_ptr = tmpp;
						LDAP_LIST_INSERT_HEAD(&sh->sh_free[i-order_start],
								so, so_link);
					}
					break;
				}
			} else {
				if (!(sh->sh_map[i-order_start][(diff-1)>>3] &
						(1<<((diff-1)&0x7)))) {
					so = LDAP_LIST_FIRST(&sh->sh_free[i-order_start]);
					while (so) {
						if ((char*)so->so_ptr == (char*)tmpp) {
							LDAP_LIST_REMOVE(so, so_link);
						} else if ((char*)tmpp == (char *)so->so_ptr + order_size) {
							LDAP_LIST_REMOVE(so, so_link);
							tmpp = so->so_ptr;
							break;
						}
						so = LDAP_LIST_NEXT(so, so_link);
					}
					if (so) {
						if (i < sh->sh_maxorder) {
							inserted = 1;
							LDAP_LIST_INSERT_HEAD(&sh->sh_free[i-order_start+1],									so, so_link);
							continue;
						}
					} else {
						if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
							slap_replenish_sopool(sh);
						}
						so = LDAP_LIST_FIRST(&sh->sh_sopool);
						LDAP_LIST_REMOVE(so, so_link);
						so->so_ptr = tmpp;
						LDAP_LIST_INSERT_HEAD(&sh->sh_free[i-order_start],
								so, so_link);
						break;

						Debug(LDAP_DEBUG_TRACE, "slap_sl_free: "
							"free object not found while bit is clear.\n",
							0, 0, 0 );
						assert(so != NULL);

					}
				} else {
					if ( !inserted ) {
						if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
							slap_replenish_sopool(sh);
						}
						so = LDAP_LIST_FIRST(&sh->sh_sopool);
						LDAP_LIST_REMOVE(so, so_link);
						so->so_ptr = tmpp;
						LDAP_LIST_INSERT_HEAD(&sh->sh_free[i-order_start],
								so, so_link);
					}
					break;
				}
			}
		}
	}
}

void *
slap_sl_context( void *ptr )
{
	struct slab_heap *sh;
	void *ctx, *sh_tmp;

	if ( slapMode & SLAP_TOOL_MODE ) return NULL;

#ifdef NO_THREADS
	sh = slheap;
#else
	ctx = ldap_pvt_thread_pool_context();

	sh_tmp = NULL;
	ldap_pvt_thread_pool_getkey(
		ctx, (void *)slap_sl_mem_init, &sh_tmp, NULL);
	sh = sh_tmp;
#endif

	if (sh && ptr >= sh->sh_base && ptr <= sh->sh_end) {
		return sh;
	}
	return NULL;
}

static struct slab_object *
slap_replenish_sopool(
    struct slab_heap* sh
)
{
    struct slab_object *so_block;
    int i;

    so_block = (struct slab_object *)ch_malloc(
                    SLAP_SLAB_SOBLOCK * sizeof(struct slab_object));

    if ( so_block == NULL ) {
        return NULL;
    }

    so_block[0].so_blockhead = 1;
    LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, &so_block[0], so_link);
    for (i = 1; i < SLAP_SLAB_SOBLOCK; i++) {
        so_block[i].so_blockhead = 0;
        LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, &so_block[i], so_link );
    }

    return so_block;
}

#ifdef SLAPD_UNUSED
static void
print_slheap(int level, void *ctx)
{
	struct slab_heap *sh = ctx;
	int order_start = -1;
	int pad = 2*sizeof(int)-1, pad_shift;
	struct slab_object *so;
	int i, j, once = 0;

	if (!ctx) {
		Debug(level, "NULL memctx\n", 0, 0, 0);
		return;
	}

	pad_shift = pad - 1;
	do {
		order_start++;
	} while (pad_shift >>= 1);

	Debug(level, "sh->sh_maxorder=%d\n", sh->sh_maxorder, 0, 0);

	for (i = order_start; i <= sh->sh_maxorder; i++) {
		once = 0;
		Debug(level, "order=%d\n", i, 0, 0);
		for (j = 0; j < (1<<(sh->sh_maxorder-i))/8; j++) {
			Debug(level, "%02x ", sh->sh_map[i-order_start][j], 0, 0);
			once = 1;
		}
		if (!once) {
			Debug(level, "%02x ", sh->sh_map[i-order_start][0], 0, 0);
		}
		Debug(level, "\n", 0, 0, 0);
		Debug(level, "free list:\n", 0, 0, 0);
		so = LDAP_LIST_FIRST(&sh->sh_free[i-order_start]);
		while (so) {
			Debug(level, "%lx\n", (unsigned long) so->so_ptr, 0, 0);
			so = LDAP_LIST_NEXT(so, so_link);
		}
	}
}
#endif
