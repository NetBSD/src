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
 *
 *	$Id: brk.S,v 1.3 1994/05/03 20:40:33 phil Exp $
 */

#include "SYS.h"

	.globl	_curbrk
	.globl	_minbrk

ENTRY(_brk)
	br	ok2

ENTRY(brk)
	PIC_PROLOGUE
	cmpd	PIC_S_ARG0, PIC_GOT(_minbrk)
	bge	ok1
	movd	PIC_GOT(_minbrk), PIC_S_ARG0
ok1:
	PIC_EPILOGUE
ok2:
	movd	SYS_break, r0
	SVC
	bcs	cerror
	PIC_PROLOGUE
	movd	PIC_S_ARG0, PIC_GOT(_curbrk)
	movqd	0, r0
	PIC_EPILOGUE
	ret	0
