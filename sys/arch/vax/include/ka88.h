/*	$NetBSD: ka88.h,v 1.5.48.1 2017/12/03 11:36:48 jdolecek Exp $	*/

/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * KA88 system defines, gotten from the
 * VAX 8530/8550/8700/8800 System Maintenance Guide.
 */
#ifndef _VAX_KA88_H_
#define _VAX_KA88_H_

/* Console communication ID fields */
#define KA88_CSA1	0x0100		/* first floppy */
#define KA88_CSA3	0x0200		/* hard drive */
#define KA88_LOCAL	0x0300		/* local console */
#define KA88_CSA2	0x0400		/* second floppy */
#define KA88_REMOTE	0x0600		/* remote console */
#define KA88_DIAGDATA	0x0700		/* */
#define	KA88_CONSMSG	0x0800		/* messages from console */
#define KA88_CONFDATA	0x0900		/* config data from PRO (in) */
#define KA88_CSACMND	0x0900		/* floppy command (out) */
#define KA88_TOY	0x0d00		/* time of year clock */
#define KA88_COMM	0x0f00		/* communication channel */

/* Messages from console */
#define KA88_ENV	0x0000		/* margins passed */
#define KA88_CSA1STAT	0x0010		/* floppy 1 status */
#define KA88_CSA2STAT	0x0020		/* floppy 2 status */
#define KA88_CSA3STAT	0x0050		/* hard drive status */
#define KA88_TOYERR	0x0070		/* time of year clock error */

/* Environment margins passed */
#define KA88_BLOWER	0x0000		/* fan malfunctioning */
#define KA88_YELLOW	0x0001		/* yellow zone passed */
#define KA88_RED	0x0002		/* red zone passed */

/* floppy status */
#define KA88_CSAOK	0x0000		/* floppy command succeeded */

/* config data from PRO */
#define KA88_LEFT	0x0001		/* left CPU available */
#define KA88_SMALL	0x0001		/* small cabinet (8530/8550) */
#define KA88_RIGHT	0x0002		/* right CPU available */
#define KA88_SECONDEN	0x0004		/* secondary enabled */
#define KA88_SINGLE	0x0008		/* single CPU */
#define KA88_LEFTPRIM	0x0040		/* left CPU is primary */
#define KA88_SLOW	0x0080		/* low-speed version */

/* floppy commands */
#define KA88_CS_READ	0x0000		/* read from device */
#define KA88_CS_WRITE	0x0001		/* write to device */

/* communication channel */
#define KA88_REBOOT	0x0002		/* reboot this processor */
#define KA88_CLRW	0x0003		/* clear warm start flag */
#define KA88_CLRC	0x0004		/* clear cold start flag */
#define KA88_REBOOT2ND	0x0005		/* reboot other processor */
#define KA88_TOYREAD	0x0008		/* request toy clock */
#define KA88_TOYWRITE	0x0009		/* notify of intent to write toy */
#define KA88_GETCONF	0x000d		/* request configuration info */

/* IPR defines */
#define PR_NICTRL	128		/* NMI Interrupt control */
#define	NICTRL_DEV0	0x80		/* Device 0 interrupt enable */
#define	NICTRL_DEV1	0x40		/* Device 1 interrupt enable */
#define	NICTRL_MNF	0x20		/* memory interrupt/NMI fault */

#define PR_INOP		129		/* Interrupt other processor */
#define	INOP_IOP	1

#define PR_NMIFSR	130		/* NMI fault summary */
#define PR_NMISIL	131		/* NMI silo data */                   
#define PR_NMIEAR	132		/* NMI error address */           
#define PR_COR		133		/* Cache on register */           
#define PR_REVR1	134		/* Rev register 1 */              
#define PR_REVR2	135		/* Rev register 2 */

/* NBIA defines */
#define	NBIA_REGS(nbianr)	(0x20080000 + ((nbianr) << 26))

#define	NBIA_CSR0	0
#define	CSR0_PARERR	0x8000		/* NBI parity error */
#define	CSR0_LOOP	0x10000		/* loopback */
#define	CSR0_NBIIE	0x100000	/* NBI interrupt enable */

#define	NBIA_CSR1	4
#define NBIA_BR4VR	16
#define NBIA_BR5VR	20
#define NBIA_BR6VR	24
#define NBIA_BR7VR	28
/*
 * This is mostly fake, but simple way to get autoconf work.
 * slot 0-9:	BI bus adapters
 *       10:	Memory controller
 *    20-23:	CPUs.
 */
struct nmi_attach_args {
	const char *na_type;
	int na_slot;
	bus_space_tag_t na_iot;
	bus_dma_tag_t na_dmat;
};

#ifdef _KERNEL
int ka88_confdata;
#endif

#endif /* _VAX_KA88_H_ */
