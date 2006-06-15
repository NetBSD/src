/* $NetBSD: com_aubus_reg.h,v 1.1.2.1 2006/06/15 16:40:43 gdamore Exp $ */

/* copyright */

#undef	COM_FREQ	/* relative to CPU clock speed on Au1X00 */

/*
 * Alchemy Semi Au1X00 UART registers
 */

#define	AUCOM_RXDATA	0x00	/* receive data register (R) */
#define	AUCOM_TXDATA	0x04	/* transmit data register (W) */
#define	AUCOM_IER	0x08	/* interrupt enable (R/W) */
#define	AUCOM_IIR	0x0c	/* interrupt identification (R) */
#define	AUCOM_FIFO	0x10	/* FIFO control (R/W) */
#define	AUCOM_LCTL	0x14	/* line control register (R/W) */
#define	AUCOM_CFCR	0x14	/* line control register (R/W) */
#define	AUCOM_MCR	0x18	/* modem control register (R/W) */
#define	AUCOM_LSR	0x1c	/* line status register (R) */
#define	AUCOM_MSR	0x20	/* modem status register (R) */
#define	AUCOM_DLB	0x28	/* divisor latch (16bit) (R/W) */
#define	AUCOM_MODCTL	0x100	/* module control register (R/W) */

#define	  UMC_CE	  0x2	/* module clock enable */
#define	  UMC_ME	  0x1	/* module enable */

#define	AUCOM_NPORTS	0x104
