/*	$NetBSD: cpu.h,v 1.16 1997/06/15 18:02:20 mhitch Exp $	*/

#include <mips/cpu.h>
#include <mips/cpuregs.h> /* XXX */

#ifdef MIPS3
#define	CLKF_USERMODE(framep)	CLKF_USERMODE_R4K(framep)
#define	CLKF_BASEPRI(framep)	CLKF_BASEPRI_R4K(framep)
#else
#define	CLKF_USERMODE(framep)	CLKF_USERMODE_R3K(framep)
#define	CLKF_BASEPRI(framep)	CLKF_BASEPRI_R3K(framep)
#endif


#ifdef _KERNEL
union	cpuprid cpu_id;
union	cpuprid fpu_id;
int	cpu_arch;
u_int	machDataCacheSize;
u_int	machInstCacheSize;
u_int	machPrimaryDataCacheSize;
u_int	machPrimaryInstCacheSize;
u_int	machPrimaryDataCacheLSize;
u_int	machPrimaryInstCacheLSize;
u_int	machCacheAliasMask;
u_int	machSecondaryCacheSize;
u_int	machSecondaryCacheLSize;
extern	struct intr_tab intr_tab[];
#endif
