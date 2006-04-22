/* $NetBSD: aucomreg.h,v 1.1.44.1 2006/04/22 11:37:41 simonb Exp $ */

/* copyright */

#include <dev/ic/comreg.h>

#undef com_data		/* XXX ... */
#undef com_dlbl
#undef com_dlbh
#undef com_ier
#undef com_iir
#undef com_fifo
#undef com_lctl
#undef com_cfcr
#undef com_mcr
#undef com_lsr
#undef com_msr
#undef com_scratch

#undef	COM_FREQ	/* relative to CPU clock speed on Au1X00 */

/*
 * Alchemy Semi Au1X00 UART registers
 */

#define	com_rxdata	0x00	/* receive data register (R) */
#define	com_txdata	0x04	/* transmit data register (W) */
#define	com_ier		0x08	/* interrupt enable (R/W) */
#define	com_iir		0x0c	/* interrupt identification (R) */
#define	com_fifo	0x10	/* FIFO control (R/W) */
#define	com_lctl	0x14	/* line control register (R/W) */
#define	com_cfcr	0x14	/* line control register (R/W) */
#define	com_mcr		0x18	/* modem control register (R/W) */
#define	com_lsr		0x1c	/* line status register (R) */
#define	com_msr		0x20	/* modem status register (R) */
#define	com_dlb		0x28	/* divisor latch (16bit) (R/W) */
#define	com_modctl	0x100	/* module control register (R/W) */

#define	  UMC_CE	  0x2	/* module clock enable */
#define	  UMC_ME	  0x1	/* module enable */

/* XXX ISA-specific. */
#undef COM_NPORTS
#define	COM_NPORTS	0x104
