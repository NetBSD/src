/*	$NetBSD: exect.S,v 1.5 1997/05/08 13:39:03 matthias Exp $	*/

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
#include <machine/psl.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: exect.S,v 1.5 1997/05/08 13:39:03 matthias Exp $")
#endif

ENTRY(exect)
	bispsrb	PSL_T
	SYSTRAP(execve)
	bcs	_ASM_LABEL(cerror)
	ret	0
