/*	$NetBSD: linux_misc.h,v 1.14.8.1 2008/01/09 01:51:12 matt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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

#ifndef _LINUX_MISC_H
#define _LINUX_MISC_H

/*
 * Options passed to the Linux wait4() system call.
 */
#define LINUX_WAIT4_WNOHANG   0x00000001
#define LINUX_WAIT4_WUNTRACED 0x00000002
#define LINUX_WAIT4_WNOTHREAD 0x20000000
#define LINUX_WAIT4_WALL      0x40000000
#define LINUX_WAIT4_WCLONE    0x80000000

#define LINUX_WAIT4_KNOWNFLAGS (LINUX_WAIT4_WNOHANG | \
                                LINUX_WAIT4_WUNTRACED | \
                                LINUX_WAIT4_WNOTHREAD | \
                                LINUX_WAIT4_WALL | \
                                LINUX_WAIT4_WCLONE)

/* This looks very unportable to me, but this is how Linux defines it. */
struct linux_sysinfo {
	long uptime;
	unsigned long loads[3];
#define LINUX_SYSINFO_LOADS_SCALE 65536
	unsigned long totalram;
	unsigned long freeram;
	unsigned long sharedram;
	unsigned long bufferram;
	unsigned long totalswap;
	unsigned long freeswap;
	unsigned short procs;
	unsigned long totalbig;
	unsigned long freebig;
	unsigned int mem_unit;
	char _f[20-2*sizeof(long)-sizeof(int)];
};

#define	LINUX_RLIMIT_CPU	0
#define	LINUX_RLIMIT_FSIZE	1
#define	LINUX_RLIMIT_DATA	2
#define	LINUX_RLIMIT_STACK	3
#define	LINUX_RLIMIT_CORE	4
#define	LINUX_RLIMIT_RSS	5
#define	LINUX_RLIMIT_NPROC	6
#define	LINUX_RLIMIT_NOFILE	7
#define	LINUX_RLIMIT_MEMLOCK	8
#define	LINUX_RLIMIT_AS		9
#define	LINUX_RLIMIT_LOCKS	10
#ifdef __mips__  /* XXX only mips32. On mips64, it's ~0ul */
#define	LINUX_RLIM_INFINITY	0x7fffffffUL
#else
#define	LINUX_RLIM_INFINITY	~0ul
#endif


/* When we don't know what to do, let it believe it is local */
#define	LINUX_DEFAULT_SUPER_MAGIC	LINUX_EXT2_SUPER_MAGIC

#define	LINUX_ADFS_SUPER_MAGIC		0x0000adf5
#define	LINUX_AFFS_SUPER_MAGIC		0x0000adff
#define	LINUX_CODA_SUPER_MAGIC		0x73757245
#define	LINUX_COH_SUPER_MAGIC		(LINUX_SYSV_MAGIC_BASE + 4)
#define	LINUX_DEVFS_SUPER_MAGIC		0x00001373
#define	LINUX_EFS_SUPER_MAGIC		0x00414A53
#define	LINUX_EXT2_SUPER_MAGIC		0x0000EF53
#define	LINUX_HPFS_SUPER_MAGIC		0xf995e849
#define	LINUX_ISOFS_SUPER_MAGIC		0x00009660
#define	LINUX_MINIX2_SUPER_MAGIC	0x00002468
#define	LINUX_MINIX2_SUPER_MAGIC2	0x00002478
#define	LINUX_MINIX_SUPER_MAGIC		0x0000137F
#define	LINUX_MINIX_SUPER_MAGIC2	0x0000138F
#define	LINUX_MSDOS_SUPER_MAGIC		0x00004d44
#define	LINUX_NCP_SUPER_MAGIC		0x0000564c
#define	LINUX_NFS_SUPER_MAGIC		0x00006969
#define	LINUX_OPENPROM_SUPER_MAGIC	0x00009fa1
#define	LINUX_PROC_SUPER_MAGIC		0x00009fa0
#define	LINUX_QNX4_SUPER_MAGIC		0x0000002f
#define	LINUX_REISERFS_SUPER_MAGIC	0x52654973
#define	LINUX_SMB_SUPER_MAGIC		0x0000517B
#define	LINUX_SYSV2_SUPER_MAGIC		(LINUX_SYSV_MAGIC_BASE + 3)
#define	LINUX_SYSV4_SUPER_MAGIC		(LINUX_SYSV_MAGIC_BASE + 2)
#define	LINUX_SYSV_MAGIC_BASE		0x012FF7B3
#define	LINUX_USBDEVICE_SUPER_MAGIC	0x00009fa2
#define	LINUX_DEVPTS_SUPER_MAGIC	0x00001cd1
#define	LINUX_XENIX_SUPER_MAGIC		(LINUX_SYSV_MAGIC_BASE + 1)

struct linux_mnttypes {
	const char *bsd;
	int linux;
};
extern const struct linux_mnttypes linux_fstypes[];
extern const int linux_fstypes_cnt;

#ifdef _KERNEL
__BEGIN_DECLS
int bsd_to_linux_wstat(int);
int linux_select1(struct lwp *, register_t *, int, fd_set *, fd_set *,
		       fd_set *, struct timeval *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* !_LINUX_MISC_H */
