/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1992 Helsinki University of Technology
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND HELSINKI UNIVERSITY OF TECHNOLOGY ALLOW FREE USE
 * OF THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * HELSINKI UNIVERSITY OF TECHNOLOGY DISCLAIM ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */
/*
 * 	File: ns532/bcopy.s
 *	Author: Tatu Ylonen, Jukka Virtanen
 *	Helsinki University of Technology 1992.
 *
 *	$Id: bcopy.s,v 1.3 1994/03/22 00:18:28 phil Exp $
 */

#include <machine/asm.h>

	.text

/* ovbcopy (from, to, bcount)  -- does overlapping stuff */

ENTRY(bcopy)
ENTRY(ovbcopy)
	DFRAME
	movd	B_ARG0,r1  /* from */
	movd	B_ARG1,r2  /* to */
	cmpd	r2, r1
	bls	common	   /* safe to do standard thing */
	movd	B_ARG2,r0  /* bcount */
	addd	r1, r0     /* add to start of from */
	cmpd	r2, r0
	bhs	common	   /* safe to do standard thing */

/* Must do a reverse copy  (and assume that the start is on a
	word boundry . . . so we move the "remaining" bytes first) */

	movd	B_ARG2, r0 /* bcount */
	addqd	-1, r0
	addd	r0, r1
	addd	r0, r2
	addqd	1, r0
	andd	3, r0
	movsb	b	   /* move bytes backwards */
	addqd	-3, r1	   /* change to double pointer */
	addqd	-3, r2	   /* ditto */
	movd	B_ARG2, r0
	lshd	-2,r0
	movsd	b	   /* move words backwards */
	DEMARF
	ret	0

/* bcopy(from, to, bcount) -- non overlapping */

/* ENTRY(bcopy)  -- same as ovbcopy until furthur notice.... */
	DFRAME
	movd	B_ARG0,r1  /* from */
	movd	B_ARG1,r2  /* to */
common:	movd	B_ARG2,r0  /* bcount */
	lshd	-2,r0
	movsd		   /* move words */
	movd	B_ARG2,r0
	andd	3,r0
	movsb		   /* move bytes */
	DEMARF
	ret	0

	
#if 0
/* bcopy_bytes(from, to, bcount)
 *
 * PC532 uses memory mapped SCSI pseudo dma addresses that
 * are not in the IO address space. This means that the
 * 532 does prefetch when doing SCSI pseudo dma input.
 *
 * This code solves this by using "movsb" instruction that
 * provides a single point when interrupts are recognized
 * and does not prefetch (XXX we are not sure of the prefetch,
 * since this is not documented; however, see pp. 3-28 in the
 * 32000 instruction set reference manual (1984 version)).
 * 
 * Scsi pseudo dma involves the additional problem: The target we read
 * from might send a disconnect (or some other) message when we are
 * expecting data. In this case we will get a phase mismatch interrupt
 * and we need to know if we were doing DMA when the interrupt
 * occurred. This we know by status bits of the corresponding scsi
 * driver (like aic->state). If so, the interrupt might occur at any
 * of the following cases:
 *
 *  1) We are in splx() after we lowered the interrupt level
 *  2) We are in bcopy_bytes() but before the "movsb" is started
 *  3) We are in "movsb" instruction, so the registers are valid
 *  4) We have transferred all requested bytes (r0 == 0)
 *     and the dma is done, but not yet returned from here.
 *
 * If you need to cancel the DMA transfer for any reason, you

 * should do as follows:
 *
 * if you want to CANCEL DMA when interrupt comes, AND you are
 * doing SCSI DMA:
 *
 * if (bcopy_bytes <= INTERRUPT(PC) < bcopy_bytes_movsb)
 *   {
 *     if (INTERRUPT(PC) < bcopy_bytes_init)
 *       bcopy_bytes_failed = 1;
 *     else
 *       INTERRUPT(R0) = 0;
 *   }
 * else if (bcopy_bytes_movsb < INTERRUPT(PC) < bcopy_bytes_end)
 *   nothing
 *   ;
 */

/* Untested! */

	.data
	.globl EX(bcopy_bytes_failed)
LEX(bcopy_bytes_failed)
	.long 0

	.text
ENTRY(bcopy_bytes)
	FRAME
	movd	B_ARG0,r1  /* from */
	movd	B_ARG1,r2  /* to */
	movd	B_ARG2,r0  /* bcount */

	.globl EX(bcopy_bytes_init)
LEX(bcopy_bytes_init)
	cmpqd	0, EX(bcopy_bytes_failed)
	bne	bcopy_skip /* BNE does prefetches as if branch taken */

	.globl EX(bcopy_bytes_movsb)
LEX(bcopy_bytes_movsb)
	movsb		   # move bytes

bcopy_skip:
	EMARF
	ret	0
	.globl EX(bcopy_bytes_end)
LEX(bcopy_bytes_end)
#endif
