/*	$NetBSD: cache_octeon.h,v 1.2.16.2 2017/12/03 11:36:27 jdolecek Exp $	*/

#define	CACHE_OCTEON_I			0
#define	CACHE_OCTEON_D			1

#define	CACHEOP_OCTEON_INV_ALL			(0 << 2)	/* I, D */
#define	CACHEOP_OCTEON_INDEX_LOAD_TAG		(1 << 2)	/* I, D */
#define	CACHEOP_OCTEON_BITMAP_STORE		(3 << 2)	/* I */
#define	CACHEOP_OCTEON_VIRTUAL_TAG_INV		(4 << 2)	/* D */

#if !defined(_LOCORE)

/*
 * cache_octeon_invalidate:
 *
 *	Invalidate all cahce blocks.
 *	Argument "op" must be CACHE_OCTEON_I or CACHE_OCTEON_D.
 *	In Octeon specification, invalidate instruction works
 *	all cache blocks.
 */
#define	cache_octeon_invalidate(op)					\
do {									\
	__asm __volatile(						\
		".set noreorder					\n\t"	\
		"cache %0, 0($0)				\n\t"	\
		".set reorder"						\
	    :								\
	    : "i" (op)							\
	    : "memory");						\
} while (/*CONSTCOND*/0)

/*
 * cache_octeon_op_line:
 *
 *	Perform the specified cache operation on a single line.
 */
#define	cache_op_octeon_line(va, op)					\
do {									\
	__asm __volatile(						\
		".set noreorder					\n\t"	\
		"cache %1, 0(%0)				\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va), "i" (op)					\
	    : "memory");						\
} while (/*CONSTCOND*/0)

void octeon_icache_sync_all(void);
void octeon_icache_sync_range(register_t va, vsize_t size);
void octeon_icache_sync_range_index(vaddr_t va, vsize_t size);
void octeon_pdcache_inv_all(void);
void octeon_pdcache_inv_range(register_t va, vsize_t size);
void octeon_pdcache_inv_range_index(vaddr_t va, vsize_t size);
void octeon_pdcache_wb_range(register_t va, vsize_t size);

#endif /* !_LOCORE */
