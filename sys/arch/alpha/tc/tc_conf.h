/* $NetBSD: tc_conf.h,v 1.12.2.1 2012/04/17 00:05:58 yamt Exp $ */

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Machine-specific TurboChannel configuration definitions.
 */

#include "opt_dec_3000_300.h"
#include "opt_dec_3000_500.h"

#ifdef DEC_3000_500
#include <alpha/tc/tc_dma_3000_500.h>

extern void	tc_3000_500_intr_setup(void);
extern void	tc_3000_500_iointr(void *, unsigned long);

extern const struct evcnt *
		tc_3000_500_intr_evcnt(device_t, void *);
extern void	tc_3000_500_intr_establish(device_t, void *,
		    tc_intrlevel_t, int (*)(void *), void *);
extern void	tc_3000_500_intr_disestablish(device_t, void *);

extern int	tc_3000_500_nslots;
extern struct tc_slotdesc tc_3000_500_slots[];
extern int	tc_3000_500_graphics_nbuiltins;
extern struct tc_builtin tc_3000_500_graphics_builtins[];
extern int	tc_3000_500_nographics_nbuiltins;
extern struct tc_builtin tc_3000_500_nographics_builtins[];
#endif /* DEC_3000_500 */

#ifdef DEC_3000_300
#include <alpha/tc/tc_dma_3000_300.h>

extern void	tc_3000_300_intr_setup(void);
extern void	tc_3000_300_iointr(void *, unsigned long);

extern const struct evcnt *
		tc_3000_300_intr_evcnt(device_t, void *);
extern void	tc_3000_300_intr_establish(device_t, void *,
		    tc_intrlevel_t, int (*)(void *), void *);
extern void	tc_3000_300_intr_disestablish(device_t, void *);

extern int	tc_3000_300_nslots;
extern struct tc_slotdesc tc_3000_300_slots[];
extern int	tc_3000_300_nbuiltins;
extern struct tc_builtin tc_3000_300_builtins[];
#endif /* DEC_3000_300 */

extern int	tc_fb_cnattach(tc_addr_t);

