/*	$NetBSD: dcvar.h,v 1.4.24.1 2000/11/20 20:20:16 bouyer Exp $	*/

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

int	dcattach __P((struct dc_softc *sc, void *addr,
	    int dtrmask, int rts_ctsmask, int speed, int consline));

int	dcintr __P((void * xxxunit));

/*
 * Following declaratios for console code.
 * XXX should be redesigned to expose less driver internals.
 */
int	dcGetc __P((dev_t dev));
void	dcPutc __P((dev_t dev, int c));

/* QVSS-compatible in-kernel X input event parser, pointer tracker */
extern void	(*dcDivertXInput) __P((int));
extern void	(*dcMouseEvent) __P((void *));
extern void	(*dcMouseButtons) __P((void *));

void dc_cnattach __P((paddr_t, int));
void dckbd_cnattach __P((paddr_t));

#endif	/* _DCVAR_H */
#endif	/* _KERNEL */
