/*	$NetBSD: cache_octeon.c,v 1.1.2.3 2016/10/05 20:55:32 skrll Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cache_octeon.c,v 1.1.2.3 2016/10/05 20:55:32 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <mips/locore.h>
#include <mips/cache.h>
#include <mips/cache_octeon.h>

#define	SYNC	__asm volatile("sync")

#ifdef OCTEON_ICACHE_DEBUG
int octeon_cache_debug;
#endif

static inline void
mips_synci(vaddr_t va)
{
	__asm __volatile("synci 0(%0)" :: "r"(va));
}

void
octeon_icache_sync_all(void)
{
#ifdef OCTEON_ICACHE_DEBUG
	if (__predict_false(octeon_cache_debug != 0))
		printf("%s\n", __func__);
#endif
	mips_synci(MIPS_KSEG0_START);
//	cache_octeon_invalidate(CACHEOP_OCTEON_INV_ALL | CACHE_OCTEON_I);
	SYNC;
}
void
octeon_icache_sync_range(register_t va, vsize_t size)
{
#ifdef OCTEON_ICACHE_DEBUG
	if (__predict_false(octeon_cache_debug != 0))
		printf("%s: va=%#"PRIxREGISTER", size=%#"PRIxVSIZE"\n",
		    __func__, va, size);
#endif
	mips_synci(MIPS_KSEG0_START);
//	cache_octeon_invalidate(CACHEOP_OCTEON_INV_ALL | CACHE_OCTEON_I);
	SYNC;
}

void
octeon_icache_sync_range_index(vaddr_t va, vsize_t size)
{
#ifdef OCTEON_ICACHE_DEBUG
	if (__predict_false(octeon_cache_debug != 0))
		printf("%s: va=%#"PRIxVADDR", size=%#"PRIxVSIZE"\n",
		    __func__, va, size);
#endif
	mips_synci(MIPS_KSEG0_START);
//	cache_octeon_invalidate(CACHEOP_OCTEON_INV_ALL | CACHE_OCTEON_I);
	SYNC;
}

void
octeon_pdcache_inv_all(void)
{
#ifdef OCTEON_ICACHE_DEBUG
	if (__predict_false(octeon_cache_debug != 0))
		printf("%s\n", __func__);
#endif
	cache_octeon_invalidate(CACHEOP_OCTEON_INV_ALL | CACHE_OCTEON_D);
	SYNC;
}

void
octeon_pdcache_inv_range(register_t va, vsize_t size)
{
#ifdef OCTEON_ICACHE_DEBUG
	if (__predict_false(octeon_cache_debug != 0))
		printf("%s: va=%#"PRIxREGISTER", size=%#"PRIxVSIZE"\n",
		    __func__, va, size);
#endif
	cache_octeon_invalidate(CACHEOP_OCTEON_INV_ALL | CACHE_OCTEON_D);
	SYNC;
}

void
octeon_pdcache_inv_range_index(vaddr_t va, vsize_t size)
{
#ifdef OCTEON_ICACHE_DEBUG
	if (__predict_false(octeon_cache_debug != 0))
		printf("%s: va=%#"PRIxVADDR", size=%#"PRIxVSIZE"\n",
		    __func__, va, size);
#endif
	cache_octeon_invalidate(CACHEOP_OCTEON_INV_ALL | CACHE_OCTEON_D);
	SYNC;
}

/* ---- debug dump */

#ifdef OCTEON_ICACHE_DEBUG

#ifndef __BIT
/* __BIT(n): nth bit, where __BIT(0) == 0x1. */
#define __BIT(__n)	\
	(((__n) >= NBBY * sizeof(uintmax_t)) ? 0 : ((uintmax_t)1 << (__n)))

/* __BITS(m, n): bits m through n, m < n. */
#define __BITS(__m, __n)	\
	((__BIT(MAX((__m), (__n)) + 1) - 1) ^ (__BIT(MIN((__m), (__n))) - 1))
#endif

/* icache: 16KB, 2ways */

#define	OCTEON_ICACHE_VA_WAY(_va)		(((_va) & __BITS(14, 13)) >> 13)
#define	OCTEON_ICACHE_VA_INDEX(_va)		(((_va) & __BITS(12,  7)) >>  7)
#define	OCTEON_ICACHE_VA_WORD(_va)		(((_va) & __BITS( 6,  3)) >>  3)

#define	OCTEON_ICACHE_TAGLO_R(_taglo)		(((_taglo) & __BITS(63, 62)) >> 62)
#define	OCTEON_ICACHE_TAGLO_ASID(_taglo)	(((_taglo) & __BITS(59, 52)) >> 52)
#define	OCTEON_ICACHE_TAGLO_TAG(_taglo)		(((_taglo) & __BITS(48, 13)) >> 13)
#define	OCTEON_ICACHE_TAGLO_INDEX(_taglo)	(((_taglo) & __BITS(12,  7)) >>  7)
#define	OCTEON_ICACHE_TAGLO_G(_taglo)		(((_taglo) & __BITS( 1,  1)) >>  1)
#define	OCTEON_ICACHE_TAGLO_VALID(_taglo)	(((_taglo) & __BITS( 0,  0)) >>  0)

/* dcache: 8KB, 64ways */

#define	OCTEON_DCACHE_VA_WAY(_va)		(((_va) & __BITS(12,  7)) >>  7)
#define	OCTEON_DCACHE_VA_WORD(_va)		(((_va) & __BITS( 6,  3)) >>  3)

#define	OCTEON_DCACHE_TAGLO_R(_taglo)		(((_taglo) & __BITS(63, 62)) >> 62)
#define	OCTEON_DCACHE_TAGLO_ASID(_taglo)	(((_taglo) & __BITS(59, 52)) >> 52)
#define	OCTEON_DCACHE_TAGLO_TAG(_taglo)		(((_taglo) & __BITS(48,  7)) >>  7)
#define	OCTEON_DCACHE_TAGLO_G(_taglo)		(((_taglo) & __BITS( 1,  1)) >>  1)
#define	OCTEON_DCACHE_TAGLO_VALID(_taglo)	(((_taglo) & __BITS( 0,  0)) >>  0)

#define	OCTEON_DCACHE_TAGHI_PTAG(_taghi)	(((_taghi) & __BITS(35,  7)) >>  7)
#define	OCTEON_DCACHE_TAGHI_VALID(_taghi)	(((_taghi) & __BITS( 0,  0)) >>  0)

void		octeon_icache_dump_all(void);
void		octeon_icache_dump_way(int);
void		octeon_icache_dump_index(int, int);
void		octeon_icache_dump_va(register_t);
void		octeon_dcache_dump_all(void);
void		octeon_dcache_dump_way(int);
void		octeon_dcache_dump_index(int, int);
void		octeon_dcache_dump_va(register_t);

void
octeon_icache_dump_all(void)
{
	/* way = (0 .. 3) */
	const int maxway = 2;		/* XXX 2 << (14 - 13 + 1) == 4? */
	int way;

	for (way = 0; way < maxway; way++)
		octeon_icache_dump_way(way);
}

void
octeon_icache_dump_way(int way)
{
	/* index = (0 .. 127) */
	const int maxindex = 64;	/* XXX 2 << (12 - 7 + 1) == 128? */;
	int index;

	for (index = 0; index < maxindex; index++)
		octeon_icache_dump_index(way, index);
}

void
octeon_icache_dump_index(int way, int index)
{
	const vaddr_t va = (way << 13) | (index << 7);

	octeon_icache_dump_va(va);
}

void
octeon_icache_dump_va(register_t va)
{
	uint64_t taglo, datalo, datahi;

	/* icache */
	__asm __volatile (
		"	.set	push			\n"
		"	.set	noreorder		\n"
		"	.set	arch=octeon		\n"
		"	cache	4, 0(%[va])		\n"
		"	dmfc0	%[taglo], $28, 0	\n"
		"	dmfc0	%[datalo], $28, 1	\n"
		"	dmfc0	%[datahi], $29, 1	\n"
		"	.set	pop			\n"
		: [taglo] "=r" (taglo),
		  [datalo] "=r" (datalo),
		  [datahi] "=r" (datahi)
		: [va] "r" (va));

	printf("%s: va=%08x "
	    "(way=%01x, index=%02x"/* ", word=%01d" */"), "
	    "taglo=%016"PRIx64" "
	    "(R=%01"PRIx64", asid=%02"PRIx64", tag=%09"PRIx64", index=%02"PRIx64", G=%01"PRIx64", valid=%01"PRIx64"), "
	    "datahi=%01"PRIx64", datalo=%016"PRIx64""
	    "\n",
	    __func__,
	    (int)((taglo) & __BITS(48, 7)),	/* (int)va */
	    (int)OCTEON_ICACHE_VA_WAY(va),
	    (int)OCTEON_ICACHE_VA_INDEX(va),
	    /* (int)OCTEON_ICACHE_VA_WORD(va), */
	    taglo,
	    OCTEON_ICACHE_TAGLO_R(taglo),
	    OCTEON_ICACHE_TAGLO_ASID(taglo),
	    OCTEON_ICACHE_TAGLO_TAG(taglo),
	    OCTEON_ICACHE_TAGLO_INDEX(taglo),
	    OCTEON_ICACHE_TAGLO_G(taglo),
	    OCTEON_ICACHE_TAGLO_VALID(taglo),
	    datahi, datalo);
}

void
octeon_dcache_dump_all(void)
{
	/* way = (0 .. 63) */
	const int maxway = 64;
	int way;

	for (way = 0; way < maxway; way++)
		octeon_dcache_dump_way(way);
}

void
octeon_dcache_dump_way(int way)
{
	/* index = (0 .. 0) */
	const int maxindex = 1;
	int index;

	for (index = 0; index < maxindex; index++)
		octeon_dcache_dump_index(way, index);
}

void
octeon_dcache_dump_index(int way, int index)
{
	const vaddr_t va = (way << 7);	/* no index in dcache */

	octeon_dcache_dump_va(va);
}

void
octeon_dcache_dump_va(register_t va)
{
	uint64_t taglo, taghi, datalo, datahi;

	/* dcache */
	__asm __volatile (
		"	.set	push			\n"
		"	.set	noreorder		\n"
		"	.set	arch=octeon		\n"
		"	cache	5, 0(%[va])		\n"
		"	dmfc0	%[taglo], $28, 2	\n"
		"	dmfc0	%[taghi], $29, 2	\n"
		"	dmfc0	%[datalo], $28, 3	\n"
		"	dmfc0	%[datahi], $29, 3	\n"
		"	.set	pop			\n"
		: [taglo] "=r" (taglo),
		  [taghi] "=r" (taghi),
		  [datalo] "=r" (datalo),
		  [datahi] "=r" (datahi)
		: [va] "r" (va));

	printf("%s: va=%08x "
	    "(way=%02x"/* ", word=%01d" */"), "
	    "taglo=%016"PRIx64" "
	    "(R=%01"PRIx64", asid=%02"PRIx64", tag=%09"PRIx64", G=%01"PRIx64", valid=%01"PRIx64"), "
	    "taghi=%016"PRIx64" "
	    "(ptag=%08"PRIx64", valid=%01"PRIx64"), "
	    "datahi=%02"PRIx64", datalo=%016"PRIx64""
	    "\n",
	    __func__,
	    (int)((taglo) & __BITS(48, 13)),	/* (int)va */
	    (int)OCTEON_DCACHE_VA_WAY(va),
	    /* (int)OCTEON_DCACHE_VA_WORD(va), */
	    taglo,
	    OCTEON_DCACHE_TAGLO_R(taglo),
	    OCTEON_DCACHE_TAGLO_ASID(taglo),
	    OCTEON_DCACHE_TAGLO_TAG(taglo),
	    OCTEON_DCACHE_TAGLO_G(taglo),
	    OCTEON_DCACHE_TAGLO_VALID(taglo),
	    taghi,
	    OCTEON_DCACHE_TAGHI_PTAG(taghi),
	    OCTEON_DCACHE_TAGHI_VALID(taghi),
	    datahi, datalo);
}

#endif	/* OCTEON_ICACHE_DEBUG */
