/*	$NetBSD: dc_ds_cons.h,v 1.2 2000/01/08 01:02:35 simonb Exp $	*/

#ifdef _KERNEL
#ifndef _DC_DS_CONS_H
#define _DC_DS_CONS_H

/*
 * Following declaratios for console code.
 * XXX should be redesigned to expose less driver internals.
 */
int	dc_ds_consinit __P((dev_t dev));

#endif	/* _DC_DS_CONS_H */
#endif	/* _KERNEL */
