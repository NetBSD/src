/*	$NetBSD: linux_osf1.h,v 1.1 2019/03/24 16:24:19 maxv Exp $	*/

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

typedef int16_t		osf1_short;
typedef int32_t		osf1_int;
typedef int64_t		osf1_long;
typedef u_int32_t	osf1_u_int;

typedef int32_t		osf1_dev_t;
typedef u_int32_t	osf1_ino_t;
typedef u_int32_t	osf1_mode_t;
typedef u_int16_t	osf1_nlink_t;
typedef u_int32_t	osf1_uid_t;
typedef u_int32_t	osf1_gid_t;

typedef int32_t		osf1_time_t;
typedef u_int32_t	osf1_uint_t;
typedef u_int64_t	osf1_sigset_t;
typedef u_int64_t	osf1_size_t;
typedef u_int64_t	osf1_fsid_t;
typedef u_int64_t	osf1_rlim_t;
typedef void		*osf1_data_ptr;	/* XXX hard to fix size */
typedef void		*osf1_fcn_ptr;	/* XXX hard to fix size, bogus */
typedef	osf1_int	osf1_key_t;
typedef	osf1_int	osf1_pid_t;
typedef u_int64_t	osf1_blksize_t;
typedef u_int64_t	osf1_blkcnt_t;

#define OSF1_SI_SYSNAME		1
#define OSF1_SI_HOSTNAME	2
#define OSF1_SI_RELEASE		3
#define OSF1_SI_VERSION		4
#define OSF1_SI_MACHINE		5
#define OSF1_SI_ARCHITECTURE	6
#define OSF1_SI_HW_SERIAL	7
#define OSF1_SI_HW_PROVIDER	8
#define OSF1_SI_SRPC_DOMAIN	9
#define OSF1_SI_SET_HOSTNAME	258
#define OSF1_SI_SET_SYSNAME	259
#define OSF1_SI_SET_SRPC_DOMAIN	265

struct osf1_timeval {				/* time.h */
	osf1_time_t	tv_sec;
	osf1_int	tv_usec;
};

#define	OSF1_RUSAGE_THREAD	1
#define	OSF1_RUSAGE_SELF	0
#define	OSF1_RUSAGE_CHILDREN	-1

struct osf1_rusage {
	struct osf1_timeval ru_utime;
	struct osf1_timeval ru_stime;
	osf1_long	ru_maxrss;
	osf1_long	ru_ixrss;
	osf1_long	ru_idrss;
	osf1_long	ru_isrss;
	osf1_long	ru_minflt;
	osf1_long	ru_majflt;
	osf1_long	ru_nswap;
	osf1_long	ru_inblock;
	osf1_long	ru_oublock;
	osf1_long	ru_msgsnd;
	osf1_long	ru_msgrcv;
	osf1_long	ru_nsignals;
	osf1_long	ru_nvcsw;
	osf1_long	ru_nivcsw;
};

struct osf1_itimerval {
	struct osf1_timeval it_interval;
	struct osf1_timeval it_value;
};

#define OSF1_ITIMER_REAL	0
#define OSF1_ITIMER_VIRTUAL	1
#define OSF1_ITIMER_PROF	2

struct osf1_timezone {
	osf1_int	tz_minuteswest;
	osf1_int	tz_dsttime;
};

#define	OSF1_WNOHANG		0x01
#define	OSF1_WUNTRACED		0x02

struct	osf1_cpu_info {
	int		current_cpu;
	int     	cpus_in_box;
	int		cpu_type;
	int		ncpus;
	u_int64_t	cpus_present;
	u_int64_t 	cpus_running;
	u_int64_t	cpu_binding;
	u_int64_t	cpu_ex_binding;
	int  		mhz;
	int  		unused[3];
};

#define	OSF_SET_IEEE_FP_CONTROL  14

#define OSF_GET_MAX_UPROCS      2
#define OSF_GET_PHYSMEM         19
#define OSF_GET_MAX_CPU         30
#define OSF_GET_IEEE_FP_CONTROL 45
#define OSF_GET_CPUS_IN_BOX     55
#define OSF_GET_CPU_INFO        59
#define OSF_GET_PROC_TYPE       60
#define OSF_GET_HWRPB           101
#define OSF_GET_PLATFORM_NAME   103

#define OSF1_MOUNT_NONE		0
#define OSF1_MOUNT_UFS		1
#define OSF1_MOUNT_NFS		2
#define OSF1_MOUNT_MFS		3
#define OSF1_MOUNT_PC		4
#define OSF1_MOUNT_S5FS		5
#define OSF1_MOUNT_CDFS		6
#define OSF1_MOUNT_DFS		7
#define OSF1_MOUNT_EFS		8
#define OSF1_MOUNT_PROCFS	9
#define OSF1_MOUNT_MSFS		10
#define OSF1_MOUNT_FFM		11
#define OSF1_MOUNT_FDFS		12
#define OSF1_MOUNT_ADDON	13
#define OSF1_MOUNT_NFS3		14
#define OSF1_MNAMELEN		90

struct osf1_mfs_args {
	osf1_data_ptr	name;
	osf1_data_ptr	base;
	osf1_u_int	size;
};

struct osf1_nfs_args {
	osf1_data_ptr	addr;
	osf1_data_ptr	fh;
	osf1_int	flags;
	osf1_int	wsize;
	osf1_int	rsize;
	osf1_int	timeo;
	osf1_int	retrans;
	osf1_data_ptr	hostname;
	osf1_int	acregmin;
	osf1_int	acregmax;
	osf1_int	acdirmin;
	osf1_int	acdirmax;
	osf1_data_ptr	netname;
	osf1_data_ptr	pathconf;
};

union osf1_mount_info {
	struct osf1_mfs_args mfs_args;
	struct osf1_nfs_args nfs_args;
	char		pad[80];
};

#define OSF1_NFSMNT_SOFT	0x00000001
#define OSF1_NFSMNT_WSIZE	0x00000002
#define OSF1_NFSMNT_RSIZE	0x00000004
#define OSF1_NFSMNT_TIMEO	0x00000008
#define OSF1_NFSMNT_RETRANS	0x00000010
#define OSF1_NFSMNT_HOSTNAME	0x00000020
#define OSF1_NFSMNT_INT		0x00000040
#define OSF1_NFSMNT_NOCONN	0x00000080
#define OSF1_NFSMNT_NOAC	0x00000100
#define OSF1_NFSMNT_ACREGMIN	0x00000200
#define OSF1_NFSMNT_ACREGMAX	0x00000400
#define OSF1_NFSMNT_ACDIRMIN	0x00000800
#define OSF1_NFSMNT_ACDIRMAX	0x00001000
#define OSF1_NFSMNT_NOCTO	0x00002000
#define OSF1_NFSMNT_POSIX	0x00004000
#define OSF1_NFSMNT_AUTO	0x00008000
#define OSF1_NFSMNT_SEC		0x00010000
#define OSF1_NFSMNT_TCP		0x00020000
#define OSF1_NFSMNT_PROPLIST	0x00040000

struct osf1_statfs {
	osf1_short	f_type;
	osf1_short	f_flags;
	osf1_int	f_fsize;
	osf1_int	f_bsize;
	osf1_int	f_blocks;
	osf1_int	f_bfree;
	osf1_int	f_bavail;
	osf1_int	f_files;
	osf1_int	f_ffree;
	osf1_fsid_t	f_fsid;
	osf1_int	f_spare[9];
	char		f_mntonname[OSF1_MNAMELEN];
	char		f_mntfromname[OSF1_MNAMELEN];
	union osf1_mount_info mount_info;
};
