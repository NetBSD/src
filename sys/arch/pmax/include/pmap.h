/*	$NetBSD: pmap.h,v 1.12 1997/06/15 18:02:22 mhitch Exp $	*/

#include <mips/pmap.h>

#define pmax_trunc_seg(a) mips_trunc_seg(a)
#define pmax_round_seg(a) mips_round_seg(a)

#ifdef MIPS3
#define PMAP_PREFER(pa, va)             pmap_prefer((pa), (va))
void	pmap_prefer __P((vm_offset_t, vm_offset_t *));
#endif

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	firstaddr is the first unused kseg0 address (not page aligned).
 */
void	pmap_bootstrap __P((vm_offset_t firstaddr));
