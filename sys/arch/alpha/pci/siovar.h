/* $NetBSD: siovar.h,v 1.11.22.1 2014/08/20 00:02:41 tls Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
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

void	sio_intr_setup(pci_chipset_tag_t, bus_space_tag_t);
void	sio_iointr(void *framep, unsigned long vec);

const char *sio_intr_string(void *, int, char *, size_t);
const struct evcnt *sio_intr_evcnt(void *, int);
void	*sio_intr_establish(void *, int, int, int, int (*)(void *),
	    void *);
void	sio_intr_disestablish(void *, void *);
int	sio_intr_alloc(void *, int, int, int *);
