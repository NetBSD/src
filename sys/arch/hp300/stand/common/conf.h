/*	$NetBSD: conf.h,v 1.3.2.1 2024/06/22 10:57:10 martin Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
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
 */

#ifdef SUPPORT_ETHERNET
int netstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int netopen(struct open_file *, ...);
int netclose(struct open_file *);
#endif

#ifdef SUPPORT_TAPE
int ctstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int ctopen(struct open_file *, ...);
int ctclose(struct open_file *);
int ctpunit(int, int, int *);
#endif

#ifdef SUPPORT_DISK
int rdstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int rdopen(struct open_file *, ...);
int rdclose(struct open_file *);

int sdstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int sdopen(struct open_file *, ...);
int sdclose(struct open_file *);
#endif
#ifdef SUPPORT_UFS2
#define NFSYS_UFS2	1
#else
#define NFSYS_UFS2	0
#endif
#ifdef SUPPORT_CD
#define NFSYS_CD9660	1
#else
#define NFSYS_CD9660	0
#endif
#define NFSYS_FS	(1 + NFSYS_UFS2 + NFSYS_CD9660)

#ifdef SUPPORT_ETHERNET
extern struct netif_driver le_driver;
#endif

/*
 * Switch we use to set punit in devopen.
 */
struct punitsw {
	int	(*p_punit)(int, int, int *);
};
extern	struct punitsw punitsw[];
extern	int npunit;

extern	struct fs_ops file_system_rawfs[1];
extern	struct fs_ops file_system_ufs[NFSYS_FS];
extern	struct fs_ops file_system_nfs[1];
