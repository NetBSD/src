/*	$NetBSD: cpu.h,v 1.2 1996/03/24 07:54:42 jonathan Exp $	*/

#include <mips/cpu.h>

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	COPY_SIGCODE		/* copy sigcode above user stack in exec */

#define #define	CLKF_USERMODE(framep)	CLKF_USERMODE_R4K(framep)
#define	CLKF_BASEPRI(framep)	CLKF_BASEPRI_R4K(framep)

#ifdef _KERNEL
union	cpuprid cpu_id;
union	cpuprid fpu_id;
u_int	machPrimaryDataCacheSize;
u_int	machPrimaryInstCacheSize;
u_int	machPrimaryDataCacheLSize;
u_int	machPrimaryInstCacheLSize;
u_int	machCacheAliasMask;
extern	struct intr_tab intr_tab[];
#endif
