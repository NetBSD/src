/*	$NetBSD: dc_cons.h,v 1.2 2000/01/08 01:02:35 simonb Exp $	*/

#ifdef _KERNEL
#ifndef _DC_CONS_H
#define _DC_CONS_H

/*
 * Following declaratios for console code.
 * XXX should be redesigned to expose less driver internals.
 */
void	dc_consinit __P((dev_t dev, dcregs *dcaddr));
int	dcGetc __P((dev_t dev));
int	dcparam __P((register struct tty *tp, register struct termios *t));
void	dcPutc __P((dev_t dev, int c));

#endif	/* _DCVAR_H */
#endif	/* _KERNEL */
