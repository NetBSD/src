/*	$NetBSD: pccreg.h,v 1.5 1996/09/12 04:54:19 thorpej Exp $	*/

/*
 *
 * Copyright (c) 1995 Charles D. Cranor
 * All rights reserved.
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
 *      This product includes software developed by Charles D. Cranor.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * peripheral channel controller (at pa fffe0000)
 */

#define PCC_BASE	0xfffe0000	/* PA of PCC chip space */
#define PCC_CLOCK_OFF	0x0000		/* offset of Mostek clock chip */
#define PCC_RTC_OFF	0x07f8		/* offset of clock registers in MK */
#define PCC_REG_OFF	0x1000		/* offset of PCC chip registers */
#define PCC_LE_OFF	0x1800		/* offset of LANCE ethernet chip */
#define PCC_VME_OFF	0x2000		/* offset of VME chip */
#define PCC_LPT_OFF	0x2800		/* offset of parallel port register */
#define PCC_ZS0_OFF	0x3000		/* offset of first 8530 UART */
#define PCC_ZS1_OFF	0x3800		/* offset of second 8530 UART */
#define PCC_WDSC_OFF	0x4000		/* offset of 33c93 SCSI chip */

#define PCC_PADDR(off)	((void *)(PCC_BASE + (off)))

/*
 * The PCC space is permanently mapped by pmap_bootstrap().  This
 * macro translates PCC offsets into the corresponding KVA.
 */
#define PCC_VADDR(off)	((void *)IIOV(PCC_BASE + (off))) 

struct pcc {
  volatile u_long dma_taddr;		/* dma table address */
  volatile u_long dma_daddr;		/* dma data address */
  volatile u_long dma_bcnt;		/* dma byte count */
  volatile u_long dma_hold;		/* dma data hold register */
  volatile u_short t1_pload;		/* timer1 preload */
  volatile u_short t1_count;		/* timer1 count */
  volatile u_short t2_pload;		/* timer2 preload */
  volatile u_short t2_count;		/* timer2 count */
  volatile u_char t1_int;		/* timer1 interrupt ctrl */
  volatile u_char t1_cr;		/* timer1 ctrl reg */
  volatile u_char t2_int;		/* timer2 interrupt ctrl */
  volatile u_char t2_cr;		/* timer2 ctrl reg */
  volatile u_char acf_int;		/* acfail intr reg */
  volatile u_char dog_int;		/* watchdog intr reg */
  volatile u_char pr_int;		/* printer intr reg */
  volatile u_char pr_cr;		/* printer ctrl */
  volatile u_char dma_int;		/* dma interrupt control */
  volatile u_char dma_csr;		/* dma csr */
  volatile u_char bus_int;		/* bus error interrupt */
  volatile u_char dma_sr;		/* dma status register */
  volatile u_char abrt_int;		/* abort interrupt control reg */
  volatile u_char ta_fcr;		/* table address function code reg */
  volatile u_char zs_int;		/* serial interrupt reg */
  volatile u_char gen_cr;		/* general control register */
  volatile u_char le_int;		/* ethernet interrupt */
  volatile u_char gen_sr;		/* general status */
  volatile u_char scsi_int;		/* scsi interrupt reg */
  volatile u_char slave_ba;		/* slave base addr reg */
  volatile u_char sw1_int;		/* software interrupt #1 cr */
  volatile u_char int_vectr;		/* interrupt base vector register */
  volatile u_char sw2_int;		/* software interrupt #2 cr */
  volatile u_char pcc_rev;		/* revision level */
};


/*
 * points to system's PCC
 */

extern struct pcc *sys_pcc;

/*
 * we lock off our interrupt vector at 0x40.
 */

#define PCC_VECBASE 0x40
#define PCC_NVEC 12

/*
 * vectors we use
 */

#define PCCV_ACFAIL	0
#define PCCV_BERR	1
#define PCCV_ABORT	2
#define PCCV_ZS		3
#define PCCV_LE		4
#define PCCV_SCSIP	5
#define PCCV_SCSID	6
#define PCCV_PRINTER	7
#define PCCV_TIMER1	8
#define PCCV_TIMER2	9
#define PCCV_SOFT1	10
#define PCCV_SOFT2	11

/*
 * enable interrupt
 */

#define PCC_ICLEAR  0x80
#define PCC_IENABLE 0x08

/*
 * interrupt mask
 */

#define PCC_IMASK 0x7

/*
 * clock/timer
 */

#define PCC_TIMERACK 0x80	/* ack intr */
#define PCC_TIMER100HZ 63936	/* load value for 100Hz */
#define PCC_TIMERCLEAR 0x0	/* reset and clear timer */
#define PCC_TIMERSTOP  0x1	/* stop clock, but don't clear it */
#define PCC_TIMERSTART 0x3      /* start timer */

#define pcc_timer_hz2lim(hz)	(65536 - (160000/(hz)))
#define pcc_timer_us2lim(us)	(65536 - (160000/(1000000/(us))))

/*
 * serial control
 */

#define PCC_ZSEXTERN 0x10	/* let PCC supply vector */

/*
 * abort switch
 */

#define PCC_ABORT_IEN	0x08	/* enable interrupt */
#define PCC_ABORT_ABS	0x40	/* current state of switch */
#define PCC_ABORT_ACK	0x80	/* interrupt active; write to ack */

/*
 * general control register
 */

#define PCC_GENCR_IEN	0x10	/* global interrupt enable */
