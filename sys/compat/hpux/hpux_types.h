/*	$NetBSD: hpux_types.h,v 1.2 1999/08/23 20:59:33 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _COMPAT_HPUX_HPUX_TYPES_H_
#define	_COMPAT_HPUX_HPUX_TYPES_H_

/*
 * HP-UX 9.x hp9000s300 compatible types.
 */

typedef	long		hpux_dev_t;	/* device numbers */
typedef	u_long		hpux_ino_t;	/* file serial numbers */
typedef	u_short		hpux_mode_t;	/* file types and modes */
typedef	short		hpux_nlink_t;	/* link counts */
typedef	long		hpux_off_t;	/* file offsets and sizes */
typedef	long		hpux_pid_t;	/* process and session IDs */
typedef	hpux_pid_t	hpux_sid_t;
typedef	long		hpux_gid_t;	/* group IDs */
typedef	long		hpux_uid_t;	/* user IDs */
typedef	long		hpux_time_t;	/* times in seconds */
typedef	u_int		hpux_size_t;	/* in-core sizes */
typedef	int		hpux_ssize_t;	/* signed size_t */
typedef	u_short		hpux_site_t;	/* see hpux_stat.h */
typedef	u_short		hpux_cnode_t;	/* see hpux_stat.h */
typedef	u_long		hpux_clock_t;	/* clock ticks */
typedef	long		hpux_key_t;	/* for IPC IDs */
typedef	long		hpux_daddr_t;	/* disk block addresses */
typedef	long		hpux_swblk_t;	/* swap block addresses */

typedef	long		hpux_paddr_t;
typedef	short		hpux_cnt_t;
typedef	u_int		hpux_space_t;
typedef	u_int		hpux_prot_t;
typedef	u_long		hpux_cdno_t;
typedef	u_short		hpux_use_t;

typedef	struct hpux__physaddr { int r[1]; } *hpux_physadr;
typedef struct hpux__quad { long val[2]; } hpux_quad;

typedef	char		hpux_spu_t;

typedef	long		hpux_fd_mask;

#define	HPUX_FD_SETSIZE	2048		/* MAXFUPLIM from <sys/types.h> */
#define	HPUX_NFDBITS	(sizeof(hpux_fd_mask) * 8)

#ifdef howmany
typedef	struct hpux_fd_set {
	hpux_fd_mask fds_bits[howmany(HPUX_FD_SETSIZE, HPUX_NFDBITS)];
} hpux_fd_set;
#endif

#endif /* _COMPAT_HPUX_HPUX_TYPES_H_ */
