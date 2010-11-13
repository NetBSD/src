/* $NetBSD: imxuartreg.h,v 1.4 2010/11/13 06:12:17 bsh Exp $ */
/*
 * register definitions for Freescale i.MX31 and i.MX31L UARTs
 *
 * UART specification obtained from:
 *	MCIMX31 and MCIMX31L Application Processors
 *	Reference Manual
 *	MCIMC31RM
 *	Rev. 2.3
 *	1/2007
 *
 *	MCIMC51 Multimedia Application Processor
 *	Reference Manual
 *      MCMIMX51RM
 *	Rev.1
 *	2/2010
 */
#ifndef	_IMXUARTREG_H
#define	_IMXUARTREG_H

#ifdef	__ASSEMBLER__
#define	__BIT(n)	(1<<(n))
#define	__BITS(hi,lo)   ((~((~0)<<((hi)+1)))&((~0)<<(lo)))
#else
#include <sys/cdefs.h>
#endif

/*
 * Registers are 32 bits wide; the 16 MSBs are unused --
 * they read as zeros and are ignored on write.
 */


/*
 * register offsets
 */
#define	IMX_URXD	0x00	/* r  */	/* UART Receiver Reg */
#define	IMX_UTXD	0x40	/* w  */	/* UART Transmitter Reg */
#define	IMX_UCR1	0x80	/* rw */	/* UART Control Reg 1 */
#define	IMX_UCR2	0x84	/* rw */	/* UART Control Reg 2 */
#define	IMX_UCR3	0x88	/* rw */	/* UART Control Reg 3 */
#define	IMX_UCR4	0x8c	/* rw */	/* UART Control Reg 4 */
#define	IMX_UCRn(n)	(IMX_UCR1 + ((n) << 2))
#define	IMX_UFCR	0x90	/* rw */	/* UART FIFO Control Reg */
#define	IMX_USR1	0x94	/* rw */	/* UART Status Reg 1 */
#define	IMX_USR2	0x98	/* rw */	/* UART Status Reg 2 */
#define	IMX_USRn(n)	(IMX_USR1 + ((n) << 2))
#define	IMX_UESC	0x9c	/* rw */	/* UART Escape Character Reg */
#define	IMX_UTIM	0xa0	/* rw */	/* UART Escape Timer Reg */
#define	IMX_UBIR	0xa4	/* rw */	/* UART BRM Incremental Reg */
#define	IMX_UBMR	0xa8	/* rw */	/* UART BRM Modulator Reg */
#define	IMX_UBRC	0xac	/* r  */	/* UART Baud Rate Count Reg */
#define	IMX_ONEMS	0xb0	/* rw */	/* UART One Millisecond Reg */
#define	IMX_UTS		0xb4	/* rw */	/* UART Test Reg */

#define	IMX_UART_SIZE	0xb8

/*
 * bit attributes:
 * 	ro	read-only
 * 	wo	write-only
 *	rw	read/write
 * 	w1c	write 1 to clear
 *
 * attrs defined but apparently unused for the UART:
 *	rwm	rw bit that can be modified by HW (other than reset)
 *	scb	self-clear: write 1 has some effect, always reads as 0
 */

/*
 * IMX_URXD bits
 */
#define	IMX_URXD_RX_DATA	__BITS(7,0)	/* ro */
#define	IMX_URXD_RESV		__BITS(9,8)	/* ro */
#define	IMX_URXD_PRERR		__BIT(10)		/* ro */
#define	IMX_URXD_BRK		__BIT(11)		/* ro */
#define	IMX_URXD_FRMERR		__BIT(12)		/* ro */
#define	IMX_URXD_OVRRUN		__BIT(13)		/* ro */
#define	IMX_URXD_ERR		__BIT(14)		/* ro */
#define	IMX_URXD_CHARDY		__BIT(15)		/* ro */

/*
 * IMX_UTXD bits
 */
#define	IMX_UTXD_TX_DATA	__BITS(7,0)	/* wo */
#define	IMX_UTXD_RESV		__BITS(15,8)

/*
 * IMX_UCR1 bits
 */
#define	IMX_UCR1_UARTEN		__BIT(0)		/* rw */
#define	IMX_UCR1_DOZE		__BIT(1)		/* rw */
#define	IMX_UCR1_ATDMAEN	__BIT(2)		/* rw */
#define	IMX_UCR1_TXDMAEN	__BIT(3)		/* rw */
#define	IMX_UCR1_SNDBRK		__BIT(4)		/* rw */
#define	IMX_UCR1_RTSDEN		__BIT(5)		/* rw */
#define	IMX_UCR1_TXMPTYEN	__BIT(6)		/* rw */
#define	IMX_UCR1_IREN		__BIT(7)		/* rw */
#define	IMX_UCR1_RXDMAEN	__BIT(8)		/* rw */
#define	IMX_UCR1_RRDYEN		__BIT(9)		/* rw */
#define	IMX_UCR1_ICD		__BITS(11,10)	/* rw */
#define	IMX_UCR1_IDEN		__BIT(12)		/* rw */
#define	IMX_UCR1_TRDYEN		__BIT(13)		/* rw */
#define	IMX_UCR1_ADBR		__BIT(14)		/* rw */
#define	IMX_UCR1_ADEN		__BIT(15)		/* rw */

/*
 * IMX_UCR2 bits
 */
#define	IMX_UCR2_SRST		__BIT(0)		/* rw */
#define	IMX_UCR2_RXEN		__BIT(1)		/* rw */
#define	IMX_UCR2_TXEN		__BIT(2)		/* rw */
#define	IMX_UCR2_ATEN		__BIT(3)		/* rw */
#define	IMX_UCR2_RTSEN		__BIT(4)		/* rw */
#define	IMX_UCR2_WS		__BIT(5)		/* rw */
#define	IMX_UCR2_STPB		__BIT(6)		/* rw */
#define	IMX_UCR2_PROE		__BIT(7)		/* rw */
#define	IMX_UCR2_PREN		__BIT(8)		/* rw */
#define	IMX_UCR2_RTEC		__BITS(10,9)	/* rw */
#define	IMX_UCR2_ESCEN		__BIT(11)		/* rw */
#define	IMX_UCR2_CTS		__BIT(12)		/* rw */
#define	IMX_UCR2_CTSC		__BIT(13)		/* rw */
#define	IMX_UCR2_IRTS		__BIT(14)		/* rw */
#define	IMX_UCR2_ESCI		__BIT(15)		/* rw */

/*
 * IMX_UCR3 bits
 */
#define	IMX_UCR3_ACIEN		__BIT(0)		/* rw */
#define	IMX_UCR3_INVT		__BIT(1)		/* rw */
#define	IMX_UCR3_RXDMUXSEL	__BIT(2)		/* rw */
#define	IMX_UCR3_DTRDEN		__BIT(3)		/* rw */
#define	IMX_UCR3_AWAKEN		__BIT(4)		/* rw */
#define	IMX_UCR3_AIRINTEN	__BIT(5)		/* rw */
#define	IMX_UCR3_RXDSEN		__BIT(6)		/* rw */
#define	IMX_UCR3_ADNIMP		__BIT(7)		/* rw */
#define	IMX_UCR3_RI		__BIT(8)		/* rw */
#define	IMX_UCR3_DCD		__BIT(9)		/* rw */
#define	IMX_UCR3_DSR		__BIT(10)		/* rw */
#define	IMX_UCR3_FRAERREN	__BIT(11)		/* rw */
#define	IMX_UCR3_PARERREN	__BIT(12)		/* rw */
#define	IMX_UCR3_DTREN		__BIT(13)		/* rw */
#define	IMX_UCR3_DPEC		__BITS(15,14)	/* rw */

/*
 * IMX_UCR4 bits
 */
#define	IMX_UCR4_DREN		__BIT(0)		/* rw */
#define	IMX_UCR4_OREN		__BIT(1)		/* rw */
#define	IMX_UCR4_BKEN		__BIT(2)		/* rw */
#define	IMX_UCR4_TCEN		__BIT(3)		/* rw */
#define	IMX_UCR4_LPBYP		__BIT(4)		/* rw */
#define	IMX_UCR4_IRSC		__BIT(5)		/* rw */
#define	IMX_UCR4_IDDMAEN	__BIT(6)		/* rw */
#define	IMX_UCR4_WKEN		__BIT(7)		/* rw */
#define	IMX_UCR4_ENIRI		__BIT(8)		/* rw */
#define	IMX_UCR4_INVR		__BIT(9)		/* rw */
#define	IMX_UCR4_CTSTL		__BITS(15,10)	/* rw */

/*
 * IMX_UFCR bits
 */
#define	IMX_UFCR_RXTL_SHIFT	0
#define	IMX_UFCR_RXTL		__BITS(5,IMX_UFCR_RXTL_SHIFT)	/* rw */
#define	IMX_UFCR_DCEDTE		__BIT(6)		/* rw */
#define	IMX_UFCR_RFDIV_SHIFT	7
#define	IMX_UFCR_RFDIV		__BITS(9,IMX_UFCR_RFDIV_SHIFT)	/* rw */
#define	IMX_UFCR_TXTL_SHIFT	10
#define	IMX_UFCR_TXTL		__BITS(15,IMX_UFCR_TXTL_SHIFT)	/* rw */

#define	IMX_UFCR_DIVIDER_TO_RFDIV(div)	\
	(((6 - div) % 7) & 0x7)

/*
 * IMX_USR1 bits
 */
#define	IMX_USR1_RESV		__BITS(3,0)
#define	IMX_USR1_AWAKE		__BIT(4)		/* w1c */
#define	IMX_USR1_AIRINT		__BIT(5)		/* w1c */
#define	IMX_USR1_RXDS		__BIT(6)		/* ro  */
#define	IMX_USR1_DTRD		__BIT(7)		/* w1c */
#define	IMX_USR1_AGTIM		__BIT(8)		/* w1c */
#define	IMX_USR1_RRDY		__BIT(9)		/* ro  */
#define	IMX_USR1_FRAMERR	__BIT(10)		/* w1c */
#define	IMX_USR1_ESCF		__BIT(11)		/* w1c */
#define	IMX_USR1_RTSD		__BIT(12)		/* w1c */
#define	IMX_USR1_TRDY		__BIT(13)		/* ro  */
#define	IMX_USR1_RTSS		__BIT(14)		/* ro  */
#define	IMX_USR1_PARITYERR	__BIT(15)		/* w1c */

/*
 * IMX_USR2 bits
 */
#define	IMX_USR2_RDR		__BIT(0)
#define	IMX_USR2_ORE		__BIT(1)		/* w1c */
#define	IMX_USR2_BRCD		__BIT(2)		/* w1c */
#define	IMX_USR2_TXDC		__BIT(3)		/* ro  */
#define	IMX_USR2_RTSF		__BIT(4)		/* w1c */
#define	IMX_USR2_DCDIN		__BIT(5)		/* ro  */
#define	IMX_USR2_DCDDELT	__BIT(6)		/* rw  */
#define	IMX_USR2_WAKE		__BIT(7)		/* w1c */
#define	IMX_USR2_IRINT		__BIT(8)		/* rw  */
#define	IMX_USR2_RIIN		__BIT(9)		/* ro  */
#define	IMX_USR2_RIDELT		__BIT(10)		/* w1c */
#define	IMX_USR2_ACST		__BIT(11)		/* rw  */
#define	IMX_USR2_IDLE		__BIT(12)		/* w1c */
#define	IMX_USR2_DTRF		__BIT(13)		/* rw  */
#define	IMX_USR2_TXFE		__BIT(14)		/* ro  */
#define	IMX_USR2_ADET		__BIT(15)		/* w1c */

/*
 * IMX_UESC bits
 */
#define	IMX_UESC_ESC_CHAR	__BITS(7,0)	/* rw */
#define	IMX_UESC_RESV		__BITS(15,8)

/*
 * IMX_UTIM bits
 */
#define	IMX_UTIM_TIM		__BITS(11,0)	/* rw */
#define	IMX_UTIM_RESV		__BITS(15,12)

/*
 * IMX_UBIR bits
 */
#define	IMX_UBIR_INC		__BITS(15,0)	/* rw */

/*
 * IMX_UBMR bits
 */
#define	IMX_UBMR_MOD		__BITS(15,0)	/* rw */

/*
 * IMX_UBRC bits
 */
#define	IMX_UBRC_BCNT		__BITS(15,0)	/* ro */

/*
 * IMX_ONEMS bits
 */
#define	IMX_ONEMS_ONEMS		__BITS(15,0)	/* rw */

/*
 * IMX_UTS bits
 */
#define	IMX_UTS_SOFTRST		__BIT(0)		/* rw */
#define	IMX_UTS_RESVa		__BITS(2,1)
#define	IMX_UTS_RXFULL		__BIT(3)		/* rw */
#define	IMX_UTS_TXFUL		__BIT(4)		/* rw */
#define	IMX_UTS_RXEMPTY		__BIT(5)		/* rw */
#define	IMX_UTS_TXEMPTY		__BIT(5)		/* rw */
#define	IMX_UTS_RESVb		__BITS(8,7)
#define	IMX_UTS_RXDBG		__BIT(9)		/* rw */
#define	IMX_UTS_LOOPIR		__BIT(10)		/* rw */
#define	IMX_UTS_DBGEN		__BIT(11)		/* rw */
#define	IMX_UTS_LOOP		__BIT(12)		/* rw */
#define	IMX_UTS_FRCPERR		__BIT(13)		/* rw */
#define	IMX_UTS_RESVc		__BITS(15,14)
#define	IMX_UTS_RESV		(IMX_UTS_RESVa|IMX_UTS_RESVb|IMX_UTS_RESVc)

#ifndef	__ASSEMBLER__
/*
 * interrupt specs
 * see Table 31-25. "Interrupts an DMA"
 */

/*
 * abstract interrupts spec indexing
 */
typedef enum {
	RX_RRDY=0,
	RX_ID,
	RX_DR,
	RX_RXDS,
	RX_AT,
	TX_TXMPTY,
	TX_TRDY,
	TX_TC,
	MINT_OR,
	MINT_BR,
	MINT_WK,
	MINT_AD,
	MINT_ACI,
	MINT_ESCI,
	MINT_IRI,
	MINT_AIRINT,
	MINT_AWAK,
	MINT_FRAERR,
	MINT_PARERR,
	MINT_RTSD,
	MINT_RTS,
	MINT_DCE_DTR,
	MINT_DTE_RI,
	MINT_DTE_DCE,
	MINT_DTRD,
	RX_DMAREQ_RXDMA,
	RX_DMAREQ_ATDMA,
	RX_DMAREQ_IDDMA,
	RX_DMAREQ_TXDMA,
} imxuart_intrix_t;

/*
 * abstract interrupts spec
 */
typedef struct {
	const uint32_t	enb_bit;
	const uint	enb_reg;
	const uint32_t	flg_bit;
	const uint	flg_reg;
	const char *	name;		/* for debug */
} imxuart_intrspec_t;

#define	IMXUART_INTRSPEC(cv, cr, sv, sr) \
	{ IMX_UCR##cr##_##cv, ((cr) - 1), IMX_USR##sr##_##sv, ((sr) - 1), #sv }

static const imxuart_intrspec_t imxuart_intrspec_tab[] = {
/* ipi_uart_rx */
	IMXUART_INTRSPEC(RRDYEN,   1, RRDY,      1),
	IMXUART_INTRSPEC(IDEN,     1, IDLE,      2),
	IMXUART_INTRSPEC(DREN,     4, RDR,       2),
	IMXUART_INTRSPEC(RXDSEN,   3, RXDS,      1),
	IMXUART_INTRSPEC(ATEN,     2, AGTIM,     1),
/* ipi_uart_tx */
	IMXUART_INTRSPEC(TXMPTYEN, 1, TXFE,      2),
	IMXUART_INTRSPEC(TRDYEN,   1, TRDY,      1),
	IMXUART_INTRSPEC(TCEN,     4, TXDC,      2),
/* ipi_uart_mint */
	IMXUART_INTRSPEC(OREN,     4, ORE,       2),
	IMXUART_INTRSPEC(BKEN,     4, BRCD,      2),
	IMXUART_INTRSPEC(WKEN,     4, WAKE,      2),
	IMXUART_INTRSPEC(ADEN,     1, ADET,      2),
	IMXUART_INTRSPEC(ACIEN,    3, ACST,      2),
	IMXUART_INTRSPEC(ESCI,     2, ESCF,      1),
	IMXUART_INTRSPEC(ENIRI,    4, IRINT,     2),
	IMXUART_INTRSPEC(AIRINTEN, 3, AIRINT,    1),
	IMXUART_INTRSPEC(AWAKEN,   3, AWAKE,     1),
	IMXUART_INTRSPEC(FRAERREN, 3, FRAMERR,   1),
	IMXUART_INTRSPEC(PARERREN, 3, PARITYERR, 1),
	IMXUART_INTRSPEC(RTSDEN,   1, RTSD,      1),
	IMXUART_INTRSPEC(RTSEN,    2, RTSF,      2),
	IMXUART_INTRSPEC(DTREN,    3, DTRF,      2),
	IMXUART_INTRSPEC(RI,       3, DTRF,      2),
	IMXUART_INTRSPEC(DCD,      3, DCDDELT,   2),
	IMXUART_INTRSPEC(DTRDEN,   3, DTRD,      1),
/* ipd_uart_rx_dmareq */
	IMXUART_INTRSPEC(RXDMAEN,  1, RRDY,      1),
	IMXUART_INTRSPEC(ATDMAEN,  1, AGTIM,     1),
	IMXUART_INTRSPEC(IDDMAEN,  4, IDLE,      2),
/* ipd_uart_tx_dmareq */
	IMXUART_INTRSPEC(TXDMAEN,  1, TRDY,      1),
};
#define	IMXUART_INTRSPEC_TAB_SZ	\
	(sizeof(imxuart_intrspec_tab) / sizeof(imxuart_intrspec_tab[0]))

#endif	/* __ASSEMBLER__ */

/*
 * functional groupings of intr status flags by reg
 */
#define	IMXUART_RXINTR_USR1	(IMX_USR1_RRDY|IMX_USR1_RXDS|IMX_USR1_AGTIM)
#define	IMXUART_RXINTR_USR2	(IMX_USR2_IDLE|IMX_USR2_RDR)
#define	IMXUART_TXINTR_USR1	(IMX_USR1_TRDY)
#define	IMXUART_TXINTR_USR2	(IMX_USR2_TXFE|IMX_USR2_TXDC)
#define	IMXUART_MINT_USR1	(IMX_USR1_ESCF|IMX_USR1_AIRINT||IMX_USR1_AWAKE \
				|IMX_USR1_FRAMERR|IMX_USR1_PARITYERR \
				|IMX_USR1_RTSD|IMX_USR1_DTRD)
#define	IMXUART_MINT_USR2	(IMX_USR2_ORE|IMX_USR2_BRCD|IMX_USR2_WAKE \
				|IMX_USR2_ADET|IMX_USR2_ACST|IMX_USR2_IRINT \
				|IMX_USR2_RTSF|IMX_USR2_DTRF|IMX_USR2_DTRF \
				|IMX_USR2_DCDDELT)
#define	IMXUART_RXDMA_USR1	(IMX_USR1_RRDY|IMX_USR1_AGTIM)
#define	IMXUART_RXDMA_USR2	(IMX_USR2_IDLE)
#define	IMXUART_TXDMA_USR1	(IMX_USR1_TRDY)
#define	IMXUART_TXDMA_USR2	(0)

/*
 * all intr status flags by reg
 */
#define	IMXUART_INTRS_USR1	(IMXUART_RXINTR_USR1|IMXUART_TXINTR_USR1 \
				|IMXUART_MINT_USR1|IMXUART_RXDMA_USR1 \
				|IMXUART_TXDMA_USR1)
#define	IMXUART_INTRS_USR2	(IMXUART_RXINTR_USR2|IMXUART_TXINTR_USR2 \
				|IMXUART_MINT_USR2|IMXUART_RXDMA_USR2 \
				|IMXUART_TXDMA_USR2)

/*
 * all intr controls by reg
 */
#define	IMXUART_INTRS_UCR1	(IMX_UCR1_RRDYEN|IMX_UCR1_IDEN \
				|IMX_UCR1_TXMPTYEN|IMX_UCR1_TRDYEN \
				|IMX_UCR1_ADEN|IMX_UCR1_RTSDEN \
				|IMX_UCR1_RXDMAEN|IMX_UCR1_RXDMAEN \
				|IMX_UCR1_ATDMAEN|IMX_UCR1_TXDMAEN)
#define	IMXUART_INTRS_UCR2	(IMX_UCR2_ATEN|IMX_UCR2_ESCI|IMX_UCR2_RTSEN)
#define	IMXUART_INTRS_UCR3	(IMX_UCR3_RXDSEN|IMX_UCR3_ACIEN \
				|IMX_UCR3_AIRINTEN|IMX_UCR3_AWAKEN \
				|IMX_UCR3_FRAERREN|IMX_UCR3_PARERREN \
				|IMX_UCR3_DTREN|IMX_UCR3_RI \
				|IMX_UCR3_DCD|IMX_UCR3_DTRDEN)
#define	IMXUART_INTRS_UCR4	(IMX_UCR4_DREN|IMX_UCR4_TCEN|IMX_UCR4_OREN \
				|IMX_UCR4_BKEN|IMX_UCR4_WKEN \
				|IMX_UCR4_ENIRI|IMX_UCR4_IDDMAEN)


#endif	/* _IMXUARTREG_H */
