/*	$NetBSD: dc7085reg.h,v 1.1.2.1 1998/10/21 11:24:33 nisimura Exp $ */

/*
 * DC7085 gate array, DZ-11 clone, which provides quad asynchronous serial
 * interface
 */

/* Control status register definitions (dccsr) */
#define DC_OFF		0x00		/* Modem control off		*/
#define DC_MAINT	0x08		/* Maintenance			*/
#define DC_CLR		0x10		/* Reset dc7085 chip		*/
#define DC_MSE		0x20		/* Master Scan Enable		*/
#define DC_RIE		0x40		/* Receive IE */
#define DC_RDONE	0x80		/* Receiver done		*/
#define DC_TIE		0x4000		/* Trasmit IE */
#define DC_TRDY		0x8000		/* Transmit ready		*/

/* Line parameter register definitions (dclpr) */
#define DC_BITS5	0x00		/* 5 bit char width		*/
#define DC_BITS6	0x08		/* 6 bit char width		*/
#define DC_BITS7	0x10		/* 7 bit char width		*/
#define DC_BITS8	0x18		/* 8 bit char width		*/
#define DC_TWOSB	0x20		/* two stop bits		*/
#define DC_PENABLE	0x40		/* parity enable		*/
#define DC_OPAR		0x80		/* odd parity			*/
#define DC_B50		0x000		/* 50 BPS speed			*/
#define DC_B75		0x100		/* 75 BPS speed			*/
#define DC_B110		0x200		/* 110 BPS speed		*/
#define DC_B134_5	0x300		/* 134.5 BPS speed		*/
#define DC_B150		0x400		/* 150 BPS speed		*/
#define DC_B300		0x500		/* 300 BPS speed		*/
#define DC_B600		0x600		/* 600 BPS speed		*/
#define DC_B1200	0x700		/* 1200 BPS speed		*/
#define DC_B1800	0x800		/* 1800 BPS speed		*/
#define DC_B2000	0x900		/* 2000 BPS speed		*/
#define DC_B2400	0xa00		/* 2400 BPS speed		*/
#define DC_B3600	0xb00		/* 3600 BPS speed		*/
#define DC_B4800	0xc00		/* 4800 BPS speed		*/
#define DC_B7200	0xd00		/* 7200 BPS speed		*/
#define DC_B9600	0xe00		/* 9600 BPS speed		*/
#define DC_B19200	0xf00		/* 19200 BPS speed		*/
#define DC_B38400	0xf00		/* 38400 BPS speed - see LED2	*/
#define DC_RE		0x1000		/* Receive enable		*/

/* Transmit Control Register (dctcr) */
#define DC_TCR_EN_0	0x1		/* enable transmit on line 0	*/
#define DC_TCR_EN_1	0x2		/* enable transmit on line 1	*/
#define DC_TCR_EN_2	0x4		/* enable transmit on line 2	*/
#define DC_TCR_EN_3	0x8		/* enable transmit on line 3	*/

/* CPU specific Transmit Control Register definitions */
#define DS3100_DC_L2_DTR	0x0400	/* pmax DTR on line 2		*/
#define DS3100_DC_L2_DSR	0x0200	/* pmax DSR on line 2		*/
#define DS3100_DC_L2_XMIT	DS3100_DC_L2_DSR  /* All modem leads ready */

#define DS3100_DC_L3_DTR	0x0800	/* pmax DTR on line 3		*/
#define DS3100_DC_L3_DSR	0x0001	/* pmax DSR on line 3		*/
#define DS3100_DC_L3_XMIT	DS3100_DC_L3_DSR  /* All modem leads ready */

#define DS5000_DC_L2_DTR	0x0400	/* 3max DTR on line 2		*/

#define DS5000_DC_L3_DTR	0x0100	/* 3max DTR on line 3		*/
#define DS5000_DC_L3_RTS	0x0200	/* 3max RTS on line 3		*/

#define DS5100_DC_L2_SS		0x0200	/* mipsmate SS on line 2	*/
#define DS5100_DC_L2_DTR	0x0400	/* mipsmate DTR on line 2	*/
#define DS5100_DC_L2_RTS	0x0800	/* mipsmate RTS on line 2	*/

/* CPU specific Modem Status Register definitions */
#define DS3100_DC_L2_SS		0x0200	/* mipsmate ss on line 2	*/

#define DS5000_DC_L2_CTS	0x0100	/* 3max CTS on line 2		*/
#define DS5000_DC_L2_DSR	0x0200	/* 3max DSR on line 2		*/
#define DS5000_DC_L2_CD		0x0400	/* 3max CD on line 2		*/
#define DS5000_DC_L2_RI		0x0800	/* 3max RI on line 2		*/
#define DS5000_DC_L2_XMIT	(DS5000_DC_L2_CTS| DS5000_DC_L2_DSR| DS5000_DC_L2_CD)					/* All modem leads ready	*/

#define DS5000_DC_L3_CTS	0x0001	/* 3max CTS on line 3		*/
#define DS5000_DC_L3_DSR	0x0002	/* 3max DSR on line 3		*/
#define DS5000_DC_L3_CD		0x0004	/* 3max CD on line 3		*/
#define DS5000_DC_L3_RI		0x0008	/* 3max RI on line 3		*/
#define DS5000_DC_L3_XMIT	(DS5000_DC_L3_CTS| DS5000_DC_L3_DSR| DS5000_DC_L3_CD)					/* All modem leads ready	*/

#define DS5100_DC_L2_CTS	0x0001	/* mipsmate CTS on line 2	*/
#define DS5100_DC_L2_DSR	0x0002	/* mipsmate DSR on line 2	*/
#define DS5100_DC_L2_CD		0x0004	/* mipsmate CD on line 2	*/
#define DS5100_DC_L2_RI		0x0008	/* mipsmate CD on line 2	*/
#define DS5100_DC_L2_SI		0x0080	/* mipsmate CD on line 2	*/
#define DS5100_DC_L2_XMIT	(DS5100_DC_L2_CTS| DS5100_DC_L2_DSR| DS5100_DC_L2_CD)					/* All modem leads ready	*/

/* Receiver buffer register definitions (dcrbuf) */
#define DC_PE		0x1000		/* Parity error			*/
#define DC_FE		0x2000		/* Framing error		*/
#define DC_DO		0x4000		/* Data overrun error		*/
#define DC_DVAL		0x8000		/* Receive buffer data valid	*/

/* Line control status definitions (dclcs) */
#define DC_SR		0x08		/* Secondary Receive		*/
#define DC_CTS		0x10		/* Clear To Send		*/
#define DC_CD		0x20		/* Carrier Detect		*/
#define DC_RI		0x40		/* Ring Indicate		*/
#define DC_DSR		0x80		/* Data Set Ready		*/
#define DC_LE		0x100		/* Line Enable			*/
#define DC_DTR		0x200		/* Data Terminal Ready		*/
#define DC_BRK		0x400		/* Break			*/
#define DC_ST		0x800		/* Secondary Transmit		*/
#define DC_RTS		0x1000		/* Request To Send		*/

/* DM lsr definitions */
#define SML_LE		0x01		/* Line enable			*/
#define SML_DTR		0x02		/* Data terminal ready		*/
#define SML_RTS		0x04		/* Request to send		*/
#define SML_ST		0x08		/* Secondary transmit		*/
#define SML_SR		0x10		/* Secondary receive		*/
#define SML_CTS		0x20		/* Clear to send		*/
#define SML_CAR		0x40		/* Carrier detect		*/
#define SML_RNG		0x80		/* Ring				*/
#define SML_DSR		0x100		/* Data set ready, not DM bit	*/

/* Line Prameter Register bits */
#define SER_KBD		000000
#define SER_POINTER	000001
#define SER_COMLINE	000002
#define SER_PRINTER	000003
#define SER_CHARW	000030
#define SER_STOP	000040
#define SER_PARENB	000100
#define SER_ODDPAR	000200
#define SER_SPEED	006000
#define SER_RXENAB	010000
