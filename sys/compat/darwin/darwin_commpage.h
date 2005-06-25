/*	$NetBSD: darwin_commpage.h,v 1.5 2005/06/25 06:30:19 christos Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef	_DARWIN_COMMPAGE_H_
#define	_DARWIN_COMMPAGE_H_

struct darwin_commpage {
	int dcp_signature;		/* 0/0x000 */
	char dcp_pad1[26];
	short dcp_version;		/* 30/0x01e */
	char dcp_cap;			/* 32/0x020 */
	char dcp_ncpu;			/* 33/0x021 */
	char dcp_pad2[2];
	char dcp_vector;		/* 36/0x024 */
	char dcp_64bit;			/* 37/0x025 */
	short dcp_cachelinelen;		/* 38/0x026 */
	char dcp_pad3[24];
	long long dcp_2pow52;			/* 64/0x040 */
	long long dcp_10pow6;			/* 72/0x048 */
	char dcp_pad4[16];
	long long dcp_timebase;			/* 96/0x060 */
	long long dcp_timestamp;		/* 104/0x068 */
	long long dcp_secpertick;		/* 112/0x070 */
	char dcp_pad5[392];
	char dcp_mach_absolute_time[32];	/* 512/0x200 */
	char dcp_spinlock_try[64];		/* 544/0x220 */
	char dcp_spinlock_lock[64];		/* 608/0x260 */
	char dcp_spinlock_unlock[32];		/* 672/0x2a0 */
	char dcp_pthread_getspecific[32];	/* 704/0x2c0 */
	char dcp_gettimeofday[512];		/* 736/0x2e0 */
	char dcp_sys_dcache_flush[64];		/* 1248/0x4e0 */
	char dcp_sys_icache_invalidate[96];	/* 1312/0x520 */
	char dcp_pthread_self[64];		/* 1408/0x580 */
	char dcp_spinlock_relinquish[64];	/* 1472/0x5c0 */
	char dcp_bzero[384];			/* 1536/0x600 */
	char dcp_bcopy[32];			/* 1920/0x780 */
	char dcp_memcpy[2144];			/* 1952/0x7a0 */
	char dcp_bigcopy[1536];			/* 4096/0x1000 */
};

int darwin_commpage_map(struct proc *);

extern int darwin_commpage_mach_absolute_time[];
extern int darwin_commpage_spinlock_try[];
extern int darwin_commpage_spinlock_lock[];
extern int darwin_commpage_spinlock_unlock[];
extern int darwin_commpage_pthread_getspecific[];
extern int darwin_commpage_gettimeofday[];
extern int darwin_commpage_sys_dcache_flush[];
extern int darwin_commpage_sys_icache_invalidate[];
extern int darwin_commpage_pthread_self[];
extern int darwin_commpage_spinlock_relinquish[];
extern int darwin_commpage_bzero[];
extern int darwin_commpage_bcopy[];
extern int darwin_commpage_memcpy[];
extern int darwin_commpage_bigcopy[];

extern int darwin_commpage_mach_absolute_time_size[];
extern int darwin_commpage_spinlock_try_size[];
extern int darwin_commpage_spinlock_lock_size[];
extern int darwin_commpage_spinlock_unlock_size[];
extern int darwin_commpage_pthread_getspecific_size[];
extern int darwin_commpage_gettimeofday_size[];
extern int darwin_commpage_sys_dcache_flush_size[];
extern int darwin_commpage_sys_icache_invalidate_size[];
extern int darwin_commpage_pthread_self_size[];
extern int darwin_commpage_spinlock_relinquish_size[];
extern int darwin_commpage_bzero_size[];
extern int darwin_commpage_bcopy_size[];
extern int darwin_commpage_memcpy_size[];
extern int darwin_commpage_bigcopy_size[];

#endif /* _DARWIN_COMMPAGE_H_ */
