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
 *
 *	$Id: asm.h,v 1.1.1.1 1993/09/09 23:53:46 phil Exp $
 */

/*
 * 	File: asm.h
 *	Author: Johannes Helander, Tero Kivinen, Tatu Ylonen
 *	Modified by Phil Nelson for NetBSD.
 *	Helsinki University of Technology 1992.
 */

#ifndef _NS532_ASM_H_ 
#define _NS532_ASM_H_

#define S_ARG0	4(sp)
#define S_ARG1	8(sp)
#define S_ARG2	12(sp)
#define S_ARG3	16(sp)

#define FRAME	enter [ ],0
#define EMARF	exit []

#if 1 /* DEBUG */
#define DFRAME	FRAME
#define DEMARF	EMARF
#else
#define DFRAME
#define DEMARF
#endif

#define B_ARG0	 8(fp)
#define B_ARG1	12(fp)
#define B_ARG2	16(fp)
#define B_ARG3	20(fp)

#define ALIGN 0

#ifdef  __STDC__

#define EX(x) _ ## x
#define LEX(x) _ ## x ## :
#define MCOUNT
#define	ENTRY(x)	.globl EX(x); .align ALIGN; LEX(x)
#define	ASENTRY(x)	.globl x; .align ALIGN; x ## :

#else __STDC__

#define EX(x) _/**/x
#define LEX(x) _/**/x/**/:
#define MCOUNT
#define	ENTRY(x)	.globl EX(x); .align ALIGN; LEX(x)
#define	ASENTRY(x)	.globl x; .align ALIGN; x:

#endif __STDC__

#define	SVC svc

#define	Entry(x)	.globl EX(x); LEX(x)
#define	DATA(x)		.globl EX(x); .align ALIGN; LEX(x)

#endif /* _NS532_ASM_H_ */
