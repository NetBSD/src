/*	$NetBSD: dcvar.h,v 1.7 2000/01/08 01:02:35 simonb Exp $	*/

/*
 * External declarations from DECstation dc serial driver.
 */

#ifdef _KERNEL
#ifndef _DCVAR_H
#define _DCVAR_H

#include <pmax/dev/pdma.h>

struct dc_softc {
	struct device sc_dv;
	struct pdma dc_pdma[4];
	struct	tty *dc_tty[4];
	/*
	 * Software copy of brk register since it isn't readable
	 */
	int	dc_brk;

	int	dc_flags;

	char	dc_19200;		/* this unit supports 19200 */
	char	dcsoftCAR;		/* mask, lines with carrier on (DSR) */
	char	dc_rtscts;		/* mask, lines with hw flow control */
	char	dc_modem;		/* mask, lines with  DTR wired  */
};

/* flags */
#define DC_KBDMOUSE	0x01		/* keyboard and mouse attached */

int	dcattach __P((struct dc_softc *sc, void *addr, int dtrmask,
	    int rts_ctsmask, int speed, int consline));
int	dcintr __P((void * xxxunit));

/*
 * Following declaratios for console code.
 * XXX shuould be separated, or redesigned.
 */
int	dcGetc __P((dev_t dev));
int	dcparam __P((register struct tty *tp, register struct termios *t));
void	dcPutc __P((dev_t dev, int c));

/* QVSS-compatible in-kernel X input event parser, pointer tracker */
void	(*dcDivertXInput) __P((int cc)); /* X windows keyboard input routine */
void	(*dcMouseEvent) __P((int));	/* X windows mouse motion event routine */
void	(*dcMouseButtons) __P((int));	/* X windows mouse buttons event routine */

#endif	/* _DCVAR_H */
#endif	/* _KERNEL */
