/*	$NetBSD: cache_octeon.h,v 1.5 2020/07/26 08:08:41 simonb Exp $	*/

#ifndef _MIPS_CACHE_OCTEON_H_
#define	_MIPS_CACHE_OCTEON_H_

#define	CACHE_OCTEON_I			0
#define	CACHE_OCTEON_D			1

#define	CACHEOP_OCTEON_INV_ALL			(0 << 2)	/* I, D */
#define	CACHEOP_OCTEON_INDEX_LOAD_TAG		(1 << 2)	/* I, D */
#define	CACHEOP_OCTEON_BITMAP_STORE		(3 << 2)	/* I */
#define	CACHEOP_OCTEON_VIRTUAL_TAG_INV		(4 << 2)	/* D */

#define	OCTEON_CACHELINE_SIZE			128

/*
 * Note that for the Dcache the 50XX manual says 1 set per way (Config1
 * register - DS=0 ("... actual is 1"), p173) as does U-boot sources,
 * however this only adds up to an 8kB Dcache.  The 50XX manual
 * elsewhere references a 16kB Dcache as does the CN50XX product brief.
 * The original NetBSD code, current OpenBSD and Linux code all use 2
 * sets per way. lmbench's "cache" program also detects a 16kB Dcache.
 * So we assume that all Octeon 1 and Octeon Plus cores have a 16kB
 * Dcache.
 */
#define	OCTEON_I_DCACHE_WAYS			64
#define	OCTEON_I_DCACHE_SETS			2

#define	OCTEON_II_DCACHE_SETS			8
#define	OCTEON_II_DCACHE_WAYS			32
#define	OCTEON_II_ICACHE_SETS			8
#define	OCTEON_II_ICACHE_WAYS			37

#define	OCTEON_III_DCACHE_SETS			8
#define	OCTEON_III_DCACHE_WAYS			32
#define	OCTEON_III_ICACHE_SETS			16
#define	OCTEON_III_ICACHE_WAYS			39

#if !defined(_LOCORE)

/*
 * cache_octeon_invalidate:
 *
 *	Invalidate all cache blocks.
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
#endif /* _MIPS_CACHE_OCTEON_H_ */
