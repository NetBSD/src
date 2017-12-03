/*	$NetBSD: spr.h,v 1.45.20.1 2017/12/03 11:36:37 jdolecek Exp $	*/

#ifndef _POWERPC_SPR_H_
#define	_POWERPC_SPR_H_

#ifndef _LOCORE
#ifdef PPC_OEA64_BRIDGE

static inline uint64_t
mfspr(int reg)
{
	uint64_t ret;
	register_t h, l;
	__asm volatile( "mfspr %0,%2;" \
			"srdi %1,%0,32;" \
			 : "=r"(l), "=r"(h) : "K"(reg));
	ret = ((uint64_t)h << 32) | l;
	return ret;
}

#define mtspr(reg, v) \
( {						\
	volatile register_t h, l;		\
	uint64_t val = v;			\
	h = (val >> 32);			\
	l = val & 0xffffffff;			\
	__asm volatile( \
			"sldi %2,%2,32;" \
			"or %2,%2,%1;" \
			"sync;" \
			"mtspr %0,%2;" \
			"mfspr %1,%0;" \
			"mfspr %1,%0;" \
			"mfspr %1,%0;" \
			"mfspr %1,%0;" \
			"mfspr %1,%0;" \
			"mfspr %1,%0;" \
			 : : "K"(reg), "r"(l), "r"(h)); \
} )

#else
#define	mtspr(reg, val)							\
	__asm volatile("mtspr %0,%1" : : "K"(reg), "r"(val))
#ifdef __GNUC__
#define	mfspr(reg)							\
	( { register_t val;						\
	  __asm volatile("mfspr %0,%1" : "=r"(val) : "K"(reg));	\
	  val; } )
#endif
#endif /* PPC_OEA64_BRIDGE */
#endif /* _LOCORE */

/*
 * Special Purpose Register declarations.
 *
 * The first column in the comments indicates which PowerPC architectures the
 * SPR is valid on - E for BookE series, 4 for 4xx series,
 * 6 for 6xx/7xx series and 8 for 8xx and 8xxx (but not 85xx) series.
 */

#define	SPR_XER			0x001	/* E468 Fixed Point Exception Register */
#define	SPR_LR			0x008	/* E468 Link Register */
#define	SPR_CTR			0x009	/* E468 Count Register */
#define	SPR_DEC			0x016	/* E468 DECrementer register */
#define	SPR_SRR0		0x01a	/* E468 Save/Restore Register 0 */
#define	SPR_SRR1		0x01b	/* E468 Save/Restore Register 1 */
#define	SPR_SPRG0		0x110	/* E468 SPR General 0 */
#define	SPR_SPRG1		0x111	/* E468 SPR General 1 */
#define	SPR_SPRG2		0x112	/* E468 SPR General 2 */
#define	SPR_SPRG3		0x113	/* E468 SPR General 3 */
#define	SPR_SPRG4		0x114	/* E4.. SPR General 4 */
#define	SPR_SPRG5		0x115	/* E4.. SPR General 5 */
#define	SPR_SPRG6		0x116	/* E4.. SPR General 6 */
#define	SPR_SPRG7		0x117	/* E4.. SPR General 7 */
#define	SPR_TBL			0x11c	/* E468 Time Base Lower */
#define	SPR_TBU			0x11d	/* E468 Time Base Upper */
#define	SPR_PVR			0x11f	/* E468 Processor Version Register */

/* Time Base Register declarations */
#define	TBR_TBL			0x10c	/* E468 Time Base Lower */
#define	TBR_TBU			0x10d	/* E468 Time Base Upper */

#endif /* !_POWERPC_SPR_H_ */
