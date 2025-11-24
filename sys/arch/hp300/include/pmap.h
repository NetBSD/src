/*	$NetBSD: pmap.h,v 1.39 2025/11/24 16:36:14 thorpej Exp $	*/

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>

/*
 * The Hibler/Utah pmap put the virtual kernel PTE array near the top of
 * the address space because it needed to use the very last page of kernel
 * virtual space to map the last page of RAM VA==PA to faciliate MMU on/off
 * transitions, and doing so made for more efficient use of kernel PT pages.
 * We use the SYSMAP_VA hook to do the same for the same reason.
 *
 * (The *2 is because we can't use the very top of the address space
 * for the virtual kernel PTE array because MAXADDR is otherwise occupied.)
 */
#define	SYSMAP_VA	(0U - PAGE_SIZE*NPTEPG*2)

/*
 * Because we're reserving the last page of RAM for the MMU trampoline,
 * we might as well put it to good use as the NULL segment table.
 */
#define	NULL_SEGTAB_PA	MAXADDR

/*
 * Some hp300 systems have a virtually-addressed cache.  Enable
 * pmap_prefer() for cache alias avoidance on those machines.
 */
#define	PMAP_PREFER(h, vap, sz, td)	pmap_prefer((h), (vap), (td))
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#include <m68k/mmu_30.h>
#include <m68k/mmu_40.h>
#define __HAVE_MACHINE_BOOTMAP
