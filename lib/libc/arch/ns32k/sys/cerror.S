/*	$NetBSD: cerror.S,v 1.5 1997/05/08 13:39:03 matthias Exp $	*/

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
 * 29-Apr-92  Johannes Helander (jvh) at Helsinki University of Technology
 *	Created.
 */

#include "SYS.h"

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: cerror.S,v 1.5 1997/05/08 13:39:03 matthias Exp $")
#endif

ASENTRY(cerror)
#ifdef _REENTRANT
	movd	r0,tos
	bsr	_C_LABEL(__errno)
	movd	tos,0(r0)
#else
	PIC_PROLOGUE
	movd	r0,PIC_GOT(_C_LABEL(errno))
	PIC_EPILOGUE
#endif
	movqd	-1,r0
	movqd	-1,r1
	ret	0
