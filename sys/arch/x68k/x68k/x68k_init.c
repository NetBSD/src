/*	$NetBSD: x68k_init.c,v 1.1.1.1 1996/05/05 12:17:03 oki Exp $	*/

#include <x68k/x68k/iodevice.h>
#define zschan IODEVbase->io_inscc.zs_chan

volatile struct IODEVICE *IODEVbase = (volatile struct IODEVICE *) PHYS_IODEV;

/*
 * disable all interrupt.
 */
void
intr_reset()
{
	/* I/O Controller */
	ioctlr.intr = 0;

	/* Internal RS-232C port */
	zschan[1].zc_csr = 1;
	asm("nop");
	zschan[1].zc_csr = 0;
	asm("nop");

	/* mouse */
	zschan[0].zc_csr = 1;
	asm("nop");
	zschan[0].zc_csr = 0;
	asm("nop");
	while(!(mfp.tsr & MFP_TSR_BE))
		;
	mfp.udr = 0x41;

	/* MFP (hard coded interrupt vector XXX) */
	mfp.vr = 0x40;
}
