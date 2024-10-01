/*	$NetBSD: linux_misc.h,v 1.34 2024/10/01 16:41:29 riastradh Exp $	*/

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
 * Options passed to the Linux wait4() and waitid() system calls.
 */
#define LINUX_WNOHANG		0x00000001
#define LINUX_WUNTRACED		0x00000002
#define LINUX_WEXITED		0x00000004
#define LINUX_WCONTINUED	0x00000008
#define LINUX_WNOWAIT		0x01000000
#define LINUX_WNOTHREAD		0x20000000
#define LINUX_WALL		0x40000000
#define LINUX_WCLONE		0x80000000

#define LINUX_WAIT4_KNOWNFLAGS (LINUX_WNOHANG | \
                                LINUX_WUNTRACED | \
                                LINUX_WCONTINUED | \
                                LINUX_WNOTHREAD | \
                                LINUX_WALL | \
                                LINUX_WCLONE)

#define LINUX_WAITID_KNOWNFLAGS (LINUX_WNOHANG | \
				 LINUX_WEXITED | \
				 LINUX_WUNTRACED | \
				 LINUX_WCONTINUED | \
				 LINUX_WNOWAIT)

/*
 * Passed as the first argument of waitid(2).
 */
#define LINUX_P_ALL	0
#define LINUX_P_PID	1
#define LINUX_P_PGID	2
#define LINUX_P_PIDFD	3

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
#define	LINUX_RLIMIT_SIGPENDING	11
#define	LINUX_RLIMIT_MSGQUEUE	12
#define	LINUX_RLIMIT_NICE	13
#define	LINUX_RLIMIT_RTPRIO	14
#define	LINUX_RLIMIT_RTTIME	15
#ifdef __mips__  /* XXX only mips32. On mips64, it's ~0ul */
#define	LINUX_RLIM_INFINITY	0x7fffffffUL
#define	LINUX32_RLIM_INFINITY	0x7fffffffU
#else
#define	LINUX_RLIM_INFINITY	~0ul
#define	LINUX32_RLIM_INFINITY	~0u
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
#define	LINUX_TMPFS_SUPER_MAGIC		0x01021994
#define	LINUX_USBDEVICE_SUPER_MAGIC	0x00009fa2
#define	LINUX_DEVPTS_SUPER_MAGIC	0x00001cd1
#define	LINUX_XENIX_SUPER_MAGIC		(LINUX_SYSV_MAGIC_BASE + 1)

struct linux_mnttypes {
	const char *mty_bsd;
	int mty_linux;
};
extern const struct linux_mnttypes linux_fstypes[];
extern const int linux_fstypes_cnt;

/* Personality types. */
#define LINUX_PER_QUERY		0xffffffff
#define LINUX_PER_LINUX		0x00
#define LINUX_PER_LINUX32	0x08
#define LINUX_PER_MASK		0xff

/* Personality flags. */
#define LINUX_PER_ADDR_NO_RANDOMIZE	0x00040000

/*
 * Convert POSIX_FADV_* constants from Linux to NetBSD
 * (it's f(x)=x everywhere except S390)
 */
#define linux_to_bsd_posix_fadv(advice) (advice)

struct linux_getcpu_cache{
	unsigned long blob[128 / sizeof(long)];
};

struct linux_epoll_event {
	uint32_t	events;
	uint64_t	data;
}

#if defined(__amd64__)
/* Only for x86_64. See include/uapi/linux/eventpoll.h. */
__packed
#endif
;

#ifdef _KERNEL
__BEGIN_DECLS
struct linux_timeval;
int bsd_to_linux_wstat(int);
int linux_select1(struct lwp *, register_t *, int, fd_set *, fd_set *,
		       fd_set *, struct linux_timeval *);
int linux_do_sys_utimensat(struct lwp *, int, const char *,
    struct timespec *, int, register_t *);
int linux_do_futex(int *, int, int, struct timespec *, int *, int, int,
    register_t *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* !_LINUX_MISC_H */
