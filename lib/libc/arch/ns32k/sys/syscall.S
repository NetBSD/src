/*	$NetBSD: syscall.S,v 1.3.14.1 2001/10/08 20:17:51 nathanw Exp $	*/

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
	RCSID("$NetBSD: syscall.S,v 1.3.14.1 2001/10/08 20:17:51 nathanw Exp $")
#endif

RSYSCALL(syscall)
