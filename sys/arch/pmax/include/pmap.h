/*	$NetBSD: pmap.h,v 1.11 1997/05/25 05:04:03 jonathan Exp $	*/

#include <mips/pmap.h>

#define pmax_trunc_seg(a) mips_trunc_seg(a)
#define pmax_round_seg(a) mips_round_seg(a)

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	firstaddr is the first unused kseg0 address (not page aligned).
 */
void	pmap_bootstrap __P((vm_offset_t firstaddr));
