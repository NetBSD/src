/*	$NetBSD: cpu.h,v 1.14 1996/03/24 07:54:44 jonathan Exp $	*/

#include <mips/cpu.h>

#define	CLKF_USERMODE(framep)	CLKF_USERMODE_R3K(framep)
#define	CLKF_BASEPRI(framep)	CLKF_BASEPRI_R3K(framep)

#ifdef _KERNEL
union	cpuprid cpu;
union	cpuprid fpu;
u_int	machDataCacheSize;
u_int	machInstCacheSize;
extern	struct intr_tab intr_tab[];
#endif
