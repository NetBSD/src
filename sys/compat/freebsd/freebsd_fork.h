/*	$NetBSD: freebsd_fork.h,v 1.2 2003/08/07 16:30:39 agc Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)unistd.h	8.2 (Berkeley) 1/7/94
 * $FreeBSD: src/sys/sys/unistd.h,v 1.22.2.2 2000/08/22 01:46:30 jhb Exp $
 */

#ifndef _FREEBSD_FORK_H_
#define	_FREEBSD_FORK_H_

/*
 * rfork() options.
 *
 * XXX currently, operations without RFPROC set are not supported.
 */
#define FREEBSD_RFNAMEG		(1<<0)  /* UNIMPL new plan9 `name space' */
#define FREEBSD_RFENVG		(1<<1)  /* UNIMPL copy plan9 `env space' */
#define FREEBSD_RFFDG		(1<<2)  /* copy fd table */
#define FREEBSD_RFNOTEG		(1<<3)  /* UNIMPL create new plan9 note grou' */
#define FREEBSD_RFPROC		(1<<4)  /* change child (else curproc) */
#define FREEBSD_RFMEM		(1<<5)  /* share `address space' */
#define FREEBSD_RFNOWAIT	(1<<6)  /* parent need not wait() on child */ 
#define FREEBSD_RFCNAMEG	(1<<10) /* UNIMPL zero plan9 `name space' */
#define FREEBSD_RFCENVG		(1<<11) /* UNIMPL zero plan9 `env space' */
#define FREEBSD_RFCFDG		(1<<12) /* zero fd table */
#define FREEBSD_RFTHREAD	(1<<13)	/* enable kernel thread support */
#define FREEBSD_RFSIGSHARE	(1<<14)	/* share signal handlers */
#define FREEBSD_RFLINUXTHPN     (1<<16) /* do linux clone exit notification */
#define FREEBSD_RFPPWAIT	(1<<31) /* sleep until child exits (vfork) */

#endif /* !_FREEBSD_FORK_H_ */
