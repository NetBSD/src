/*	$NetBSD: fdsub.S,v 1.1 2001/09/27 10:03:28 minoura Exp $	*/

/*
 * Copyright (c) 2001 MINOURA Makoto.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>
#include "iocscall.h"

/*
 * int fd_check_fmt(drive, head, minmax)
 *	int drive;		(0-3)
 *	int head;		(0-1)
 *	struct fdfmt *minmax;	(results are stored)
 *	return value:	0 on success, or negative 
 */

	.text
ENTRY_NOPROFILE(fd_check_format)
||	movl	%sp@(4),%d1
||	andil	#3,%d1
||	orib	#0x90,%d1
||	lslw	#8,%d1
||	movql	#0,%d2
||	IOCS(__B_DRVCHK)
||	btst.b	#1,%d0
||	beq	fail
||	IOCS(__B_RECALI)

	moveml	%sp@(4),%d0-%d1
	andil	#1,%d1
	andil	#3,%d0
	lslw	#2,%d1
	orw	%d1,%d0
	jbsr	check_fd_format

	tstl	%d1
	beq	fail1

	moval	%sp@(12),%a0
	moveml	%d0-%d1,%a0@

	movl	#0,%d0
	rts

||fail:
||	movl	#-6,%d0
||	rts
fail1:
	movl	#-5,%d0

#define BOOT_ERROR(a)	movl	#0,%d1;	\
			rts

#include "chkfmt.s"
