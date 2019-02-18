/*	$NetBSD: cpu.h,v 1.49 2019/02/18 01:12:24 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */

#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#include "opt_m68k_arch.h"
#endif

/*
 * Get common m68k definitions.
 */
#include <m68k/cpu.h>

#if defined(_KERNEL)
/*
 * Exported definitions unique to next68k/68k cpu support.
 */
#define	M68K_MMU_MOTOROLA

/*
 * Get interrupt glue.
 */
#include <machine/intr.h>

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  On the next68k, we use
 * what the hardware pushes on an interrupt (frame format 0).
 */
struct clockframe {
	u_short	sr;		/* sr at time of interrupt */
	u_long	pc;		/* pc at time of interrupt */
	u_short	fmt:4,
		vec:12;		/* vector offset (4-word frame) */
} __attribute__((packed));

#define	CLKF_USERMODE(framep)	(((framep)->sr & PSL_S) == 0)
#define	CLKF_PC(framep)		((framep)->pc)

/*
 * The clock interrupt handler can determine if it's a nested
 * interrupt by checking for interrupt_depth > 1.
 * (Remember, the clock interrupt handler itself will cause the
 * depth counter to be incremented).
 */
extern volatile unsigned int interrupt_depth;
#define	CLKF_INTR(framep)	(interrupt_depth > 1)

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	cpu_need_resched(ci,flags)	do {	\
	__USE(flags); 				\
	ci->ci_want_resched = 1;		\
	aston();				\
} while (/*CONSTCOND*/0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the next68k, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	cpu_need_proftick(l)	((l)->l_pflag |= LP_OWEUPC, aston())

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	cpu_signotify(l)	aston()

#define aston() (astpending++)

extern	int	astpending;	/* need to trap before returning to user mode */

extern	void (*vectab[])(void);

/* locore.s functions */
void	loadustp(int);

void	doboot(void) __attribute__((__noreturn__));
int	nmihand(void *);

/* clock.c functions */
void	next68k_calibrate_delay(void);

#endif /* _KERNEL */

#define NEXT_RAMBASE  (0x4000000) /* really depends on slot, but... */
#define NEXT_BANKSIZE (0x1000000) /* Size of a memory bank in physical address */

#if 0
/* @@@ this needs to be fixed to work on 030's */
#define	NEXT_SLOT_ID		0x0
#ifdef	M68030
#define	NEXT_SLOT_ID_BMAP	0x0
#endif	/* M68030 */
#endif
#ifdef	M68040
#ifdef DISABLE_NEXT_BMAP_CHIP		/* @@@ For turbo testing */
#define	NEXT_SLOT_ID_BMAP	0x0
#else
#define	NEXT_SLOT_ID_BMAP	0x00100000
#endif
#define NEXT_SLOT_ID		0x0
#endif	/* M68040 */

/****************************************************************/

/* Eventually, I'd like to move these defines off into
 * configure somewhere
 * Darrin B Jewell <jewell@mit.edu>  Thu Feb  5 03:50:58 1998
 */
/* ROM */
#define NEXT_P_EPROM		(NEXT_SLOT_ID+0x00000000)
#define NEXT_P_EPROM_BMAP	(NEXT_SLOT_ID+0x01000000)
#define NEXT_P_EPROM_SIZE	(128 * 1024)

/* device space */
#define NEXT_P_DEV_SPACE	(NEXT_SLOT_ID+0x02000000)
#define NEXT_P_DEV_BMAP		(NEXT_SLOT_ID+0x02100000)
#define NEXT_DEV_SPACE_SIZE	0x0001c000

/* DMA control/status (writes MUST be 32-bit) */
#define NEXT_P_SCSI_CSR		(NEXT_SLOT_ID+0x02000010)
#define NEXT_P_SOUNDOUT_CSR	(NEXT_SLOT_ID+0x02000040)
#define NEXT_P_DISK_CSR		(NEXT_SLOT_ID+0x02000050)
#define NEXT_P_SOUNDIN_CSR	(NEXT_SLOT_ID+0x02000080)
#define NEXT_P_PRINTER_CSR	(NEXT_SLOT_ID+0x02000090)
#define NEXT_P_SCC_CSR		(NEXT_SLOT_ID+0x020000c0)
#define NEXT_P_DSP_CSR		(NEXT_SLOT_ID+0x020000d0)
#define NEXT_P_ENETX_CSR	(NEXT_SLOT_ID+0x02000110)
#define NEXT_P_ENETR_CSR	(NEXT_SLOT_ID+0x02000150)
#define NEXT_P_VIDEO_CSR	(NEXT_SLOT_ID+0x02000180)
#define NEXT_P_M2R_CSR		(NEXT_SLOT_ID+0x020001d0)
#define NEXT_P_R2M_CSR		(NEXT_SLOT_ID+0x020001c0)

/* DMA scratch pad (writes MUST be 32-bit) */
#define NEXT_P_VIDEO_SPAD	(NEXT_SLOT_ID+0x02004180)
#define NEXT_P_EVENT_SPAD	(NEXT_SLOT_ID+0x0200418c)
#define NEXT_P_M2M_SPAD		(NEXT_SLOT_ID+0x020041e0)

/* device registers */
#define NEXT_P_ENET		(NEXT_SLOT_ID_BMAP+0x02006000)
#define NEXT_P_DSP		(NEXT_SLOT_ID_BMAP+0x02008000)
#define NEXT_P_MON		(NEXT_SLOT_ID+0x0200e000)
#define NEXT_P_PRINTER		(NEXT_SLOT_ID+0x0200f000)
#define NEXT_P_DISK		(NEXT_SLOT_ID_BMAP+0x02012000)
#define NEXT_P_SCSI		(NEXT_SLOT_ID_BMAP+0x02014000)
#define NEXT_P_FLOPPY		(NEXT_SLOT_ID_BMAP+0x02014100)
#define NEXT_P_TIMER		(NEXT_SLOT_ID_BMAP+0x02016000)
#define NEXT_P_TIMER_CSR	(NEXT_SLOT_ID_BMAP+0x02016004)
#define NEXT_P_SCC		(NEXT_SLOT_ID_BMAP+0x02018000)
#define NEXT_P_SCC_CLK		(NEXT_SLOT_ID_BMAP+0x02018004)
#define NEXT_P_EVENTC		(NEXT_SLOT_ID_BMAP+0x0201a000)
#define NEXT_P_BMAP		(NEXT_SLOT_ID+0x020c0000)
/* All COLOR_FB registers are 1 byte wide */
#define NEXT_P_C16_DAC_0	(NEXT_SLOT_ID_BMAP+0x02018100)	/* COLOR_FB - RAMDAC */
#define NEXT_P_C16_DAC_1	(NEXT_SLOT_ID_BMAP+0x02018101)
#define NEXT_P_C16_DAC_2	(NEXT_SLOT_ID_BMAP+0x02018102)
#define NEXT_P_C16_DAC_3	(NEXT_SLOT_ID_BMAP+0x02018103)
#define NEXT_P_C16_CMD_REG	(NEXT_SLOT_ID_BMAP+0x02018180)	/* COLOR_FB - CSR */

/* system control registers */
#define NEXT_P_MEMTIMING	(NEXT_SLOT_ID_BMAP+0x02006010)
#define NEXT_P_INTRSTAT		(NEXT_SLOT_ID+0x02007000)
#define NEXT_P_INTRSTAT_CON	0x02007000
/* #define NEXT_P_INTRSTAT_0	(NEXT_SLOT_ID+0x02008000) */
#define NEXT_P_INTRMASK		(NEXT_SLOT_ID+0x02007800)
#define NEXT_P_INTRMASK_CON	0x02007800
/* #define NEXT_P_INTRMASK_0	(NEXT_SLOT_ID+0x0200a000) */
#define NEXT_P_SCR1		(NEXT_SLOT_ID+0x0200c000)
#define NEXT_P_SCR1_CON	0x0200c000
#define NEXT_P_SID		0x0200c800		/* NOT slot-relative */
#define NEXT_P_SCR2		(NEXT_SLOT_ID+0x0200d000)
#define NEXT_P_SCR2_CON	0x0200d000
#define NEXT_P_RMTINT		(NEXT_SLOT_ID+0x0200d800)
#define NEXT_P_BRIGHTNESS	(NEXT_SLOT_ID_BMAP+0x02010000)
#define NEXT_P_DRAM_TIMING	(NEXT_SLOT_ID_BMAP+0x02018190) /* Warp 9C memory ctlr */
#define NEXT_P_VRAM_TIMING	(NEXT_SLOT_ID_BMAP+0x02018198) /* Warp 9C memory ctlr */

/* memory */
#define NEXT_P_MAINMEM		(NEXT_SLOT_ID+0x04000000)
#define NEXT_P_MEMSIZE		0x04000000
#define NEXT_P_VIDEOMEM		(NEXT_SLOT_ID+0x0b000000)
#define NEXT_P_VIDEOSIZE	0x0003a800
#if 0
#define NEXT_P_C16_VIDEOMEM	(NEXT_SLOT_ID+0x06000000)	/* COLOR_FB */
#endif
#define NEXT_P_C16_VIDEOMEM	(0x2c000000)
#define NEXT_P_C16_VIDEOSIZE	0x001D4000		/* COLOR_FB */
#define NEXT_P_WF4VIDEO		(NEXT_SLOT_ID+0x0c000000)	/* w A+B-AB function */
#define NEXT_P_WF3VIDEO		(NEXT_SLOT_ID+0x0d000000)	/* w (1-A)B function */
#define NEXT_P_WF2VIDEO		(NEXT_SLOT_ID+0x0e000000)	/* w ceil(A+B) function */
#define NEXT_P_WF1VIDEO		(NEXT_SLOT_ID+0x0f000000)	/* w AB function */
#define NEXT_P_WF4MEM		(NEXT_SLOT_ID+0x10000000)	/* w A+B-AB function */
#define NEXT_P_WF3MEM		(NEXT_SLOT_ID+0x14000000)	/* w (1-A)B function */
#define NEXT_P_WF2MEM		(NEXT_SLOT_ID+0x18000000)	/* w ceil(A+B) function */
#define NEXT_P_WF1MEM		(NEXT_SLOT_ID+0x1c000000)	/* w AB function */
#define NEXT_NMWF		4			/* # of memory write funcs */

/*
 * Interrupt structure.
 * BASE and BITS define the origin and length of the bit field in the
 * interrupt status/mask register for the particular interrupt level.
 * The first component of the interrupt device name indicates the bit
 * position in the interrupt status and mask registers; the second is the
 * interrupt level; the third is the bit index relative to the start of the
 * bit field.
 */
#define	NEXT_I(l,i,b)	(((b) << 8) | ((l) << 4) | (i))
#define	NEXT_I_INDEX(i)	((i) & 0xf)
#define	NEXT_I_IPL(i)	(((i) >> 4) & 7)
#define	NEXT_I_BIT(i)	( 1 << (((i) >> 8) & 0x1f))

#define	NEXT_I_IPL7_BASE	0
#define	NEXT_I_IPL7_BITS	2
#define	NEXT_I_NMI		NEXT_I(7,0,31)
#define	NEXT_I_PFAIL		NEXT_I(7,1,30)

#define	NEXT_I_IPL6_BASE	2
#define	NEXT_I_IPL6_BITS	12
#define	NEXT_I_TIMER		NEXT_I(6,0,29)
#define	NEXT_I_ENETX_DMA	NEXT_I(6,1,28)
#define	NEXT_I_ENETR_DMA	NEXT_I(6,2,27)
#define	NEXT_I_SCSI_DMA		NEXT_I(6,3,26)
#define	NEXT_I_DISK_DMA		NEXT_I(6,4,25)
#define	NEXT_I_PRINTER_DMA	NEXT_I(6,5,24)
#define	NEXT_I_SOUND_OUT_DMA	NEXT_I(6,6,23)
#define	NEXT_I_SOUND_IN_DMA	NEXT_I(6,7,22)
#define	NEXT_I_SCC_DMA		NEXT_I(6,8,21)
#define	NEXT_I_DSP_DMA		NEXT_I(6,9,20)
#define	NEXT_I_M2R_DMA		NEXT_I(6,10,19)
#define	NEXT_I_R2M_DMA		NEXT_I(6,11,18)

#define	NEXT_I_IPL5_BASE	14
#define	NEXT_I_IPL5_BITS	3
#define	NEXT_I_SCC		NEXT_I(5,0,17)
#define	NEXT_I_REMOTE		NEXT_I(5,1,16)
#define	NEXT_I_BUS		NEXT_I(5,2,15)

#define	NEXT_I_IPL4_BASE	17
#define	NEXT_I_IPL4_BITS	1
#define	NEXT_I_DSP_4		NEXT_I(4,0,14)

#define	NEXT_I_IPL3_BASE	18
#define	NEXT_I_IPL3_BITS	12
#define	NEXT_I_DISK		NEXT_I(3,0,13)
#define	NEXT_I_C16_VIDEO	NEXT_I(3,0,13)	/* COLOR_FB - Steals old ESDI interrupt */
#define	NEXT_I_SCSI		NEXT_I(3,1,12)
#define	NEXT_I_PRINTER		NEXT_I(3,2,11)
#define	NEXT_I_ENETX		NEXT_I(3,3,10)
#define	NEXT_I_ENETR		NEXT_I(3,4,9)
#define	NEXT_I_SOUND_OVRUN	NEXT_I(3,5,8)
#define	NEXT_I_PHONE		NEXT_I(3,6,7)
#define	NEXT_I_DSP_3		NEXT_I(3,7,6)
#define	NEXT_I_VIDEO		NEXT_I(3,8,5)
#define	NEXT_I_MONITOR		NEXT_I(3,9,4)
#define	NEXT_I_KYBD_MOUSE	NEXT_I(3,10,3)
#define	NEXT_I_POWER		NEXT_I(3,11,2)

#define	NEXT_I_IPL2_BASE	30
#define	NEXT_I_IPL2_BITS	1
#define	NEXT_I_SOFTINT1		NEXT_I(2,0,1)

#define	NEXT_I_IPL1_BASE	31
#define	NEXT_I_IPL1_BITS	1
#define	NEXT_I_SOFTINT0		NEXT_I(1,0,0)

/****************************************************************/

/* physical memory sections */
#if 0
#define	ROMBASE		(0x00000000)
#endif

#define	INTIOBASE	(0x02000000)
#define	INTIOTOP	(0x02120000)
#define MONOBASE	(0x0b000000)
#define MONOTOP		(0x0b03a800)
#define COLORBASE	(0x2c000000)
#define COLORTOP	(0x2c1D4000)

#define NEXT_INTR_BITS \
"\20\40NMI\37PFAIL\36TIMER\35ENETX_DMA\34ENETR_DMA\33SCSI_DMA\32DISK_DMA\31PRINTER_DMA\30SOUND_OUT_DMA\27SOUND_IN_DMA\26SCC_DMA\25DSP_DMA\24M2R_DMA\23R2M_DMA\22SCC\21REMOTE\20BUS\17DSP_4\16DISK|C16_VIDEO\15SCSI\14PRINTER\13ENETX\12ENETR\11SOUND_OVRUN\10PHONE\07DSP_3\06VIDEO\05MONITOR\04KYBD_MOUSE\03POWER\02SOFTINT1\01SOFTINT0"

/*
 * Internal IO space:
 *
 * Ranges from 0x400000 to 0x600000 (IIOMAPSIZE).
 *
 * Internal IO space is mapped in the kernel from ``intiobase'' to
 * ``intiolimit'' (defined in locore.s).  Since it is always mapped,
 * conversion between physical and kernel virtual addresses is easy.
 */
#define	IIOV(pa)	((int)(pa)-INTIOBASE+intiobase)
#define	IIOP(va)	((int)(va)-intiobase+INTIOBASE)
#define	IIOMAPSIZE	btoc(INTIOTOP-INTIOBASE)	/* 2mb */

/* mono fb space */
#define	MONOMAPSIZE	btoc(MONOTOP-MONOBASE)	/* who cares */

/* color fb space */
#define	COLORMAPSIZE	btoc(COLORTOP-COLORBASE)	/* who cares */

#endif	/* _MACHINE_CPU_H_ */
