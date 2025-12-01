/*	$NetBSD: pmap.h,v 1.41 2025/12/01 17:50:17 thorpej Exp $	*/

#ifndef _MAC68K_PMAP_H_
#define	_MAC68K_PMAP_H_

#ifdef __HAVE_NEW_PMAP_68K
/*
 * On mac68k, pmap_bootstrap1() may be called with the MMU off
 * (and running VA==PA) *or* with the MMU on with foreign mappings
 * (this can be the case on Macs where the frame buffer is located
 * at $0000.0000 and the MMU is used by MacOS to simulate the
 * traditional MacOS memory map).
 *
 * In either case, we do not need any relocations to reference global
 * symbols.  However, in the latter case, we have to account for where
 * we've actually been loaded in order to access any real physical
 * addresses.
 *
 * To override these macros, define them before including <m68k/pmap_68k.h>.
 */
#define	PMAP_BOOTSTRAP_RELOC_GLOB(va)	((vaddr_t)(va))
#define	PMAP_BOOTSTRAP_RELOC_PA(pa)	(((vaddr_t)(pa)) - reloff)

#include <m68k/pmap_68k.h>

/*
 * We need to map the region between the start of kernel text and
 * start() read/write-write-though-cacheable, as this region contains
 * the vector table and the MacOS "low ram" with a bunch of variables
 * used by the ROMs.  We override the default kernel text protection
 * mode for pmap_bootstrap1() here, and then fix up the region between
 * start() and etext to be read-only once we're running on the kernel's
 * mappings.
 */
#define	PMAP_BOOTSTRAP_TEXT_PROTO_PTE	(PTE_VALID|PTE_WIRED)

#define	PMAP_MACHINE_CHECK_BOOTSTRAP_ALLOCATIONS
#else
#include <m68k/pmap_motorola.h>

paddr_t	pmap_bootstrap1(paddr_t, paddr_t);
#endif /* __HAVE_NEW_PMAP_68K */

void	pmap_machine_check_bootstrap_allocations(paddr_t, paddr_t);

#include <m68k/mmu_30.h>

/*
 * Transparent translation register used in locore.s:get_pte().
 * User Data set up for R/W access of the entire address space.
 *
 * (XXX TT30_RW isn't actually needed because of TT30_RWM, but
 * this the value historically used.)
 */
#define	MAC68K_TT_GET_PTE	(0x00000000 |				\
				 __SHIFTIN(0xff,TT30_LAM) |		\
				 TT30_E | TT30_CI | TT30_RW | TT30_RWM |\
				 TT30_USERD)

#endif /* _MAC68K_PMAP_H_ */
