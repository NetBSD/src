/* $NetBSD: intrcnt.h,v 1.7.2.1 1997/06/01 04:12:20 cgd Exp $ */

/*
 * Copyright Notice:
 *
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 *
 * License:
 *
 * This License applies to this software ("Software"), created
 * by Christopher G. Demetriou ("Author").
 *
 * You may use, copy, modify and redistribute this Software without
 * charge, in either source code form, binary form, or both, on the
 * following conditions:
 *
 * 1.  (a) Binary code: (i) a complete copy of the above copyright notice
 * must be included within each copy of the Software in binary code form,
 * and (ii) a complete copy of the above copyright notice and all terms
 * of this License as presented here must be included within each copy of
 * all documentation accompanying or associated with binary code, in any
 * medium, along with a list of the software modules to which the license
 * applies.
 *
 * (b) Source Code: A complete copy of the above copyright notice and all
 * terms of this License as presented here must be included within: (i)
 * each copy of the Software in source code form, and (ii) each copy of
 * all accompanying or associated documentation, in any medium.
 *
 * 2. The following Acknowledgment must be used in communications
 * involving the Software as described below:
 *
 *      This product includes software developed by
 *      Christopher G. Demetriou for the NetBSD Project.
 *
 * The Acknowledgment must be conspicuously and completely displayed
 * whenever the Software, or any software, products or systems containing
 * the Software, are mentioned in advertising, marketing, informational
 * or publicity materials of any kind, whether in print, electronic or
 * other media (except for information provided to support use of
 * products containing the Software by existing users or customers).
 *
 * 3. The name of the Author may not be used to endorse or promote
 * products derived from this Software without specific prior written
 * permission (conditions (1) and (2) above are not considered
 * endorsement or promotion).
 *
 * 4.  This license applies to: (a) all copies of the Software, whether
 * partial or whole, original or modified, and (b) your actions, and the
 * actions of all those who may act on your behalf.  All uses not
 * expressly permitted are reserved to the Author.
 *
 * 5.  Disclaimer.  THIS SOFTWARE IS MADE AVAILABLE BY THE AUTHOR TO THE
 * PUBLIC FOR FREE AND "AS IS.''  ALL USERS OF THIS FREE SOFTWARE ARE
 * SOLELY AND ENTIRELY RESPONSIBLE FOR THEIR OWN CHOICE AND USE OF THIS
 * SOFTWARE FOR THEIR OWN PURPOSES.  BY USING THIS SOFTWARE, EACH USER
 * AGREES THAT THE AUTHOR SHALL NOT BE LIABLE FOR DAMAGES OF ANY KIND IN
 * RELATION TO ITS USE OR PERFORMANCE.
 *
 * 6.  If you have a special need for a change in one or more of these
 * license conditions, please contact the Author via electronic mail to
 *
 *     cgd@NetBSD.ORG
 *
 * or via the contact information on
 *
 *     http://www.NetBSD.ORG/People/Pages/cgd.html
 */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#define	INTRNAMES_DEFINITION						\
/* 0x00 */	ASCIZ "clock";						\
		ASCIZ "isa irq 0";					\
		ASCIZ "isa irq 1";					\
		ASCIZ "isa irq 2";					\
		ASCIZ "isa irq 3";					\
		ASCIZ "isa irq 4";					\
		ASCIZ "isa irq 5";					\
		ASCIZ "isa irq 6";					\
		ASCIZ "isa irq 7";					\
		ASCIZ "isa irq 8";					\
		ASCIZ "isa irq 9";					\
		ASCIZ "isa irq 10";					\
		ASCIZ "isa irq 11";					\
		ASCIZ "isa irq 12";					\
		ASCIZ "isa irq 13";					\
		ASCIZ "isa irq 14";					\
/* 0x10 */	ASCIZ "isa irq 15";					\
		ASCIZ "kn20aa irq 0";					\
		ASCIZ "kn20aa irq 1";					\
		ASCIZ "kn20aa irq 2";					\
		ASCIZ "kn20aa irq 3";					\
		ASCIZ "kn20aa irq 4";					\
		ASCIZ "kn20aa irq 5";					\
		ASCIZ "kn20aa irq 6";					\
		ASCIZ "kn20aa irq 7";					\
		ASCIZ "kn20aa irq 8";					\
		ASCIZ "kn20aa irq 9";					\
		ASCIZ "kn20aa irq 10";					\
		ASCIZ "kn20aa irq 11";					\
		ASCIZ "kn20aa irq 12";					\
		ASCIZ "kn20aa irq 13";					\
		ASCIZ "kn20aa irq 14";					\
/* 0x20 */	ASCIZ "kn20aa irq 15";					\
		ASCIZ "kn20aa irq 16";					\
		ASCIZ "kn20aa irq 17";					\
		ASCIZ "kn20aa irq 18";					\
		ASCIZ "kn20aa irq 19";					\
		ASCIZ "kn20aa irq 20";					\
		ASCIZ "kn20aa irq 21";					\
		ASCIZ "kn20aa irq 22";					\
		ASCIZ "kn20aa irq 23";					\
		ASCIZ "kn20aa irq 24";					\
		ASCIZ "kn20aa irq 25";					\
		ASCIZ "kn20aa irq 26";					\
		ASCIZ "kn20aa irq 27";					\
		ASCIZ "kn20aa irq 28";					\
		ASCIZ "kn20aa irq 29";					\
		ASCIZ "kn20aa irq 30";					\
/* 0x30 */	ASCIZ "kn20aa irq 31";					\
		ASCIZ "kn15 tc slot 0";					\
		ASCIZ "kn15 tc slot 1";					\
		ASCIZ "kn15 tc slot 2";					\
		ASCIZ "kn15 tc slot 3";					\
		ASCIZ "kn15 tc slot 4";					\
		ASCIZ "kn15 tc slot 5";					\
		ASCIZ "kn15 tcds";					\
		ASCIZ "kn15 ioasic";					\
		ASCIZ "kn15 sfb";					\
		ASCIZ "kn16 tc slot 0";					\
		ASCIZ "kn16 tc slot 1";					\
		ASCIZ "kn16 tcds";					\
		ASCIZ "kn16 ioasic";					\
		ASCIZ "kn16 sfb";					\
		ASCIZ "tcds esp 0";					\
/* 0x40 */	ASCIZ "tcds esp 1";					\
		ASCIZ "ioasic le";					\
		ASCIZ "ioasic scc 0";					\
		ASCIZ "ioasic scc 1";					\
		ASCIZ "ioasic am79c30";					\
		ASCIZ "eb164 irq 0";					\
		ASCIZ "eb164 irq 1";					\
		ASCIZ "eb164 irq 2";					\
		ASCIZ "eb164 irq 3";					\
		ASCIZ "eb164 irq 4";					\
		ASCIZ "eb164 irq 5";					\
		ASCIZ "eb164 irq 6";					\
		ASCIZ "eb164 irq 7";					\
		ASCIZ "eb164 irq 8";					\
		ASCIZ "eb164 irq 9";					\
		ASCIZ "eb164 irq 10";					\
/* 0x50 */	ASCIZ "eb164 irq 11";					\
		ASCIZ "eb164 irq 12";					\
		ASCIZ "eb164 irq 13";					\
		ASCIZ "eb164 irq 14";					\
		ASCIZ "eb164 irq 15";					\
		ASCIZ "eb164 irq 16";					\
		ASCIZ "eb164 irq 17";					\
		ASCIZ "eb164 irq 18";					\
		ASCIZ "eb164 irq 19";					\
		ASCIZ "eb164 irq 20";					\
		ASCIZ "eb164 irq 21";					\
		ASCIZ "eb164 irq 22";					\
		ASCIZ "eb164 irq 23";					\
		ASCIZ "eb64+ irq 0";					\
		ASCIZ "eb64+ irq 1";					\
		ASCIZ "eb64+ irq 2";					\
/* 0x60 */	ASCIZ "eb64+ irq 3";					\
		ASCIZ "eb64+ irq 4";					\
		ASCIZ "eb64+ irq 5";					\
		ASCIZ "eb64+ irq 6";					\
		ASCIZ "eb64+ irq 7";					\
		ASCIZ "eb64+ irq 8";					\
		ASCIZ "eb64+ irq 9";					\
		ASCIZ "eb64+ irq 10";					\
		ASCIZ "eb64+ irq 11";					\
		ASCIZ "eb64+ irq 12";					\
		ASCIZ "eb64+ irq 13";					\
		ASCIZ "eb64+ irq 14";					\
		ASCIZ "eb64+ irq 15";					\
		ASCIZ "eb64+ irq 16";					\
		ASCIZ "eb64+ irq 17";					\
		ASCIZ "eb64+ irq 18";					\
/* 0x70 */	ASCIZ "eb64+ irq 19";					\
		ASCIZ "eb64+ irq 20";					\
		ASCIZ "eb64+ irq 21";					\
		ASCIZ "eb64+ irq 22";					\
		ASCIZ "eb64+ irq 23";					\
		ASCIZ "eb64+ irq 24";					\
		ASCIZ "eb64+ irq 25";					\
		ASCIZ "eb64+ irq 26";					\
		ASCIZ "eb64+ irq 27";					\
		ASCIZ "eb64+ irq 28";					\
		ASCIZ "eb64+ irq 29";					\
		ASCIZ "eb64+ irq 30";					\
		ASCIZ "eb64+ irq 31";

#define INTRCNT_DEFINITION						\
/* 0x00 */	.quad 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;	\
/* 0x10 */	.quad 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;	\
/* 0x20 */	.quad 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;	\
/* 0x30 */	.quad 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;	\
/* 0x40 */	.quad 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;	\
/* 0x50 */	.quad 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;	\
/* 0x60 */	.quad 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;	\
/* 0x70 */	.quad 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0;

#define	INTRCNT_CLOCK		0
#define	INTRCNT_ISA_IRQ		(INTRCNT_CLOCK + 1)
#define	INTRCNT_ISA_IRQ_LEN	16
#define	INTRCNT_KN20AA_IRQ	(INTRCNT_ISA_IRQ + INTRCNT_ISA_IRQ_LEN)
#define	INTRCNT_KN20AA_IRQ_LEN	32
#define	INTRCNT_KN15		(INTRCNT_KN20AA_IRQ + INTRCNT_KN20AA_IRQ_LEN)
#define	INTRCNT_KN15_LEN	9
#define	INTRCNT_KN16		(INTRCNT_KN15 + INTRCNT_KN15_LEN)
#define	INTRCNT_KN16_LEN	5
#define	INTRCNT_TCDS		(INTRCNT_KN16 + INTRCNT_KN16_LEN)
#define	INTRCNT_TCDS_LEN	2
#define	INTRCNT_IOASIC		(INTRCNT_TCDS + INTRCNT_TCDS_LEN)
#define	INTRCNT_IOASIC_LEN	4
#define	INTRCNT_EB164_IRQ	(INTRCNT_IOASIC + INTRCNT_IOASIC_LEN)
#define	INTRCNT_EB164_IRQ_LEN	24
#define	INTRCNT_EB64PLUS_IRQ	(INTRCNT_EB164_IRQ + INTRCNT_EB164_IRQ_LEN)
#define	INTRCNT_EB64PLUS_IRQ_LEN 32

#ifndef _LOCORE
extern volatile long intrcnt[];
#endif
