/*	$NetBSD: adrsmap.h,v 1.3 1999/12/22 05:55:26 tsubai Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: $Hdr: adrsmap.h,v 4.300 91/06/09 06:34:29 root Rel41 $ SONY
 *
 *	@(#)adrsmap.h	8.1 (Berkeley) 6/11/93
 */

/*
 * adrsmap.h
 *
 * Define all hardware address map.
 */

#ifndef __MACHINE_ADRSMAP__
#define __MACHINE_ADRSMAP__

/*----------------------------------------------------------------------
 *	news3400
 *----------------------------------------------------------------------*/
/*
 * timer
 */
#define	RTC_PORT	0xbff407f8
#define	DATA_PORT	0xbff407f9

#ifdef notdef
#define	EN_ITIMER	0xb8000004	/*XXX:???*/
#endif

#define	INTEN0	0xbfc80000
#define		INTEN0_PERR	0x80
#define		INTEN0_ABORT	0x40
#define		INTEN0_BERR	0x20
#define		INTEN0_TIMINT	0x10
#define		INTEN0_KBDINT	0x08
#define		INTEN0_MSINT	0x04
#define		INTEN0_CFLT	0x02
#define		INTEN0_CBSY	0x01

#define	INTEN1	0xbfc80001
#define		INTEN1_BEEP	0x80
#define		INTEN1_SCC	0x40
#define		INTEN1_LANCE	0x20
#define		INTEN1_DMA	0x10
#define		INTEN1_SLOT1	0x08
#define		INTEN1_SLOT3	0x04
#define		INTEN1_EXT1	0x02
#define		INTEN1_EXT3	0x01

#define	INTST0	0xbfc80002
#define		INTST0_PERR	0x80
#define		INTST0_ABORT	0x40
#define		INTST0_BERR	0x00	/* N/A */
#define		INTST0_TIMINT	0x10
#define		INTST0_KBDINT	0x08
#define		INTST0_MSINT	0x04
#define		INTST0_CFLT	0x02
#define		INTST0_CBSY	0x01
#define			INTST0_PERR_BIT		7
#define			INTST0_ABORT_BIT	6
#define			INTST0_BERR_BIT		5	/* N/A */
#define			INTST0_TIMINT_BIT	4
#define			INTST0_KBDINT_BIT	3
#define			INTST0_MSINT_BIT	2
#define			INTST0_CFLT_BIT		1
#define			INTST0_CBSY_BIT		0

#define	INTST1	0xbfc80003
#define		INTST1_BEEP	0x80
#define		INTST1_SCC	0x40
#define		INTST1_LANCE	0x20
#define		INTST1_DMA	0x10
#define		INTST1_SLOT1	0x08
#define		INTST1_SLOT3	0x04
#define		INTST1_EXT1	0x02
#define		INTST1_EXT3	0x01
#define			INTST1_BEEP_BIT		7
#define			INTST1_SCC_BIT		6
#define			INTST1_LANCE_BIT	5
#define			INTST1_DMA_BIT		4
#define			INTST1_SLOT1_BIT	3
#define			INTST1_SLOT3_BIT	2
#define			INTST1_EXT1_BIT		1
#define			INTST1_EXT3_BIT		0

#define	INTCLR0	0xbfc80004
#define		INTCLR0_PERR	0x80
#define		INTCLR0_ABORT	0x40
#define		INTCLR0_BERR	0x20
#define		INTCLR0_TIMINT	0x10
#define		INTCLR0_KBDINT	0x00	/* N/A */
#define		INTCLR0_MSINT	0x00	/* N/A */
#define		INTCLR0_CFLT	0x02
#define		INTCLR0_CBSY	0x01

#define	INTCLR1	0xbfc80005
#define		INTCLR1_BEEP	0x80
#define		INTCLR1_SCC	0x00	/* N/A */
#define		INTCLR1_LANCE	0x00	/* N/A */
#define		INTCLR1_DMA	0x00	/* N/A */
#define		INTCLR1_SLOT1	0x00	/* N/A */
#define		INTCLR1_SLOT3	0x00	/* N/A */
#define		INTCLR1_EXT1	0x00	/* N/A */
#define		INTCLR1_EXT3	0x00	/* N/A */

#define	ITIMER		0xbfc80006
#define	IOCLOCK		4915200

#define	DIP_SWITCH	0xbfe40000
#define	IDROM		0xbfe80000

#define	DEBUG_PORT	0xbfcc0003
#define		DP_READ		0x00
#define		DP_WRITE	0xf0
#define		DP_LED0		0x01
#define		DP_LED1		0x02
#define		DP_LED2		0x04
#define		DP_LED3		0x08


#define	LANCE_PORT	0xbff80000
#define	LANCE_MEMORY	0xbffc0000
#define	ETHER_ID	IDROM_PORT

#define	LANCE_PORT1	0xb8c30000	/* expansion lance #1 */
#define	LANCE_MEMORY1	0xb8c20000
#define	ETHER_ID1	0xb8c38000

#define	LANCE_PORT2	0xb8c70000	/* expansion lance #2 */
#define	LANCE_MEMORY2	0xb8c60000
#define	ETHER_ID2	0xb8c78000

#define	IDROM_PORT	0xbfe80000

#define	SCCPORT0B	0xbfec0000
#define	SCCPORT0A	0xbfec0002
#define SCCPORT1B	0xb8c40100
#define SCCPORT1A	0xb8c40102
#define SCCPORT2B	0xb8c40104
#define SCCPORT2A	0xb8c40106
#define SCCPORT3B	0xb8c40110
#define SCCPORT3A	0xb8c40112
#define SCCPORT4B	0xb8c40114
#define SCCPORT4A	0xb8c40116

#define	SCC_STATUS0	0xbfcc0002
#define	SCC_STATUS1	0xb8c40108
#define	SCC_STATUS2	0xb8c40118

#define	SCCVECT		(0x1fcc0007 | MIPS_KSEG1_START)
#define	SCC_RECV	2
#define	SCC_XMIT	0
#define	SCC_CTRL	3
#define	SCC_STAT	1
#define	SCC_INT_MASK	0x6

/*XXX: SHOULD BE FIX*/
#define	KEYB_DATA	0xbfd00000	/* keyboard data port */
#define KEYB_STAT	0xbfd00001	/* keyboard status port */
#define	KEYB_INTE	INTEN0		/* keyboard interrupt enable */
#define	KEYB_RESET	0xbfd00002	/* keyboard reset port*/
#define	KEYB_INIT1	0xbfd00003	/* keyboard speed */
#define	KEYB_INIT2	KEYB_INIT1	/* keyboard clock */
#define	KEYB_BUZZ	0xbfd40001	/* keyboard buzzer (length) */
#define	KEYB_BUZZF	0xbfd40000	/* keyboard buzzer frequency */
#define	MOUSE_DATA	0xbfd00004	/* mouse data port */
#define MOUSE_STAT	0xbfd00005	/* mouse status port */
#define	MOUSE_INTE	INTEN0		/* mouse interrupt enable */
#define	MOUSE_RESET	0xbfd00006	/* mouse reset port */
#define	MOUSE_INIT1	0xbfd00007	/* mouse speed */
#define	MOUSE_INIT2	MOUSE_INIT1	/* mouse clock */

#define	RX_MSINTE	0x04		/* Mouse Interrupt Enable */
#define RX_KBINTE	0x08		/* Keyboard Intr. Enable */
#define	RX_MSINT	0x04		/* Mouse Interrupted */
#define	RX_KBINT	0x08		/* Keyboard Interrupted */
#define	RX_MSBUF	0x01		/* Mouse data buffer Full */
#define	RX_KBBUF	0x01		/* Keyboard data Full */
#define	RX_MSRDY	0x02		/* Mouse data ready */
#define	RX_KBRDY	0x02		/* Keyboard data ready */
/*XXX: SHOULD BE FIX*/

#define	ABEINT_BADDR	0xbfdc0038

#define	NEWS5000_DIP_SWITCH	0xbf3d0000
#define	NEWS5000_IDROM		0xbf3c0000

#define	NEWS5000_TIMER0		0xbf800000
#define	NEWS5000_TIMER1		0xbf800004
#define	NEWS5000_TIMER1_PERIOD	0xbf80000c
#define	NEWS5000_TIMER_COUNTER	0xbf840000
#define	NEWS5000_TIMER_LOAD	0xbf840004
#define	NEWS5000_NVRAM		0xbf880000
#define	NEWS5000_NVRAM_SIZE	0x07f8
#define	NEWS5000_RTC_ADDR	0xbf881fe0

#define	NEWS5000_INTCLR0	0xbf4e0000
#define	NEWS5000_INTCLR1	0xbf4e0004
#define	NEWS5000_INTCLR2	0xbf4e0008
#define	NEWS5000_INTCLR3	0xbf4e000c
#define	NEWS5000_INTCLR4	0xbf4e0010
#define	NEWS5000_INTCLR5	0xbf4e0014
#define	NEWS5000_INTCLRNMI	0xbf4e0018

#define	NEWS5000_INTMASK0	0xbfa00000
#define	NEWS5000_INTMASK1	0xbfa00004
#define	NEWS5000_INTMASK2	0xbfa00008
#define	NEWS5000_INTMASK3	0xbfa0000c
#define	NEWS5000_INTMASK4	0xbfa00010
#define	NEWS5000_INTMASK5	0xbfa00014
#define	NEWS5000_INTMASKNMI	0xbfa00018

#define	NEWS5000_INTSTAT0	0xbfa00020
#define	NEWS5000_INTSTAT1	0xbfa00024
#define	NEWS5000_INTSTAT2	0xbfa00028
#define	NEWS5000_INTSTAT3	0xbfa0002c
#define	NEWS5000_INTSTAT4	0xbfa00030
#define	NEWS5000_INTSTAT5	0xbfa00034
#define	NEWS5000_INTSTATNMI	0xbfa00038

#define	NEWS5000_INTSLOT0	0x00000100
#define	NEWS5000_INTSLOT1	0x00000200
#define	NEWS5000_INTSLOT2	0x00000400
#define	NEWS5000_INTSLOT3	0x00000800
#define	NEWS5000_INTSLOT4	0x00001000
#define	NEWS5000_INTSLOT5	0x00002000
#define	NEWS5000_INTSLOT_ALL	0x00003f00


/*
 * level0 intr (INTMASK0/INTSTAT0)
 */
#define	NEWS5000_INT0_DMAC	0x0001
#define	NEWS5000_INT0_SONIC	0x0002
#define	NEWS5000_INT0_FIFO0	0x0004
#define	NEWS5000_INT0_FIFO1	0x0008
#define	NEWS5000_INT0_FDC	0x0010
#define	NEWS5000_INT0_ALL	(NEWS5000_INT0_DMAC|NEWS5000_INT0_SONIC|NEWS5000_INT0_FIFO0|NEWS5000_INT0_FIFO1|NEWS5000_INT0_FDC)

/*
 * level1 intr (INTMASK1/INTSTAT1)
 */
#define	NEWS5000_INT1_KBD	0x0001
#define	NEWS5000_INT1_SERIAL	0x0002
#define	NEWS5000_INT1_AUDIO0	0x0004
#define	NEWS5000_INT1_AUDIO1	0x0008
#define	NEWS5000_INT1_PARALLEL	0x0020
#define	NEWS5000_INT1_FB	0x0080
#define	NEWS5000_INT1_ALL	(NEWS5000_INT1_KBD|NEWS5000_INT1_SERIAL|NEWS5000_INT1_AUDIO0|NEWS5000_INT1_AUDIO1|NEWS5000_INT1_PARALLEL|NEWS5000_INT1_FB)

/*
 * level2 intr (INTMASK2/INTSTAT2)
 */
#define	NEWS5000_INT2_TIMER0	0x0001
#define	NEWS5000_INT2_TIMER1	0x0002
#define	NEWS5000_INT2_ALL	(NEWS5000_INT2_TIMER0|NEWS5000_INT2_TIMER1)

/*
 * level4 intr (INTMASK4/INTSTAT4)
 */
#define	NEWS5000_INT4_ABIF	0x0001
#define	NEWS5000_INT4_WBERR	NEWS5000_INT4_ABIF
#define	NEWS5000_INT4_MFBIF	0x0002
#define	NEWS5000_INT4_SBIF	0x0004
#define	NEWS5000_INT4_ALL	(NEWS5000_INT4_ABIF|NEWS5000_INT4_MFBIF|NEWS5000_INT4_SBIF)

/*
 * level5 intr (INTMASK5/INTSTAT5)
 */
#define	NEWS5000_INT5_ABIF	0x0001
#define	NEWS5000_INT5_MBIF	0x0002
#define	NEWS5000_INT5_SBIF	0x0004
#define	NEWS5000_INT5_POWER	0x0008
#define	NEWS5000_INT5_TEMP	0x0010
#define	NEWS5000_INT5_ABORT	0x0020
#define	NEWS5000_INT5_EXTSENSE	NEWS5000_INT5_ABORT
#define	NEWS5000_INT5_ALL	(NEWS5000_INT5_ABIF|NEWS5000_INT5_MBIF|NEWS5000_INT5_SBIF|NEWS5000_INT5_POWER|NEWS5000_INT5_TEMP|NEWS5000_INT5_ABORT)

/*
 * NMI intr (INTMASKNMI/INTSTATNMI)
 */
#define	NEWS5000_INTNMI_ABORT	0x0001
#define	NEWS5000_INTNMI_ALL	(NEWS5000_INTNMI_ABORT)


#define	NEWS5000_WB		0xbf520004	/* I/O write buffer control */



#define	NEWS5000_LED0		0xbf3f0000	/* POWER LED */
#define	NEWS5000_LED1		0xbf3f0004	/* DISK LED */
#define	NEWS5000_LED2		0xbf3f0008	/* FLOPPY LED */
#define	NEWS5000_LED3		0xbf3f000c	/* SECURITY LED */
#define	NEWS5000_LED4		0xbf3f0010	/* NETWORK LED */
#define	NEWS5000_LED5		0xbf3f0014	/* CDROM LED */



#define	NEWS5000_APBUS_INTMASK	0xb4c0000c
#define	NEWS5000_APBUS_INTSTAT	0xb4c00014
#define	NEWS5000_APBUS_CONFIG	0xb4c00034
#define	NEWS5000_APBUS_DUMCOH	0xb4c00084
#define		NEWS5000_APBUS_DEVICE_DMAC3    0x80000000	/* DMAC3 */
#define		NEWS5000_APBUS_DEVICE_SONIC    0x40000000	/* SONIC */
#define		NEWS5000_APBUS_DEVICE_SLOT6    0x00000020	/* slot #6 */
#define		NEWS5000_APBUS_DEVICE_SLOT5    0x00000010	/* slot #5 */
#define		NEWS5000_APBUS_DEVICE_SLOT4    0x00000008	/* slot #4 */
#define		NEWS5000_APBUS_DEVICE_SLOT3    0x00000004	/* slot #3 */
#define		NEWS5000_APBUS_DEVICE_SLOT2    0x00000002	/* slot #2 */
#define		NEWS5000_APBUS_DEVICE_SLOT1    0x00000001	/* slot #1 */
#define		NEWS5000_APBUS_DEVICE_ALLSLOT  0x0000003f	/* slot #1-#6 */



#define	NEWS5000_INTAPBUS_DMADOUBLE	0x8000	/* DMA double error */
#define	NEWS5000_INTAPBUS_DMAPARITY	0x0200	/* DMA parity error */
#define	NEWS5000_INTAPBUS_DMAADDRESS	0x0100	/* DMA address error */
#define	NEWS5000_INTAPBUS_IODOUBLE	0x0080	/* I/O access double bus error */
#define	NEWS5000_INTAPBUS_IOPARITY	0x0010	/* I/O read parity error */
#define	NEWS5000_INTAPBUS_RDIOERR	0x0008	/* I/O read ioerr */
#define	NEWS5000_INTAPBUS_RDTIMEO	0x0004	/* I/O read time-out error */
#define	NEWS5000_INTAPBUS_WRIOERR	0x0002	/* I/O write ioerr */
#define	NEWS5000_INTAPBUS_WRTIMEO	0x0001	/* I/O write time-out error */
#define NEWS5000_INTAPBUS_ALL		(NEWS5000_INTAPBUS_DMADOUBLE|\
					NEWS5000_INTAPBUS_DMAPARITY|\
					NEWS5000_INTAPBUS_DMAADDRESS|\
					NEWS5000_INTAPBUS_IODOUBLE|\
					NEWS5000_INTAPBUS_IOPARITY|\
					NEWS5000_INTAPBUS_RDIOERR|\
					NEWS5000_INTAPBUS_RDTIMEO|\
					NEWS5000_INTAPBUS_WRIOERR|\
					NEWS5000_INTAPBUS_WRTIMEO)

#define NEWS5000_SCCPORT0A	0xbe950000

#endif /* !__MACHINE_ADRSMAP__ */
