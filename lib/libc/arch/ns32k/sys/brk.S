/*	$NetBSD: brk.S,v 1.5 1997/05/08 13:39:02 matthias Exp $	*/

/* 
 * Copyright (c) 1992 Helsinki University of Technology
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * HELSINKI UNIVERSITY OF TECHNOLOGY ALLOWS FREE USE OF THIS SOFTWARE IN
 * ITS "AS IS" CONDITION. HELSINKI UNIVERSITY OF TECHNOLOGY DISCLAIMS ANY
 * LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
 * USE OF THIS SOFTWARE.
 */
/*
 * HISTORY
 * 11-May-92  Johannes Helander (jvh) at Helsinki University of Technology
 *	Created.
 */

#include "SYS.h"

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: brk.S,v 1.5 1997/05/08 13:39:02 matthias Exp $")
#endif

ASDATA_D(minbrk, _C_LABEL(end))

ENTRY(brk)
	PIC_PROLOGUE
	cmpd	PIC_S_ARG0,PIC_GOT(_ASM_LABEL(minbrk))
	bge	0f
	movd	PIC_GOT(_ASM_LABEL(minbrk)),PIC_S_ARG0
0:	PIC_EPILOGUE
	SYSTRAP(break)
	bcs	_ASM_LABEL(cerror)
	PIC_PROLOGUE
	movd	PIC_S_ARG0,PIC_GOT(_C_LABEL(curbrk))
	movqd	0,r0
	PIC_EPILOGUE
	ret	0
