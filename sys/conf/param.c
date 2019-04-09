/*	$NetBSD: param.c,v 1.68 2019/04/09 22:05:27 pgoyette Exp $	*/

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
 *	@(#)param.c	7.20 (Berkeley) 6/27/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: param.c,v 1.68 2019/04/09 22:05:27 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_hz.h"
#include "opt_rtc_offset.h"
#include "opt_sysv.h"
#include "opt_sysvparam.h"
#include "opt_multiprocessor.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <ufs/ufs/quota.h>
#include <sys/kernel.h>
#include <sys/utsname.h>
#include <sys/lwp.h>
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
 * PCC cannot handle the 80KB string literal.
 */
#if !defined(__PCC__)
#define CONFIG_FILE
#include "config_file.h"
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

#ifndef MAXEXEC
#define	MAXEXEC		16
#endif

int	hz = HZ;
int	tick = 1000000 / HZ;
/* can adjust 240ms in 60s */
int	tickadj = (240000 / (60 * HZ)) ? (240000 / (60 * HZ)) : 1;
int	rtc_offset = RTC_OFFSET;
int	maxproc = NPROC;
int	maxlwp = MAXLWP;
int	desiredvnodes = NVNODE;
u_int	maxfiles = MAXFILES;
int	fscale = FSCALE;	/* kernel uses `FSCALE', user uses `fscale' */
int	maxexec = MAXEXEC;	/* max number of concurrent exec() calls */

#ifdef MULTIPROCESSOR
u_int	maxcpus = MAXCPUS;
size_t	coherency_unit = COHERENCY_UNIT;
#else
u_int	maxcpus = 1;
size_t	coherency_unit = ALIGNBYTES + 1;
#endif

/*
 * Various mbuf-related parameters.  These can also be changed at run-time
 * with sysctl.
 */
int	nmbclusters = 0;

#ifndef MBLOWAT
#define	MBLOWAT		16
#endif
int	mblowat = MBLOWAT;

#ifndef MCLLOWAT
#define	MCLLOWAT	8
#endif
int	mcllowat = MCLLOWAT;

/*
 * Actual network mbuf sizes (read-only), for netstat.
 */
const	int msize = MSIZE;
const	int mclbytes = MCLBYTES;
