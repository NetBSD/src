/*	$NetBSD: if_edreg.h,v 1.17 1997/11/02 00:57:26 thorpej Exp $	*/

/*
 * National Semiconductor DS8390 NIC register definitions.
 *
 * Copyright (C) 1993, David Greenman.  This software may be used, modified,
 * copied, distributed, and sold, in both source and binary form provided that
 * the above copyright and these terms are retained.  Under no circumstances is
 * the author responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with its use.
 */

/*
 * Vendor types
 */
#define ED_VENDOR_WD_SMC	0x00	/* Western Digital/SMC */

/*
 * Compile-time config flags
 */
/*
 * This sets the default for enabling/disablng the tranceiver.
 */
#define ED_FLAGS_DISABLE_TRANCEIVER	0x0001

/*
 * This forces the board to be used in 8/16-bit mode even if it autoconfigs
 * differently.
 */
#define ED_FLAGS_FORCE_8BIT_MODE	0x0002
#define ED_FLAGS_FORCE_16BIT_MODE	0x0004

/*
 * This disables the use of double transmit buffers.
 */
#define ED_FLAGS_NO_MULTI_BUFFERING	0x0008

/*
 * This forces all operations with the NIC memory to use Programmed I/O (i.e.
 * not via shared memory).
 */
#define ED_FLAGS_FORCE_PIO		0x0010

/*
 *		Definitions for Western digital/SMC WD80x3 series ASIC
 */
/*
 * Memory Select Register (MSR)
 */
#define ED_WD_MSR	0

/* next three definitions for Toshiba */
#define	ED_WD_MSR_POW	0x02	/* 0 = power save, 1 = normal (R/W) */
#define	ED_WD_MSR_BSY	0x04	/* gate array busy (R) */
#define	ED_WD_MSR_LEN	0x20	/* 0 = 16-bit, 1 = 8-bit (R/W) */

#define ED_WD_MSR_ADDR	0x3f	/* Memory decode bits 18-13 */
#define ED_WD_MSR_MENB	0x40	/* Memory enable */
#define ED_WD_MSR_RST	0x80	/* Reset board */

/*
 * Interface Configuration Register (ICR)
 */
#define ED_WD_ICR	1

#define ED_WD_ICR_16BIT	0x01	/* 16-bit interface */
#define ED_WD_ICR_OAR	0x02	/* select register (0=BIO 1=EAR) */
#define ED_WD_ICR_IR2	0x04	/* high order bit of encoded IRQ */
#define ED_WD_ICR_MSZ	0x08	/* memory size (0=8k 1=32k) */
#define ED_WD_ICR_RLA	0x10	/* recall LAN address */
#define ED_WD_ICR_RX7	0x20	/* recall all but i/o and LAN address */
#define	ED_WD_ICR_RIO	0x40	/* recall i/o address */
#define ED_WD_ICR_STO	0x80	/* store to non-volatile memory */
#ifdef TOSH_ETHER
#define	ED_WD_ICR_MEM	0xe0	/* shared mem address A15-A13 (R/W) */
#define	ED_WD_ICR_MSZ1	0x0f	/* memory size, 0x08 = 64K, 0x04 = 32K,
				   0x02 = 16K, 0x01 = 8K */
				/* 64K can only be used if mem address
				   above 1MB */
				/* IAR holds address A23-A16 (R/W) */
#endif

/*
 * IO Address Register (IAR)
 */
#define ED_WD_IAR	2

/*
 * EEROM Address Register
 */
#define ED_WD_EAR	3

/*
 * Interrupt Request Register (IRR)
 */
#define ED_WD_IRR	4

#define	ED_WD_IRR_0WS	0x01	/* use 0 wait-states on 8 bit bus */
#define ED_WD_IRR_OUT1	0x02	/* WD83C584 pin 1 output */
#define ED_WD_IRR_OUT2	0x04	/* WD83C584 pin 2 output */
#define ED_WD_IRR_OUT3	0x08	/* WD83C584 pin 3 output */
#define ED_WD_IRR_FLASH	0x10	/* Flash RAM is in the ROM socket */

/*
 * The three bits of the encoded IRQ are decoded as follows:
 *
 * IR2 IR1 IR0  IRQ
 *  0   0   0   2/9
 *  0   0   1   3
 *  0   1   0   5
 *  0   1   1   7
 *  1   0   0   10
 *  1   0   1   11
 *  1   1   0   15
 *  1   1   1   4
 */
#define ED_WD_IRR_IR0	0x20	/* bit 0 of encoded IRQ */
#define ED_WD_IRR_IR1	0x40	/* bit 1 of encoded IRQ */
#define ED_WD_IRR_IEN	0x80	/* Interrupt enable */

/*
 * LA Address Register (LAAR)
 */
#define ED_WD_LAAR	5

#define ED_WD_LAAR_ADDRHI	0x1f	/* bits 23-19 of RAM address */
#define ED_WD_LAAR_0WS16	0x20	/* enable 0 wait-states on 16 bit bus */
#define ED_WD_LAAR_L16EN	0x40	/* enable 16-bit operation */
#define ED_WD_LAAR_M16EN	0x80	/* enable 16-bit memory access */

/* i/o base offset to station address/card-ID PROM */
#define ED_WD_PROM	8

/*
 *	83C790 specific registers
 */
/*
 * Hardware Support Register (HWR) ('790)
 */
#define	ED_WD790_HWR	4

#define	ED_WD790_HWR_RST	0x10	/* hardware reset */
#define	ED_WD790_HWR_LPRM	0x40	/* LAN PROM select */
#define	ED_WD790_HWR_SWH	0x80	/* switch register set */

/*
 * ICR790 Interrupt Control Register for the 83C790
 */
#define	ED_WD790_ICR	6

#define	ED_WD790_ICR_EIL	0x01	/* enable interrupts */

/*
 * REV/IOPA Revision / I/O Pipe register for the 83C79X
 */
#define ED_WD790_REV	7

#define ED_WD790	0x20
#define ED_WD795	0x40

/*
 * 79X RAM Address Register (RAR)
 *      Enabled with SWH bit=1 in HWR register
 */

#define ED_WD790_RAR	0x0b

#define ED_WD790_RAR_SZ8	0x00	/* 8k memory buffer */
#define ED_WD790_RAR_SZ16	0x10	/* 16k memory buffer */
#define ED_WD790_RAR_SZ32	0x20	/* 32k memory buffer */
#define ED_WD790_RAR_SZ64	0x30	/* 64k memory buffer */

/*
 * General Control Register (GCR)
 * Eanbled with SWH bit == 1 in HWR register
 */
#define	ED_WD790_GCR	0x0d

#define	ED_WD790_GCR_IR0	0x04	/* bit 0 of encoded IRQ */
#define	ED_WD790_GCR_IR1	0x08	/* bit 1 of encoded IRQ */
#define	ED_WD790_GCR_ZWSEN	0x20	/* zero wait state enable */
#define	ED_WD790_GCR_IR2	0x40	/* bit 2 of encoded IRQ */
/*
 * The three bits of the encoded IRQ are decoded as follows:
 *
 * IR2 IR1 IR0  IRQ
 *  0   0   0   none
 *  0   0   1   9
 *  0   1   0   3
 *  0   1   1   5
 *  1   0   0   7
 *  1   0   1   10
 *  1   1   0   11
 *  1   1   1   15
 */

/* i/o base offset to CARD ID */
#define ED_WD_CARD_ID	ED_WD_PROM+6

/* Board type codes in card ID */
#define ED_TYPE_WD8003S		0x02
#define ED_TYPE_WD8003E		0x03
#define ED_TYPE_WD8013EBT	0x05
#define	ED_TYPE_TOSHIBA1	0x11	/* named PCETA1 */
#define	ED_TYPE_TOSHIBA2	0x12	/* named PCETA2 */
#define	ED_TYPE_TOSHIBA3	0x13	/* named PCETB */
#define	ED_TYPE_TOSHIBA4	0x14	/* named PCETC */
#define	ED_TYPE_WD8003W		0x24
#define	ED_TYPE_WD8003EB	0x25
#define	ED_TYPE_WD8013W		0x26
#define ED_TYPE_WD8013EP	0x27
#define ED_TYPE_WD8013WC	0x28
#define ED_TYPE_WD8013EPC	0x29
#define	ED_TYPE_SMC8216T	0x2a
#define	ED_TYPE_SMC8216C	0x2b
#define ED_TYPE_WD8013EBP	0x2c

/* Bit definitions in card ID */
#define	ED_WD_REV_MASK		0x1f	/* Revision mask */
#define	ED_WD_SOFTCONFIG	0x20	/* Soft config */
#define	ED_WD_LARGERAM		0x40	/* Large RAM */
#define	ED_MICROCHANEL		0x80	/* Microchannel bus (vs. isa) */

/*
 * Checksum total.  All 8 bytes in station address PROM will add up to this.
 */
#ifdef TOSH_ETHER
#define ED_WD_ROM_CHECKSUM_TOTAL	0xA5
#else
#define ED_WD_ROM_CHECKSUM_TOTAL	0xFF
#endif

#define ED_WD_NIC_OFFSET	0x10	/* I/O base offset to NIC */
#define ED_WD_ASIC_OFFSET	0	/* I/O base offset to ASIC */
#define ED_WD_IO_PORTS		32	/* # of i/o addresses used */

#define ED_WD_PAGE_OFFSET	0	/* page offset for NIC access to mem */
