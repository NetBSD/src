/*	$NetBSD: iomdreg.h,v 1.1 2001/10/05 22:27:41 reinoud Exp $	*/

/*
 * Copyright (c) 1994-1997 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * iomd.h
 *
 * IOMD registers
 *
 * Created      : 18/09/94
 *
 * Based on kate/display/iomd.h
 */

#define IOMD_HW_BASE	0x03200000

#define IOMD_BASE	0xf6000000

#define IOMD_IOCR	0x00000000
#define IOMD_KBDDAT	0x00000001
#define IOMD_KBDCR	0x00000002
#define IOMD_IOLINES	0x00000003	/* ARM7500FE */

#define IOMD_IRQSTA	0x00000004
#define IOMD_IRQRQA	0x00000005
#define IOMD_IRQMSKA	0x00000006
#define IOMD_SUSPEND	0x00000007	/* ARM7500 */

#define IOMD_IRQSTB	0x00000008
#define IOMD_IRQRQB	0x00000009
#define IOMD_IRQMSKB	0x0000000a
#define IOMD_STOP	0x0000000b	/* ARM7500 */

#define IOMD_FIQST	0x0000000c
#define IOMD_FIQRQ	0x0000000d
#define IOMD_FIQMSK	0x0000000e
#define IOMD_CLKCTL	0x0000000f	/* ARM7500 */

#define IOMD_T0LOW	0x00000010
#define IOMD_T0HIGH	0x00000011
#define IOMD_T0GO	0x00000012
#define IOMD_T0LATCH	0x00000013

#define IOMD_T1LOW	0x00000014
#define IOMD_T1HIGH	0x00000015
#define IOMD_T1GO	0x00000016
#define IOMD_T1LATCH	0x00000017

/*
 * For ARM7500, it's not really a IOMD device.
 */

#define IOMD_IRQSTC	0x00000018	/* ARM7500 */
#define IOMD_IRQRQC	0x00000019	/* ARM7500 */
#define IOMD_IRQMSKC	0x0000001a	/* ARM7500 */
#define IOMD_VIDMUX	0x0000001b	/* ARM7500 */
 
#define IOMD_IRQSTD	0x0000001c	/* ARM7500 */
#define IOMD_IRQRQD	0x0000001d	/* ARM7500 */
#define IOMD_IRQMSKD	0x0000001e	/* ARM7500 */

#define IOMD_ROMCR0	0x00000020
#define IOMD_ROMCR1	0x00000021
#define IOMD_DRAMCR	0x00000022	/* !ARM7500 */
#define IOMD_VREFCR	0x00000023	/* !ARM7500 */
#define IOMD_REFCR	0x00000023	/* ARM7500 */

#define IOMD_FSIZE	0x00000024
#define IOMD_ID0	0x00000025
#define IOMD_ID1	0x00000026
#define IOMD_VERSION	0x00000027

#define IOMD_MOUSEX	0x00000028
#define IOMD_MOUSEY	0x00000029
#define IOMD_MSDATA	0x0000002a	/* ARM7500 */
#define IOMD_MSCR	0x0000002b	/* ARM7500 */

#define IOMD_DMATCR	0x00000030
#define IOMD_IOTCR	0x00000031
#define IOMD_ECTCR	0x00000032
#define IOMD_DMAEXT	0x00000033	/* !ARM7500 */
#define IOMD_ASTCR	0x00000033	/* ARM7500 */

#define IOMD_DRAMWID	0x00000034	/* ARM7500 */
#define IOMD_SELFREF	0x00000035	/* ARM7500 */

#define IOMD_ATODICR	0x00000038	/* ARM7500 */
#define IOMD_ATODSR	0x00000039	/* ARM7500 */
#define IOMD_ATODCR	0x0000003a	/* ARM7500 */
#define IOMD_ATODCNT1	0x0000003b	/* ARM7500 */
#define IOMD_ATODCNT2	0x0000003c	/* ARM7500 */
#define IOMD_ATODCNT3	0x0000003d	/* ARM7500 */
#define IOMD_ATODCNT4	0x0000003e	/* ARM7500 */

#define IOMD_DMA_SIZE		24
#define IOMD_DMA_SPACING	32
#define IOMD_IO0CURA	0x00000040
#define IOMD_IO0ENDA	0x00000041
#define IOMD_IO0CURB	0x00000042
#define IOMD_IO0ENDB	0x00000043
#define IOMD_IO0CR	0x00000044
#define IOMD_IO0ST	0x00000045
#define IOMD_IO1CURA	0x00000048
#define IOMD_IO1ENDA	0x00000049
#define IOMD_IO1CURB	0x0000004a
#define IOMD_IO1ENDB	0x0000004b
#define IOMD_IO1CR	0x0000004c
#define IOMD_IO1ST	0x0000004d
#define IOMD_IO2CURA	0x00000050
#define IOMD_IO2ENDA	0x00000051
#define IOMD_IO2CURB	0x00000052
#define IOMD_IO2ENDB	0x00000053
#define IOMD_IO2CR	0x00000054
#define IOMD_IO2ST	0x00000055
#define IOMD_IO3CURA	0x00000058
#define IOMD_IO3ENDA	0x00000059
#define IOMD_IO3CURB	0x0000005a
#define IOMD_IO3ENDB	0x0000005b
#define IOMD_IO3CR	0x0000005c
#define IOMD_IO3ST	0x0000005d

#define IOMD_SD0CURA	0x00000060
#define IOMD_SD0ENDA	0x00000061
#define IOMD_SD0CURB	0x00000062
#define IOMD_SD0ENDB	0x00000063
#define IOMD_SD0CR	0x00000064
#define IOMD_SD0ST	0x00000065

#define IOMD_SD1CURA	0x00000068
#define IOMD_SD1ENDA	0x00000069
#define IOMD_SD1CURB	0x0000006a
#define IOMD_SD1ENDB	0x0000006b
#define IOMD_SD1CR	0x0000006c
#define IOMD_SD1ST	0x0000006d

#define IOMD_CURSCUR	0x00000070
#define IOMD_CURSINIT	0x00000071

#define IOMD_VIDCUR	0x00000074
#define IOMD_VIDEND	0x00000075
#define IOMD_VIDSTART	0x00000076
#define IOMD_VIDINIT	0x00000077
#define IOMD_VIDCR	0x00000078

#define IOMD_DMAST	0x0000007c
#define IOMD_DMARQ	0x0000007d
#define IOMD_DMAMSK	0x0000007e

#define IOMD_SIZE	0x100	/* XXX - should be words ? */

/*
 * Ok these mouse buttons are not strickly part of the iomd but
 * this register is required if the IOMD supports a quadrature mouse
 */

#define IO_HW_MOUSE_BUTTONS	0x03210000
#define IO_MOUSE_BUTTONS	0xf6010000

#define MOUSE_BUTTON_RIGHT  0x10
#define MOUSE_BUTTON_MIDDLE 0x20
#define MOUSE_BUTTON_LEFT   0x40

#define FREQCON	(iomd_base + 0x40000)

#define RPC600_IOMD_ID		0xd4e7
#define ARM7500_IOC_ID		0x5b98
#define ARM7500FE_IOC_ID	0xaa7c

#define IOMD_ADDRESS(reg)	(iomd_base + (reg << 2))
#define IOMD_WRITE_BYTE(reg, val)	\
	(*((volatile unsigned char *)(IOMD_ADDRESS(reg))) = (val))
#define IOMD_WRITE_WORD(reg, val)	\
	(*((volatile unsigned int *)(IOMD_ADDRESS(reg))) = (val))
#define IOMD_READ_BYTE(reg)		\
	(*((volatile unsigned char *)(IOMD_ADDRESS(reg))))
#define IOMD_READ_WORD(reg)		\
	(*((volatile unsigned int *)(IOMD_ADDRESS(reg))))

#define IOMD_ID (IOMD_READ_BYTE(IOMD_ID0) | (IOMD_READ_BYTE(IOMD_ID1) << 8))

/* End of iomdreg.h */
