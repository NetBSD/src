/* 
 * Ported to Boot 386BSD by Julian Elsicher (julian@tfs.com) Sept. 1992
 *
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
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
 *
 *	$Id: asm.h,v 1.4 1994/02/04 05:43:53 mycroft Exp $
 */

#define FRAME	pushl %ebp; movl %esp, %ebp
#define EMARF	leave

#ifdef	__STDC__
#define EXT(x) _ ## x
#define LEXT(x) _ ## x ## :
#else __STDC__
#define EXT(x) _/**/x
#define LEXT(x) _/**/x/**/:
#endif __STDC__

#define addr32	.byte 0x67
#define data32	.byte 0x66

#ifdef PROF

#define MCOUNT		.data; 9: .long 0; .text; lea 9b,%edx; call mcount
#define	ENTRY(x)	.globl EXT(x); .align 2,0x90; LEXT(x); \
			FRAME; MCOUNT; EMARF
#define	ENTRY2(x,y)	.globl EXT(x); .globl EXT(y); \
			.align 2,0x90; LEXT(x) LEXT(y); \
			FRAME; MCOUNT; EMARF
#define	ASENTRY(x) 	.globl x; .align 2,0x90; x:; \
			FRAME; MCOUNT; EMARF

#else	PROF

#define MCOUNT
#define	ENTRY(x)	.globl EXT(x); .align 2,0x90; LEXT(x)
#define	ENTRY2(x,y)	.globl EXT(x); .globl EXT(y); \
			.align 2,0x90; LEXT(x) LEXT(y)
#define	ASENTRY(x)	.globl x; .align 2,0x90; x:

#endif	PROF
