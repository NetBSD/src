/*	$NetBSD: icureg.h,v 1.5.4.2 2002/02/28 04:10:04 nathanw Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura. All rights reserved.
 * Copyright (c) 1999-2001 SATO Kazumi. All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

/*
 *	ICU (Interrupt Control UNIT) Registers definitions
 *		start 0x0B000080 (vr4102/4111/4121)
 *		start 0x0F000080 (vr4122)
 */
#include "opt_vr41xx.h"
#include <hpcmips/vr/vrcpudef.h>

#define ICU_NO_REG_W		0xffffffff	/* no register */


/* SYSINT1 & MSYSINT1 */
#define SYSINT1_REG_W		0x000	/* Level1 System intr reg 1 */
#define MSYSINT1_REG_W		0x00c	/* Level1 Mask System intr reg 1 */

#define SYSINT1_INT15			(1<<15)
#define SYSINT1_INT14			(1<<14)
#define SYSINT1_INT13			(1<<13)
#define SYSINT1_DOZEPIU			(1<<13)	/* PIU intr during Suspend */
#define SYSINT1_INT12			(1<<12)
#define SYSINT1_CLKRUN			(1<<12) /* CLKRUN intr (=vr4122) */
#define SYSINT1_INT11			(1<<11)
#define SYSINT1_SOFT			(1<<11)	/* Software intr */
#define SYSINT1_INT10			(1<<10)
#define SYSINT1_WRBERR			(1<<10)	/* Bus error intr (4102 <=,<= 4121)*/
#define SYSINT1_INT9			(1<<9)
#define SYSINT1_SIU			(1<<9)	/* SIU intr */
#define SYSINT1_INT8			(1<<8)
#define SYSINT1_GIU			(1<<8)	/* GIU intr */
#define SYSINT1_INT7			(1<<7)
#define SYSINT1_KIU			(1<<7)	/* KIU intr (4102 <=,<= 4121)*/
#define SYSINT1_INT6			(1<<6)
#define SYSINT1_AIU			(1<<6)	/* AIU intr (4102 <=,<= 4121)*/
#define SYSINT1_INT5			(1<<5)
#define SYSINT1_PIU			(1<<5)	/* PIU intr (4102 <=,<= 4121)*/
#define SYSINT1_INT4			(1<<4)
#define SYSINT1_INT3			(1<<3)
#define SYSINT1_ETIMER			(1<<3)	/* ETIMER intr */
#define SYSINT1_INT2			(1<<2)
#define SYSINT1_RTCL1			(1<<2)	/* RTClong1 intr */
#define SYSINT1_INT1			(1<<1)
#define SYSINT1_POWER			(1<<1)	/* PowerSW intr */
#define SYSINT1_INT0			(1<<0)
#define SYSINT1_BAT			(1<<0)	/* Battery intr */


/* PIUINT & MPIUINT */
#define ICUPIUINT_REG_W		0x002	/* Level2 PIU intr reg */
#define MPIUINT_REG_W		0x00e	/* Level2 Mask PIU intr reg */

#define		PIUINT_PADCMD		(1<<6)	/* PIU command scan intr */
#define		PIUINT_PADADP		(1<<5)	/* PIU AD port scan intr */
#define		PIUINT_PADPAGE1		(1<<4)	/* PIU data page 1 intr */
#define		PIUINT_PADPAGE0		(1<<3)	/* PIU data page 0 intr */
#define		PIUINT_PADLOST		(1<<2)	/* A/D data timeout intr */
#define		PIUINT_PENCHG		(1)	/* Touch Panel contact intr */


/* AIUINT & MAIUINT */
#define VR4102_AIUINT_REG_W	0x004	/* Level2 AIU intr reg */	
#define VR4102_MAIUINT_REG_W	0x010	/* Level2 Mask AIU intr reg */
#define VR4122_AIUINT_REG_W	ICU_NO_REG_W	/* Level2 AIU intr reg */	
#define VR4122_MAIUINT_REG_W	ICU_NO_REG_W	/* Level2 Mask AIU intr reg */
#define VR4181_AIUINT_REG_W	0x004	/* Level2 AIU intr reg */	
#define VR4181_MAIUINT_REG_W	0x010	/* Level2 Mask AIU intr reg */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define AIUINT_REG_W		VR4102_AIUINT_REG_W
#define MAIUINT_REG_W		VR4102_MAIUINT_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define AIUINT_REG_W		VR4122_AIUINT_REG_W
#define MAIUINT_REG_W		VR4122_MAIUINT_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define AIUINT_REG_W		VR4181_AIUINT_REG_W
#define MAIUINT_REG_W		VR4181_MAIUINT_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		AIUINT_INTMEND		(1<<11)	/* Audio input DMA buffer 2 page */
#define		AIUINT_INTM		(1<<10)	/* Audio input DMA buffer 1 page */
#define		AIUINT_INTMIDLE		(1<<9)	/* Audio input idle intr */
#define		AIUINT_INTMST		(1<<8)	/* Audio input receive completion intr */
#define		AIUINT_INTSEND		(1<<3)	/* Audio output buffer 2 page */
#define		AIUINT_INTS		(1<<2)	/* Audio output buffer 1 page */
#define		AIUINT_INTSIDLE		(1<<1)	/* Audio output idle intr */


/* KIUINT & MKIUINT */
#define VR4102_KIUINT_REG_W	0x006	/* Level2 KIU intr reg */
#define VR4102_MKIUINT_REG_W	0x012	/* Level2 Mask KIU intr reg */
#define VR4122_KIUINT_REG_W	ICU_NO_REG_W	/* Level2 KIU intr reg */
#define VR4122_MKIUINT_REG_W	ICU_NO_REG_W	/* Level2 Mask KIU intr reg */
#define VR4181_KIUINT_REG_W	0x118	/* Level2 KIU intr reg */
#define VR4181_MKIUINT_REG_W	0x012	/* Level2 Mask KIU intr reg */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define KIUINT_REG_W		VR4102_KIUINT_REG_W
#define MKIUINT_REG_W		VR4102_MKIUINT_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define KIUINT_REG_W		VR4122_KIUINT_REG_W
#define MKIUINT_REG_W		VR4122_MKIUINT_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define KIUINT_REG_W		VR4181_KIUINT_REG_W
#define MKIUINT_REG_W		VR4181_MKIUINT_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		KIUINT_KDATLOST		(1<<2)	/* Key scan data lost */
#define		KIUINT_KDATRDY		(1<<1)	/* Key scan data complete */
#define		KIUINT_SCANINT		(1)	/* Key input detect intr */


/* GIUINTL & MGIUINTL */
#define VR4102_GIUINT_L_REG_W	0x008	/* Level2 GIU intr reg Low */
#define VR4102_MGIUINT_L_REG_W	0x014	/* Level2 Mask GIU intr reg Low */
#define VR4122_GIUINT_L_REG_W	0x008	/* Level2 GIU intr reg Low */
#define VR4122_MGIUINT_L_REG_W	0x014	/* Level2 Mask GIU intr reg Low */
#define VR4181_GIUINT_L_REG_W	ICU_NO_REG_W	/* Level2 GIU intr reg Low */
#define VR4181_MGIUINT_L_REG_W	ICU_NO_REG_W	/* Level2 Mask GIU intr reg Low */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define GIUINT_L_REG_W		VR4102_GIUINT_L_REG_W
#define MGIUINT_L_REG_W		VR4102_MGIUINT_L_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define GIUINT_L_REG_W		VR4122_GIUINT_L_REG_W
#define MGIUINT_L_REG_W		VR4122_MGIUINT_L_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define GIUINT_L_REG_W		VR4181_GIUINT_L_REG_W
#define MGIUINT_L_REG_W		VR4181_MGIUINT_L_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		GIUINT_GPIO15		(1<<15)	/* GPIO 15 */
#define		GIUINT_GPIO14		(1<<14)	/* GPIO 14 */
#define		GIUINT_GPIO13		(1<<13)	/* GPIO 13 */
#define		GIUINT_GPIO12		(1<<12)	/* GPIO 12 */
#define		GIUINT_GPIO11		(1<<11)	/* GPIO 11 */
#define		GIUINT_GPIO10		(1<<10)	/* GPIO 10 */
#define		GIUINT_GPIO9		(1<<9)	/* GPIO 9 */
#define		GIUINT_GPIO8		(1<<8)	/* GPIO 8 */
#define		GIUINT_GPIO7		(1<<7)	/* GPIO 7 */
#define		GIUINT_GPIO6		(1<<6)	/* GPIO 6 */
#define		GIUINT_GPIO5		(1<<5)	/* GPIO 5 */
#define		GIUINT_GPIO4		(1<<4)	/* GPIO 4 */
#define		GIUINT_GPIO3		(1<<3)	/* GPIO 3 */
#define		GIUINT_GPIO2		(1<<2)	/* GPIO 2 */
#define		GIUINT_GPIO1		(1<<1)	/* GPIO 1 */
#define		GIUINT_GPIO0		(1)	/* GPIO 0 */


/* DSIUINT & MDSIUINT */
#define VR4102_DSIUINT_REG_W		0x00a	/* Level2 DSIU intr reg */
#define VR4102_MDSIUINT_REG_W		0x016	/* Level2 Mask DSIU intr reg */
#define VR4122_DSIUINT_REG_W		0x00a	/* Level2 DSIU intr reg */
#define VR4122_MDSIUINT_REG_W		0x016	/* Level2 Mask DSIU intr reg */
#define VR4181_DSIUINT_REG_W		ICU_NO_REG_W	/* Level2 DSIU intr reg */
#define VR4181_MDSIUINT_REG_W		ICU_NO_REG_W	/* Level2 Mask DSIU intr reg */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define DSIUINT_REG_W		VR4102_DSIUINT_REG_W
#define MDSIUINT_REG_W		VR4102_MDSIUINT_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define DSIUINT_REG_W		VR4122_DSIUINT_REG_W
#define MDSIUINT_REG_W		VR4122_MDSIUINT_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define DSIUINT_REG_W		VR4181_DSIUINT_REG_W
#define MDSIUINT_REG_W		VR4181_MDSIUINT_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		DSIUINT_DCTS		(1<<11)	/* DCTS# change */
#define		DSIUINT_SER0		(1<<10)	/* Debug serial receive error */
#define		DSIUINT_SR0		(1<<9)	/* Debug serial receive */
#define		DSIUINT_ST0		(1<<8)	/* Debug serial transmit */


/* NMI */
#define NMI_REG_W		0x018	/* NMI reg */

#define		LOWBATT_NMIORINT	(1)	/* Low battery type */
#define		LOWBATT_INT0		(1)	/* Low battery int 0 */
#define		LOWBATT_NMI		(0)	/* Low battery NMI */


/* SOFTINT */
#define SOFTINT_REG_W		0x01a	/* Software intr reg */

#define		SOFTINT_MASK3		(1<<3)	/* Softint3 mask */
#define		SOFTINT_SET3		(1<<3)	/* Softint3 set */
#define		SOFTINT_CLEAR3		(0<<3)	/* Softint3 clear */

#define		SOFTINT_MASK2		(1<<2)	/* Softint2 mask */
#define		SOFTINT_SET2		(1<<2)	/* Softint2 set */
#define		SOFTINT_CLEAR2		(0<<2)	/* Softint2 clear */

#define		SOFTINT_MASK1		(1<<1)	/* Softint1 mask */
#define		SOFTINT_SET1		(1<<1)	/* Softint1 set */
#define		SOFTINT_CLEAR1		(0<<1)	/* Softint1 clear */

#define		SOFTINT_MASK0		(1)	/* Softint0 mask */
#define		SOFTINT_SET0		(1)	/* Softint0 set */
#define		SOFTINT_CLEAR0		(0)	/* Softint0 clear */


/* SYSINT2 & MSYSINT2 */
#define VR4102_SYSINT2_REG_W	0x180	/* Level1 System intr reg 2 */
#define VR4102_MSYSINT2_REG_W	0x186	/* Level1 Mask System intr reg 2 */
#define VR4122_SYSINT2_REG_W	0x020	/* Level1 System intr reg 2 */
#define VR4122_MSYSINT2_REG_W	0x026	/* Level1 Mask System intr reg 2 */
#define VR4181_SYSINT2_REG_W	0x180	/* Level1 System intr reg 2 */
#define VR4181_MSYSINT2_REG_W	0x186	/* Level1 Mask System intr reg 2 */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define SYSINT2_REG_W		VR4102_SYSINT2_REG_W
#define MSYSINT2_REG_W		VR4102_MSYSINT2_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define SYSINT2_REG_W		VR4122_SYSINT2_REG_W
#define MSYSINT2_REG_W		VR4122_MSYSINT2_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define SYSINT2_REG_W		VR4181_SYSINT2_REG_W
#define MSYSINT2_REG_W		VR4181_MSYSINT2_REG_W
#endif /* VRGROUP_4181 */
#endif

#define SYSINT2_INT31			(1<<15)
#define SYSINT2_INT30			(1<<14)
#define SYSINT2_INT29			(1<<13)
#define SYSINT2_INT28			(1<<12)
#define SYSINT2_INT27			(1<<11)
#define SYSINT2_INT26			(1<<10)
#define SYSINT2_INT25			(1<<9)
#define SYSINT2_BCU			(1<<9)  /* BCU intr (=vr4122) */
#define SYSINT2_INT24			(1<<8)
#define SYSINT2_CSI			(1<<8)  /* CSI intr (=vr4122) */
#define SYSINT2_INT23			(1<<7)
#define SYSINT2_SCU			(1<<7)	/* SCU intr (=vr4122) */
#define SYSINT2_INT22			(1<<6)
#define SYSINT2_PCI			(1<<6)	/* PCI intr (=vr4122) */
#define SYSINT2_LCD			(1<<6)	/* LCD intr (=vr4181) */
#define SYSINT2_DSIU			(1<<5)	/* DSUI intr */
#define SYSINT2_DCU81			(1<<5)	/* DCU intr (=4181) */
#define SYSINT2_FIR			(1<<4)	/* FIR intr */
#define SYSINT2_TCLK			(1<<3)	/* TClock Counter intr */
#define SYSINT2_CSI81			(1<<3)	/* CSI intr (=4181) */
#define SYSINT2_HSP			(1<<2)	/* HSP intr (4122>=4102)*/
#define SYSINT2_ECU			(1<<2)	/* EUC intr (=4181)*/
#define SYSINT2_LED			(1<<1)	/* LED intr */
#define SYSINT2_RTCL2			(1<<0)	/* RTCLong2 intr */


/* GIUINTH & MGIUINTH */
#define VR4102_GIUINT_H_REG_W	0x182	/* Level2 GIU intr reg High */
#define VR4102_MGIUINT_H_REG_W	0x188	/* Level2 Mask GIU intr reg High */
#define VR4122_GIUINT_H_REG_W	0x022	/* Level2 GIU intr reg High */
#define VR4122_MGIUINT_H_REG_W	0x028	/* Level2 Mask GIU intr reg High */
#define VR4181_GIUINT_H_REG_W	ICU_NO_REG_W	/* Level2 GIU intr reg High */
#define VR4181_MGIUINT_H_REG_W	ICU_NO_REG_W	/* Level2 Mask GIU intr reg High */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define GIUINT_H_REG_W		VR4102_GIUINT_H_REG_W
#define MGIUINT_H_REG_W		VR4102_MGIUINT_H_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define GIUINT_H_REG_W		VR4122_GIUINT_H_REG_W
#define MGIUINT_H_REG_W		VR4122_MGIUINT_H_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define GIUINT_H_REG_W		VR4181_GIUINT_H_REG_W
#define MGIUINT_H_REG_W		VR4181_MGIUINT_H_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		GIUINT_GPIO31		(1<<15)	/* GPIO 31 */
#define		GIUINT_GPIO30		(1<<14)	/* GPIO 30 */
#define		GIUINT_GPIO29		(1<<13)	/* GPIO 29 */
#define		GIUINT_GPIO28		(1<<12)	/* GPIO 28 */
#define		GIUINT_GPIO27		(1<<11)	/* GPIO 27 */
#define		GIUINT_GPIO26		(1<<10)	/* GPIO 26 */
#define		GIUINT_GPIO25		(1<<9)	/* GPIO 25 */
#define		GIUINT_GPIO24		(1<<8)	/* GPIO 24 */
#define		GIUINT_GPIO23		(1<<7)	/* GPIO 23 */
#define		GIUINT_GPIO22		(1<<6)	/* GPIO 22 */
#define		GIUINT_GPIO21		(1<<5)	/* GPIO 21 */
#define		GIUINT_GPIO20		(1<<4)	/* GPIO 20 */
#define		GIUINT_GPIO19		(1<<3)	/* GPIO 19 */
#define		GIUINT_GPIO18		(1<<2)	/* GPIO 18 */
#define		GIUINT_GPIO17		(1<<1)	/* GPIO 17 */
#define		GIUINT_GPIO16		(1)	/* GPIO 16 */


/* FIRINT & MFIRINT */
#define VR4102_FIRINT_REG_W	0x184	/* Level2 FIR intr reg */
#define VR4102_MFIRINT_REG_W	0x18a	/* Level2 Mask FIR intr reg */
#define VR4122_FIRINT_REG_W	0x024	/* Level2 FIR intr reg */
#define VR4122_MFIRINT_REG_W	0x02a	/* Level2 Mask FIR intr reg */
#define VR4181_FIRINT_REG_W	ICU_NO_REG_W	/* Level2 FIR intr reg */
#define VR4181_MFIRINT_REG_W	ICU_NO_REG_W	/* Level2 Mask FIR intr reg */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define FIRINT_REG_W		VR4102_FIRINT_REG_W
#define MFIRINT_REG_W		VR4102_MFIRINT_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define FIRINT_REG_W		VR4122_FIRINT_REG_W
#define MFIRINT_REG_W		VR4122_MFIRINT_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define FIRINT_REG_W		VR4181_FIRINT_REG_W
#define MFIRINT_REG_W		VR4181_MFIRINT_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		FIRINT_FIR		(1<<4)	/* FIR intr */
#define		FIRINT_RECV2		(1<<3)	/* FIR DMA buf recv buffer2 */
#define		FIRINT_TRNS2		(1<<2)	/* FIR DMA buf transmit buffer2 */
#define		FIRINT_RECV1		(1<<1)	/* FIR DMA buf recv buffer1 */
#define		FIRINT_TRNS1		(1)	/* FIR DMA buf transmit buffer1 */


/* PCIINT & MPCIINT */
#define VR4102_PCIINT_REG_W	ICU_NO_REG_W	/* Level2 PCI intr reg */
#define VR4102_MPCIINT_REG_W	ICU_NO_REG_W	/* Level2 PCI intr mask */
#define VR4122_PCIINT_REG_W	0x2c	/* Level2 PCI intr reg */
#define VR4122_MPCIINT_REG_W	0x32	/* Level2 PCI intr mask */
#define VR4181_PCIINT_REG_W	ICU_NO_REG_W	/* Level2 PCI intr reg */
#define VR4181_MPCIINT_REG_W	ICU_NO_REG_W	/* Level2 PCI intr mask */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define PCIINT_REG_W		VR4102_PCIINT_REG_W
#define MPCIINT_REG_W		VR4102_MPCIINT_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define PCIINT_REG_W		VR4122_PCIINT_REG_W
#define MPCIINT_REG_W		VR4122_MPCIINT_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define PCIINT_REG_W		VR4181_PCIINT_REG_W
#define MPCIINT_REG_W		VR4181_MPCIINT_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		PCIINT_INT0		(1)	/* PCI INT 0 */


/* SCUINT & MSCUINT */
#define VR4102_SCUINT_REG_W	ICU_NO_REG_W	/* Level2 SCU intr reg */
#define VR4102_MSCUINT_REG_W	ICU_NO_REG_W	/* Level2 SCU intr mask */
#define VR4122_SCUINT_REG_W	0x2e	/* Level2 SCU intr reg */
#define VR4122_MSCUINT_REG_W	0x34	/* Level2 SCU intr mask */
#define VR4181_SCUINT_REG_W	ICU_NO_REG_W	/* Level2 SCU intr reg */
#define VR4181_MSCUINT_REG_W	ICU_NO_REG_W	/* Level2 SCU intr mask */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define SCUINT_REG_W		VR4102_SCUINT_REG_W
#define MSCUINT_REG_W		VR4102_MSCUINT_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define SCUINT_REG_W		VR4122_SCUINT_REG_W
#define MSCUINT_REG_W		VR4122_MSCUINT_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define SCUINT_REG_W		VR4181_SCUINT_REG_W
#define MSCUINT_REG_W		VR4181_MSCUINT_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		SCUINT_INT0		(1)	/* SCU INT 0 */


/* CSIINT & MCSIINT */
#define VR4102_CSIINT_REG_W	ICU_NO_REG_W	/* Level2 CSI intr reg */
#define VR4102_MCSIINT_REG_W	ICU_NO_REG_W	/* Level2 CSI intr mask */
#define VR4122_CSIINT_REG_W	0x30	/* Level2 CSI intr reg */
#define VR4122_MCSIINT_REG_W	0x36	/* Level2 CSI intr mask */
#define VR4181_CSIINT_REG_W	ICU_NO_REG_W	/* Level2 CSI intr reg */
#define VR4181_MCSIINT_REG_W	ICU_NO_REG_W	/* Level2 CSI intr mask */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define CSIINT_REG_W		VR4102_CSIINT_REG_W
#define MCSIINT_REG_W		VR4102_MCSIINT_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define CSIINT_REG_W		VR4122_CSIINT_REG_W
#define MCSIINT_REG_W		VR4122_MCSIINT_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define CSIINT_REG_W		VR4181_CSIINT_REG_W
#define MCSIINT_REG_W		VR4181_MCSIINT_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		CSIINT_TRPAGE2		(1<<6)	/* DMA send page 2 intr */
#define		CSIINT_TRPAGE1		(1<<5)	/* DMA send page 1 intr */
#define		CSIINT_TREND		(1<<4)	/* send every data intr */
#define		CSIINT_TREMPTY		(1<<3)	/* send FIFO empty intr */
#define		CSIINT_RCPAGE2		(1<<2)	/* DMA recv page 2 intr */
#define		CSIINT_RCPAGE1		(1<<1)	/* DMA recv page 1 intr */
#define		CSIINT_RCOVER		(1)	/* recv FIFO overrun intr */


/* BCUINT & MBCUINT */
#define VR4102_BCUINT_REG_W	ICU_NO_REG_W	/* Level2 BCU intr reg */
#define VR4102_MBCUINT_REG_W	ICU_NO_REG_W	/* Level2 BCU intr mask */
#define VR4122_BCUINT_REG_W	0x38	/* Level2 BCU intr reg */
#define VR4122_MBCUINT_REG_W	0x3a	/* Level2 BCU intr mask */
#define VR4181_BCUINT_REG_W	ICU_NO_REG_W	/* Level2 BCU intr reg */
#define VR4181_MBCUINT_REG_W	ICU_NO_REG_W	/* Level2 BCU intr mask */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define BCUINT_REG_W		VR4102_BCUINT_REG_W
#define MBCUINT_REG_W		VR4102_MBCUINT_REG_W
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define BCUINT_REG_W		VR4122_BCUINT_REG_W
#define MBCUINT_REG_W		VR4122_MBCUINT_REG_W
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define BCUINT_REG_W		VR4181_BCUINT_REG_W
#define MBCUINT_REG_W		VR4181_MBCUINT_REG_W
#endif /* VRGROUP_4181 */
#endif

#define		BCUINT_INT		(1)	/* BCU INT */

/* END icureg.h */
