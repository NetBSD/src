/*	$NetBSD: irix_sysmp.h,v 1.1.4.3 2002/03/16 16:00:28 jdolecek Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IRIX_SYSMP_H_
#define _IRIX_SYSMP_H_

/* From IRIX's <sys/sysmp.h> */
#define IRIX_MP_NPROCS		1
#define IRIX_MP_NAPROCS		2
#define IRIX_MP_SPACE		3
#define IRIX_MP_ENABLE		4
#define IRIX_MP_DISABLE		5
#define IRIX_MP_KERNADDR	8
#define IRIX_MP_SASZ		9
#define IRIX_MP_SAGET		10
#define IRIX_MP_SCHED		13
#define IRIX_MP_PGSIZE		14
#define IRIX_MP_SAGET1		15
#define IRIX_MP_EMPOWER		16
#define IRIX_MP_RESTRICT	17
#define IRIX_MP_CLOCK		18
#define IRIX_MP_MUSTRUN		19
#define IRIX_MP_RUNANYWHERE	20
#define IRIX_MP_STAT		21
#define IRIX_MP_ISOLATE		22
#define IRIX_MP_UNISOLATE	23
#define IRIX_MP_PREEMPTIVE	24
#define IRIX_MP_NONPREEMPTIVE	25
#define IRIX_MP_FASTCLOCK	26
#define IRIX_MP_CLEARNFSSTAT	28
#define IRIX_MP_GETMUSTRUN	29
#define IRIX_MP_MUSTRUN_PID	30
#define IRIX_MP_RUNANYWHERE_PID	31
#define IRIX_MP_GETMUSTRUN_PID	32
#define IRIX_MP_CLEARCFSSTAT	33
#define IRIX_MP_CPUSET		35
#define IRIX_MP_MISER_GETREQUEST	36
#define IRIX_MP_MISER_SENDREQUEST	37
#define IRIX_MP_MISER_RESPOND		38
#define IRIX_MP_MISER_GETRESOURCE	39
#define IRIX_MP_MISER_SETRESOURCE	40
#define IRIX_MP_MISER_CHECKACCESS	41
#define IRIX_MP_NUMNODES		42
#define IRIX_MP_NUMA_GETDISTMATRIX	43
#define IRIX_MP_NUMA_GETCPUNODEMAP	44
#define IRIX_MP_DISABLE_CPU		45
#define IRIX_MP_ENABLE_CPU		46
#define IRIX_MP_CPU_PATH		47
#define IRIX_MP_KLSTAT			48
#define IRIX_MP_NUM_CPU_PER_NODE	49

/* IRIX_MP_KERNADDR subcommands */
#define IRIX_MPKA_VAR                2
#define IRIX_MPKA_SWPLO              3
#define IRIX_MPKA_INO                4
#define IRIX_MPKA_SEMAMETER          7
#define IRIX_MPKA_PROCSIZE           9
#define IRIX_MPKA_TIME               10
#define IRIX_MPKA_MSG                11
#define IRIX_MPKA_MSGINFO            14
#define IRIX_MPKA_SPLOCKMETER        17
#define IRIX_MPKA_SPLOCKMETERTAB     18
#define IRIX_MPKA_AVENRUN            19
#define IRIX_MPKA_PHYSMEM            20
#define IRIX_MPKA_KPBASE             21
#define IRIX_MPKA_PFDAT              22
#define IRIX_MPKA_FREEMEM            23
#define IRIX_MPKA_USERMEM            24
#define IRIX_MPKA_PDWRIMEM           25
#define IRIX_MPKA_BUFMEM             26
#define IRIX_MPKA_BUF                27
#define IRIX_MPKA_CHUNKMEM           30
#define IRIX_MPKA_MAXCLICK           31
#define IRIX_MPKA_PSTART             32
#define IRIX_MPKA_TEXT               33
#define IRIX_MPKA_ETEXT              34
#define IRIX_MPKA_EDATA              35
#define IRIX_MPKA_END                36
#define IRIX_MPKA_SYSSEGSZ           37
#define IRIX_MPKA_SEM_MAC            38
#define IRIX_MPKA_MSG_MAC            40
#define IRIX_MPKA_BSD_KERNADDRS      41
#endif /* _IRIX_SYSIRIX_MP_H_ */
