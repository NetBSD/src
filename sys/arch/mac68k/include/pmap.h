/*	$NetBSD: pmap.h,v 1.40 2025/11/30 21:42:28 thorpej Exp $	*/

#ifndef _MAC68K_PMAP_H_
#define	_MAC68K_PMAP_H_

#define	PMAP_MACHINE_CHECK_BOOTSTRAP_ALLOCATIONS
void	pmap_machine_check_bootstrap_allocations(paddr_t, paddr_t);

#include <m68k/pmap_motorola.h>
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
