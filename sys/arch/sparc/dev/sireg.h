/*	$NetBSD: sireg.h,v 1.3.38.1 2000/06/22 17:03:57 minoura Exp $	*/

/*
 * Register map for the Sun3 SCSI Interface (si)
 * The first part of this register map is an NCR5380
 * SCSI Bus Interface Controller (SBIC).  The rest is a
 * DMA controller and custom logic in one of two flavors,
 * one for the OBIO interface (3/50,3/60) and one for the
 * VME interface (3/160,3/260,etc.), where some registers
 * are implemented only on one or the other, some on both.
 *
 * Modified for Sun 4 systems by Jason R. Thorpe <thorpej@NetBSD.ORG>.
 */

/*
 * Note that the obio version on the 4/1xx (the so-called "SCSI Weird", or
 * "sw" controller) is laid out a bit differently, and hence the evilness
 * with unions.  Also, the "sw" doesn't appear to have a FIFO.
 */

#if __for_reference_only__
/*
 * Am5380 Register map (no padding). See dev/ic/ncr5380reg.h
 */
struct ncr5380regs {
	u_char r[8];
};

struct si_regs {
	struct ncr5380regs sci;

	/* DMA controller registers */
	u_short	_Dma_addrh;	/* dma address (VME only) */
	u_short	_Dma_addrl;	/* (high word, low word)  */
	u_short	_Dma_counth;	/* dma count   (VME only) */
	u_short	_Dma_countl;	/* (high word, low word)  */

	u_int		si_pad0;		/* no-existent register */

	u_short	_Fifo_data;	/* fifo data register */
	u_short	_Fifo_count;	/* fifo count register */
	u_short	_Si_csr;	/* si control/status */
	u_short	_Bprh;		/* VME byte pack high */
	u_short	_Bprl;		/* VME byte pack low */
	u_short	iv_am;		/* bits 0-7: intr vector */
				/* bits 8-13: addr modifier (VME only) */
				/* bits 14-15: unused */
	u_short	fifo_cnt_hi;	/* high part of fifo_count (VME only) */

	/* Whole thing repeats after 32 bytes. */
	u_short	_space[3];
};

struct sw_regs {
	struct ncr5380regs sci;

	/* DMA controller registers on OBIO */
	u_int	_Dma_addr;	/* dma address */
	u_int	_Dma_count;	/* dma count */
	u_int	si_pad0;	/* no-existent register */
	u_int	_Sw_csr;	/* sw control/status */
	u_int	_Bpr;		/* sw byte pack */
};
#endif

/*
 * Size of NCR5380 registers located at the bottom of the register bank
 */
#define NCR5380REGS_SZ	8

/*
 * Register definition for the `si' VME controller
 */
#define SIREG_DMA_ADDRH	(NCR5380REGS_SZ + 0)	/* DMA address, high word */
#define SIREG_DMA_ADDRL	(NCR5380REGS_SZ + 2)	/* DMA address, low word */
#define SIREG_DMA_CNTH	(NCR5380REGS_SZ + 4)	/* DMA count, high word */
#define SIREG_DMA_CNTL	(NCR5380REGS_SZ + 6)	/* DMA count, low word */
#define SIREG_FIFO_DATA	(NCR5380REGS_SZ + 12)	/* FIFO data */
#define SIREG_FIFO_CNT	(NCR5380REGS_SZ + 14)	/* FIFO count, low word */
#define SIREG_CSR	(NCR5380REGS_SZ + 16)	/* Control/status register */
#define SIREG_BPRH	(NCR5380REGS_SZ + 18)	/* VME byte pack, high word */
#define SIREG_BPRL	(NCR5380REGS_SZ + 20)	/* VME byte pack, low word */
#define SIREG_IV_AM	(NCR5380REGS_SZ + 22)	/* bits 0-7: intr vector;
						   bits 8-13: addr modifier */
#define SIREG_FIFO_CNTH	(NCR5380REGS_SZ + 24)	/* FIFO count, high word */
#define SIREG_BANK_SZ	(NCR5380REGS_SZ + 26)

/*
 * Register definition for the `sw' OBIO controller
 */
#define SWREG_DMA_ADDR	(NCR5380REGS_SZ + 0)
#define SWREG_DMA_CNT	(NCR5380REGS_SZ + 4)
#define SWREG_CSR	(NCR5380REGS_SZ + 12)
#define SWREG_BPR	(NCR5380REGS_SZ + 16)
#define SWREG_BANK_SZ	(NCR5380REGS_SZ + 20)

/*
 * Status Register.
 * Note:
 *	(r)	indicates bit is read only.
 *	(rw)	indicates bit is read or write.
 *	(v)	vme host adaptor interface only.
 *	(o)	sun3/50 onboard host adaptor interface only.
 *	(b)	both vme and sun3/50 host adaptor interfaces.
 */
#define SI_CSR_DMA_ACTIVE	0x8000	/* (r,o) dma transfer active */
#define SI_CSR_DMA_CONFLICT	0x4000	/* (r,b) reg accessed while dmaing */
#define SI_CSR_DMA_BUS_ERR	0x2000	/* (r,b) bus error during dma */
#define SI_CSR_ID		0x1000	/* (r,b) 0 for 3/50, 1 for SCSI-3, */
					/* 0 if SCSI-3 unmodified */
#define SI_CSR_FIFO_FULL	0x0800	/* (r,b) fifo full */
#define SI_CSR_FIFO_EMPTY	0x0400	/* (r,b) fifo empty */
#define SI_CSR_SBC_IP		0x0200	/* (r,b) sbc interrupt pending */
#define SI_CSR_DMA_IP		0x0100	/* (r,b) dma interrupt pending */
#define SI_CSR_LOB		0x00c0	/* (r,v) number of leftover bytes */
#define SI_CSR_LOB_THREE	0x00c0	/* (r,v) three leftover bytes */
#define SI_CSR_LOB_TWO		0x0080	/* (r,v) two leftover bytes */
#define SI_CSR_LOB_ONE		0x0040	/* (r,v) one leftover byte */
#define SI_CSR_BPCON		0x0020	/* (rw,v) byte packing control */
					/* dma is in 0=longwords, 1=words */
#define SI_CSR_DMA_EN		0x0010	/* (rw,v) dma/interrupt enable */
#define SI_CSR_SEND		0x0008	/* (rw,b) dma dir, 1=to device */
#define SI_CSR_INTR_EN		0x0004	/* (rw,b) interrupts enable */
#define SI_CSR_FIFO_RES		0x0002	/* (rw,b) inits fifo, 0=reset */
#define SI_CSR_SCSI_RES		0x0001	/* (rw,b) reset sbc and udc, 0=reset */
