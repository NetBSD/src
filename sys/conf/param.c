/*	$NetBSD: param.c,v 1.37.2.2 2001/08/24 00:08:59 nathanw Exp $	*/

/*
 * Copyright (c) 1980, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)param.c	7.20 (Berkeley) 6/27/91
 */

#include "opt_rtc_offset.h"
#include "opt_sb_max.h"
#include "opt_sysv.h"
#include "opt_sysvparam.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <ufs/ufs/quota.h>
#include <sys/kernel.h>
#include <sys/utsname.h>
#ifdef SYSVSHM
#include <machine/vmparam.h>
#include <sys/shm.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVMSG
#include <sys/msg.h>
#endif

/*
 * System parameter formulae.
 *
 * This file is copied into each directory where we compile
 * the kernel; it should be modified there to suit local taste
 * if necessary.
 *
 * Compiled with -DHZ=xx -DRTC_OFFSET=x -DMAXUSERS=xx
 */

#ifdef TIMEZONE
#error TIMEZONE is an obsolete kernel option.
#endif

#ifdef DST
#error DST is an obsolete kernel option.
#endif

#ifndef RTC_OFFSET
#define RTC_OFFSET 0
#endif

#ifndef HZ
#define	HZ 100
#endif

#ifndef MAXFILES
#define	MAXFILES	(3 * (NPROC + MAXUSERS) + 80)
#endif

int	hz = HZ;
int	tick = 1000000 / HZ;
int	tickadj = 240000 / (60 * HZ);		/* can adjust 240ms in 60s */
int	rtc_offset = RTC_OFFSET;
int	maxproc = NPROC;
int	desiredvnodes = NVNODE;
int	maxfiles = MAXFILES;
int	ncallout = 16 + NPROC;	/* size of callwheel (rounded to ^2) */
u_long	sb_max = SB_MAX;	/* maximum socket buffer size */
int	fscale = FSCALE;	/* kernel uses `FSCALE', user uses `fscale' */

/*
 * Various mbuf-related parameters.  These can also be changed at run-time
 * with sysctl.
 */
int	nmbclusters = NMBCLUSTERS;

#ifndef MBLOWAT
#define	MBLOWAT		16
#endif
int	mblowat = MBLOWAT;

#ifndef MCLLOWAT
#define	MCLLOWAT	8
#endif
int	mcllowat = MCLLOWAT;

/*
 * Values in support of System V compatible shared memory.	XXX
 */
#ifdef SYSVSHM
#ifndef	SHMMAX
#define	SHMMAX	SHMMAXPGS	/* shminit() performs a `*= NBPG' */
#endif
#ifndef	SHMMIN
#define	SHMMIN	1
#endif
#ifndef	SHMMNI
#define	SHMMNI	128			/* <= SHMMMNI in shm.h */
#endif
#ifndef	SHMSEG
#define	SHMSEG	128
#endif
#define	SHMALL	SHMMAXPGS

struct	shminfo shminfo = {
	SHMMAX,
	SHMMIN,
	SHMMNI,
	SHMSEG,
	SHMALL
};
#endif

/*
 * Values in support of System V compatible semaphores.
 */
#ifdef SYSVSEM
struct	seminfo seminfo = {
	SEMMAP,		/* # of entries in semaphore map */
	SEMMNI,		/* # of semaphore identifiers */
	SEMMNS,		/* # of semaphores in system */
	SEMMNU,		/* # of undo structures in system */
	SEMMSL,		/* max # of semaphores per id */
	SEMOPM,		/* max # of operations per semop call */
	SEMUME,		/* max # of undo entries per process */
	SEMUSZ,		/* size in bytes of undo structure */
	SEMVMX,		/* semaphore maximum value */
	SEMAEM		/* adjust on exit max value */
};
#endif

/*
 * Values in support of System V compatible messages.
 */
#ifdef SYSVMSG
struct	msginfo msginfo = {
	MSGMAX,		/* max chars in a message */
	MSGMNI,		/* # of message queue identifiers */
	MSGMNB,		/* max chars in a queue */
	MSGTQL,		/* max messages in system */
	MSGSSZ,		/* size of a message segment */
			/* (must be small power of 2 greater than 4) */
	MSGSEG		/* number of message segments */
};
#endif

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted
 * (if they've been externed everywhere else; hah!).
 */
struct	buf *buf;
char	*buffers;

/*
 * These control when and to what priority a process gets after a certain
 * amount of CPU time expires.  AUTONICETIME is in seconds.
 * AUTONICEVAL is NOT offset by NZERO, i.e. it's between PRIO_MIN and PRIO_MAX.
 */
#ifndef AUTONICETIME
#define AUTONICETIME (60 * 10)	/* 10 minutes */
#endif

#ifndef AUTONICEVAL
#define AUTONICEVAL 4		/* default + 4 */
#endif

int autonicetime = AUTONICETIME;
int autoniceval = AUTONICEVAL;

/*
 * Actual network mbuf sizes (read-only), for netstat.
 */
const	int msize = MSIZE;
const	int mclbytes = MCLBYTES;
