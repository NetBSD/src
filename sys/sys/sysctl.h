/*	$NetBSD: sysctl.h,v 1.38.8.1 1999/12/27 18:36:35 wrstuden Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
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
 *	@(#)sysctl.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _SYS_SYSCTL_H_
#define	_SYS_SYSCTL_H_

/*
 * These are for the eproc structure defined below.
 */
#ifndef _KERNEL
#include <sys/time.h>
#include <sys/ucred.h>
#include <sys/proc.h>
#include <vm/vm.h>
#endif

/*
 * Definitions for sysctl call.  The sysctl call uses a hierarchical name
 * for objects that can be examined or modified.  The name is expressed as
 * a sequence of integers.  Like a file path name, the meaning of each
 * component depends on its place in the hierarchy.  The top-level and kern
 * identifiers are defined here, and other identifiers are defined in the
 * respective subsystem header files.
 */

#define CTL_MAXNAME	12	/* largest number of components supported */

/*
 * Each subsystem defined by sysctl defines a list of variables
 * for that subsystem. Each name is either a node with further
 * levels defined below it, or it is a leaf of some particular
 * type given below. Each sysctl level defines a set of name/type
 * pairs to be used by sysctl(1) in manipulating the subsystem.
 */
struct ctlname {
	char	*ctl_name;	/* subsystem name */
	int	ctl_type;	/* type of name */
};
#define	CTLTYPE_NODE	1	/* name is a node */
#define	CTLTYPE_INT	2	/* name describes an integer */
#define	CTLTYPE_STRING	3	/* name describes a string */
#define	CTLTYPE_QUAD	4	/* name describes a 64-bit number */
#define	CTLTYPE_STRUCT	5	/* name describes a structure */

/*
 * Top-level identifiers
 */
#define	CTL_UNSPEC	0		/* unused */
#define	CTL_KERN	1		/* "high kernel": proc, limits */
#define	CTL_VM		2		/* virtual memory */
#define	CTL_VFS		3		/* file system, mount type is next */
#define	CTL_NET		4		/* network, see socket.h */
#define	CTL_DEBUG	5		/* debugging parameters */
#define	CTL_HW		6		/* generic cpu/io */
#define	CTL_MACHDEP	7		/* machine dependent */
#define	CTL_USER	8		/* user-level */
#define	CTL_DDB		9		/* in-kernel debugger */
#define	CTL_PROC	10		/* per-proc attr */
#define	CTL_MAXID	11		/* number of valid top-level ids */

#define CTL_NAMES { \
	{ 0, 0 }, \
	{ "kern", CTLTYPE_NODE }, \
	{ "vm", CTLTYPE_NODE }, \
	{ "vfs", CTLTYPE_NODE }, \
	{ "net", CTLTYPE_NODE }, \
	{ "debug", CTLTYPE_NODE }, \
	{ "hw", CTLTYPE_NODE }, \
	{ "machdep", CTLTYPE_NODE }, \
	{ "user", CTLTYPE_NODE }, \
	{ "ddb", CTLTYPE_NODE }, \
	{ "proc", CTLTYPE_NODE }, \
}

/*
 * CTL_KERN identifiers
 */
#define	KERN_OSTYPE	 	 1	/* string: system version */
#define	KERN_OSRELEASE	 	 2	/* string: system release */
#define	KERN_OSREV	 	 3	/* int: system revision */
#define	KERN_VERSION	 	 4	/* string: compile time info */
#define	KERN_MAXVNODES	 	 5	/* int: max vnodes */
#define	KERN_MAXPROC	 	 6	/* int: max processes */
#define	KERN_MAXFILES	 	 7	/* int: max open files */
#define	KERN_ARGMAX	 	 8	/* int: max arguments to exec */
#define	KERN_SECURELVL	 	 9	/* int: system security level */
#define	KERN_HOSTNAME		10	/* string: hostname */
#define	KERN_HOSTID		11	/* int: host identifier */
#define	KERN_CLOCKRATE		12	/* struct: struct clockrate */
#define	KERN_VNODE		13	/* struct: vnode structures */
#define	KERN_PROC		14	/* struct: process entries */
#define	KERN_FILE		15	/* struct: file entries */
#define	KERN_PROF		16	/* node: kernel profiling info */
#define	KERN_POSIX1		17	/* int: POSIX.1 version */
#define	KERN_NGROUPS		18	/* int: # of supplemental group ids */
#define	KERN_JOB_CONTROL	19	/* int: is job control available */
#define	KERN_SAVED_IDS		20	/* int: saved set-user/group-ID */
#define	KERN_BOOTTIME		21	/* struct: time kernel was booted */
#define	KERN_DOMAINNAME		22	/* string: (YP) domainname */
#define	KERN_MAXPARTITIONS	23	/* int: number of partitions/disk */
#define KERN_RAWPARTITION	24	/* int: raw partition number */
#define	KERN_NTPTIME		25	/* struct: extended-precision time */
#define	KERN_TIMEX		26	/* struct: ntp timekeeping state */
#define KERN_AUTONICETIME	27	/* int: proc time before autonice */
#define KERN_AUTONICEVAL	28	/* int: auto nice value */
#define	KERN_RTC_OFFSET		29	/* int: offset of rtc from gmt */
#define	KERN_ROOT_DEVICE	30	/* string: root device */
#define	KERN_MSGBUFSIZE		31	/* int: max # of chars in msg buffer */
#define	KERN_FSYNC		32	/* int: file synchronization support */
#define	KERN_SYSVMSG		33	/* int: SysV message queue suppoprt */
#define	KERN_SYSVSEM		34	/* int: SysV semaphore support */
#define	KERN_SYSVSHM		35	/* int: SysV shared memory support */
#define	KERN_OLDSHORTCORENAME	36	/* old, unimplemented */
#define	KERN_SYNCHRONIZED_IO	37	/* int: POSIX synchronized I/O */
#define	KERN_IOV_MAX		38	/* int: max iovec's for readv(2) etc. */
#define	KERN_MBUF		39	/* node: mbuf parameters */
#define	KERN_MAPPED_FILES	40	/* int: POSIX memory mapped files */
#define	KERN_MEMLOCK		41	/* int: POSIX memory locking */
#define	KERN_MEMLOCK_RANGE	42	/* int: POSIX memory range locking */
#define	KERN_MEMORY_PROTECTION	43	/* int: POSIX memory protections */
#define	KERN_LOGIN_NAME_MAX	44	/* int: max length login name + NUL */
#define	KERN_DEFCORENAME	45	/* old: sort core name format */
#define	KERN_MAXID		46	/* number of valid kern ids */

#define CTL_KERN_NAMES { \
	{ 0, 0 }, \
	{ "ostype", CTLTYPE_STRING }, \
	{ "osrelease", CTLTYPE_STRING }, \
	{ "osrevision", CTLTYPE_INT }, \
	{ "version", CTLTYPE_STRING }, \
	{ "maxvnodes", CTLTYPE_INT }, \
	{ "maxproc", CTLTYPE_INT }, \
	{ "maxfiles", CTLTYPE_INT }, \
	{ "argmax", CTLTYPE_INT }, \
	{ "securelevel", CTLTYPE_INT }, \
	{ "hostname", CTLTYPE_STRING }, \
	{ "hostid", CTLTYPE_INT }, \
	{ "clockrate", CTLTYPE_STRUCT }, \
	{ "vnode", CTLTYPE_STRUCT }, \
	{ "proc", CTLTYPE_STRUCT }, \
	{ "file", CTLTYPE_STRUCT }, \
	{ "profiling", CTLTYPE_NODE }, \
	{ "posix1version", CTLTYPE_INT }, \
	{ "ngroups", CTLTYPE_INT }, \
	{ "job_control", CTLTYPE_INT }, \
	{ "saved_ids", CTLTYPE_INT }, \
	{ "boottime", CTLTYPE_STRUCT }, \
	{ "domainname", CTLTYPE_STRING }, \
	{ "maxpartitions", CTLTYPE_INT }, \
	{ "rawpartition", CTLTYPE_INT }, \
	{ "ntptime", CTLTYPE_STRUCT }, \
	{ "timex", CTLTYPE_STRUCT }, \
	{ "autonicetime", CTLTYPE_INT }, \
	{ "autoniceval", CTLTYPE_INT }, \
	{ "rtc_offset", CTLTYPE_INT }, \
	{ "root_device", CTLTYPE_STRING }, \
	{ "msgbufsize", CTLTYPE_INT }, \
	{ "fsync", CTLTYPE_INT }, \
	{ "sysvmsg", CTLTYPE_INT }, \
	{ "sysvsem", CTLTYPE_INT }, \
	{ "sysvshm", CTLTYPE_INT }, \
	{ 0, 0 }, \
	{ "synchronized_io", CTLTYPE_INT }, \
	{ "iov_max", CTLTYPE_INT }, \
	{ "mbuf", CTLTYPE_NODE }, \
	{ "mapped_files", CTLTYPE_INT }, \
	{ "memlock", CTLTYPE_INT }, \
	{ "memlock_range", CTLTYPE_INT }, \
	{ "memory_protection", CTLTYPE_INT }, \
	{ "login_name_max", CTLTYPE_INT }, \
	{ "defcorename", CTLTYPE_STRING }, \
}

/*
 * KERN_PROC subtypes
 */
#define KERN_PROC_ALL		0	/* everything */
#define	KERN_PROC_PID		1	/* by process id */
#define	KERN_PROC_PGRP		2	/* by process group id */
#define	KERN_PROC_SESSION	3	/* by session of pid */
#define	KERN_PROC_TTY		4	/* by controlling tty */
#define	KERN_PROC_UID		5	/* by effective uid */
#define	KERN_PROC_RUID		6	/* by real uid */

/*
 * KERN_PROC subtype ops return arrays of augmented proc structures:
 */
struct kinfo_proc {
	struct	proc kp_proc;			/* proc structure */
	struct	eproc {
		struct	proc *e_paddr;		/* address of proc */
		struct	session *e_sess;	/* session pointer */
		struct	pcred e_pcred;		/* process credentials */
		struct	ucred e_ucred;		/* current credentials */
		struct	vmspace e_vm;		/* address space */
		pid_t	e_ppid;			/* parent process id */
		pid_t	e_pgid;			/* process group id */
		short	e_jobc;			/* job control counter */
		dev_t	e_tdev;			/* controlling tty dev */
		pid_t	e_tpgid;		/* tty process group id */
		struct	session *e_tsess;	/* tty session pointer */
#define	WMESGLEN	8
		char	e_wmesg[WMESGLEN];	/* wchan message */
		segsz_t e_xsize;		/* text size */
		short	e_xrssize;		/* text rss */
		short	e_xccount;		/* text references */
		short	e_xswrss;
		long	e_flag;
#define	EPROC_CTTY	0x01	/* controlling tty vnode active */
#define	EPROC_SLEADER	0x02	/* session leader */
		char	e_login[MAXLOGNAME];	/* setlogin() name */
		pid_t	e_sid;			/* session id */
		long	e_spare[3];
	} kp_eproc;
};

/*
 * CTL_HW identifiers
 */
#define	HW_MACHINE	 1		/* string: machine class */
#define	HW_MODEL	 2		/* string: specific machine model */
#define	HW_NCPU		 3		/* int: number of cpus */
#define	HW_BYTEORDER	 4		/* int: machine byte order */
#define	HW_PHYSMEM	 5		/* int: total memory */
#define	HW_USERMEM	 6		/* int: non-kernel memory */
#define	HW_PAGESIZE	 7		/* int: software page size */
#define	HW_DISKNAMES	 8		/* strings: disk drive names */
#define	HW_DISKSTATS	 9		/* struct: diskstats[] */
#define	HW_MACHINE_ARCH	10		/* string: machine architecture */
#define	HW_MAXID	11		/* number of valid hw ids */

#define CTL_HW_NAMES { \
	{ 0, 0 }, \
	{ "machine", CTLTYPE_STRING }, \
	{ "model", CTLTYPE_STRING }, \
	{ "ncpu", CTLTYPE_INT }, \
	{ "byteorder", CTLTYPE_INT }, \
	{ "physmem", CTLTYPE_INT }, \
	{ "usermem", CTLTYPE_INT }, \
	{ "pagesize", CTLTYPE_INT }, \
	{ "disknames", CTLTYPE_STRUCT }, \
	{ "diskstats", CTLTYPE_STRUCT }, \
	{ "machine_arch", CTLTYPE_STRING }, \
}

/*
 * CTL_USER definitions
 */
#define	USER_CS_PATH		 1	/* string: _CS_PATH */
#define	USER_BC_BASE_MAX	 2	/* int: BC_BASE_MAX */
#define	USER_BC_DIM_MAX		 3	/* int: BC_DIM_MAX */
#define	USER_BC_SCALE_MAX	 4	/* int: BC_SCALE_MAX */
#define	USER_BC_STRING_MAX	 5	/* int: BC_STRING_MAX */
#define	USER_COLL_WEIGHTS_MAX	 6	/* int: COLL_WEIGHTS_MAX */
#define	USER_EXPR_NEST_MAX	 7	/* int: EXPR_NEST_MAX */
#define	USER_LINE_MAX		 8	/* int: LINE_MAX */
#define	USER_RE_DUP_MAX		 9	/* int: RE_DUP_MAX */
#define	USER_POSIX2_VERSION	10	/* int: POSIX2_VERSION */
#define	USER_POSIX2_C_BIND	11	/* int: POSIX2_C_BIND */
#define	USER_POSIX2_C_DEV	12	/* int: POSIX2_C_DEV */
#define	USER_POSIX2_CHAR_TERM	13	/* int: POSIX2_CHAR_TERM */
#define	USER_POSIX2_FORT_DEV	14	/* int: POSIX2_FORT_DEV */
#define	USER_POSIX2_FORT_RUN	15	/* int: POSIX2_FORT_RUN */
#define	USER_POSIX2_LOCALEDEF	16	/* int: POSIX2_LOCALEDEF */
#define	USER_POSIX2_SW_DEV	17	/* int: POSIX2_SW_DEV */
#define	USER_POSIX2_UPE		18	/* int: POSIX2_UPE */
#define	USER_STREAM_MAX		19	/* int: POSIX2_STREAM_MAX */
#define	USER_TZNAME_MAX		20	/* int: POSIX2_TZNAME_MAX */
#define	USER_MAXID		21	/* number of valid user ids */

#define	CTL_USER_NAMES { \
	{ 0, 0 }, \
	{ "cs_path", CTLTYPE_STRING }, \
	{ "bc_base_max", CTLTYPE_INT }, \
	{ "bc_dim_max", CTLTYPE_INT }, \
	{ "bc_scale_max", CTLTYPE_INT }, \
	{ "bc_string_max", CTLTYPE_INT }, \
	{ "coll_weights_max", CTLTYPE_INT }, \
	{ "expr_nest_max", CTLTYPE_INT }, \
	{ "line_max", CTLTYPE_INT }, \
	{ "re_dup_max", CTLTYPE_INT }, \
	{ "posix2_version", CTLTYPE_INT }, \
	{ "posix2_c_bind", CTLTYPE_INT }, \
	{ "posix2_c_dev", CTLTYPE_INT }, \
	{ "posix2_char_term", CTLTYPE_INT }, \
	{ "posix2_fort_dev", CTLTYPE_INT }, \
	{ "posix2_fort_run", CTLTYPE_INT }, \
	{ "posix2_localedef", CTLTYPE_INT }, \
	{ "posix2_sw_dev", CTLTYPE_INT }, \
	{ "posix2_upe", CTLTYPE_INT }, \
	{ "stream_max", CTLTYPE_INT }, \
	{ "tzname_max", CTLTYPE_INT }, \
}

/*
 * CTL_DDB definitions
 */
#define	DDBCTL_RADIX		1	/* int: Input and output radix */
#define	DDBCTL_MAXOFF		2	/* int: max symbol offset */
#define	DDBCTL_MAXWIDTH		3	/* int: width of the display line */
#define	DDBCTL_LINES		4	/* int: number of display lines */
#define	DDBCTL_TABSTOPS		5	/* int: tab width */
#define	DDBCTL_ONPANIC		6	/* int: DDB on panic if non-zero */
#define	DDBCTL_FROMCONSOLE	7	/* int: DDB via console if non-zero */
#define	DDBCTL_MAXID		8	/* number of valid DDB ids */

#define	CTL_DDB_NAMES { \
	{ 0, 0 }, \
	{ "radix", CTLTYPE_INT }, \
	{ "maxoff", CTLTYPE_INT }, \
	{ "maxwidth", CTLTYPE_INT }, \
	{ "lines", CTLTYPE_INT }, \
	{ "tabstops", CTLTYPE_INT }, \
	{ "onpanic", CTLTYPE_INT }, \
	{ "fromconsole", CTLTYPE_INT }, \
}

/*
 * CTL_DEBUG definitions
 *
 * Second level identifier specifies which debug variable.
 * Third level identifier specifies which stucture component.
 */
#define	CTL_DEBUG_NAME		0	/* string: variable name */
#define	CTL_DEBUG_VALUE		1	/* int: variable value */
#define	CTL_DEBUG_MAXID		20

/*
 * CTL_PROC subtype. Either a PID, or a magic value for the current proc.
 */

#define PROC_CURPROC	(~((u_int)1 << 31))

/*
 * CTL_PROC tree: either corename (string), or a limit
 * (rlimit.<type>.{hard,soft}, int).
 */
#define PROC_PID_CORENAME	1
#define	PROC_PID_LIMIT		2
#define	PROC_PID_MAXID		3

#define	PROC_PID_NAMES { \
	{ 0, 0 }, \
	{ "corename", CTLTYPE_STRING }, \
	{ "rlimit", CTLTYPE_NODE }, \
}

/* Limit types from <sys/resources.h> */
#define PROC_PID_LIMIT_CPU	(RLIMIT_CPU+1)
#define PROC_PID_LIMIT_FSIZE	(RLIMIT_FSIZE+1)
#define PROC_PID_LIMIT_DATA	(RLIMIT_DATA+1)
#define PROC_PID_LIMIT_STACK	(RLIMIT_STACK+1)
#define PROC_PID_LIMIT_CORE	(RLIMIT_CORE+1)
#define PROC_PID_LIMIT_RSS	(RLIMIT_RSS+1)
#define PROC_PID_LIMIT_MEMLOCK	(RLIMIT_MEMLOCK+1)
#define PROC_PID_LIMIT_NPROC	(RLIMIT_NPROC+1)
#define PROC_PID_LIMIT_NOFILE	(RLIMIT_NOFILE+1)
#define PROC_PID_LIMIT_MAXID 	10

#define PROC_PID_LIMIT_NAMES { \
	{ 0, 0 }, \
	{ "cputime", CTLTYPE_NODE }, \
	{ "filesize", CTLTYPE_NODE }, \
	{ "datasize", CTLTYPE_NODE }, \
	{ "stacksize", CTLTYPE_NODE }, \
	{ "coredumpsize", CTLTYPE_NODE }, \
	{ "memoryuse", CTLTYPE_NODE }, \
	{ "memorylocked", CTLTYPE_NODE }, \
	{ "maxproc", CTLTYPE_NODE }, \
	{ "descriptors", CTLTYPE_NODE }, \
}
/* for each type, either hard or soft value */
#define PROC_PID_LIMIT_TYPE_SOFT	1
#define PROC_PID_LIMIT_TYPE_HARD	2
#define PROC_PID_LIMIT_TYPE_MAXID	3

#define PROC_PID_LIMIT_TYPE_NAMES { \
	{0, 0}, \
	{ "soft", CTLTYPE_QUAD }, \
	{ "hard", CTLTYPE_QUAD }, \
}

#ifdef	_KERNEL
/*
 * CTL_DEBUG variables.
 *
 * These are declared as separate variables so that they can be
 * individually initialized at the location of their associated
 * variable. The loader prevents multiple use by issuing errors
 * if a variable is initialized in more than one place. They are
 * aggregated into an array in debug_sysctl(), so that it can
 * conveniently locate them when querried. If more debugging
 * variables are added, they must also be declared here and also
 * entered into the array.
 */
struct ctldebug {
	char	*debugname;	/* name of debugging variable */
	int	*debugvar;	/* pointer to debugging variable */
};
#ifdef	DEBUG
extern struct ctldebug debug0, debug1, debug2, debug3, debug4;
extern struct ctldebug debug5, debug6, debug7, debug8, debug9;
extern struct ctldebug debug10, debug11, debug12, debug13, debug14;
extern struct ctldebug debug15, debug16, debug17, debug18, debug19;
#endif	/* DEBUG */

/*
 * Internal sysctl function calling convention:
 *
 *	(*sysctlfn)(name, namelen, oldval, oldlenp, newval, newlen);
 *
 * The name parameter points at the next component of the name to be
 * interpreted.  The namelen parameter is the number of integers in
 * the name.
 */
typedef int (sysctlfn)
    __P((int *, u_int, void *, size_t *, void *, size_t, struct proc *));

int sysctl_int __P((void *, size_t *, void *, size_t, int *));
int sysctl_rdint __P((void *, size_t *, void *, int));
int sysctl_quad __P((void *, size_t *, void *, size_t, quad_t *));
int sysctl_rdquad __P((void *, size_t *, void *, quad_t));
int sysctl_string __P((void *, size_t *, void *, size_t, char *, int));
int sysctl_rdstring __P((void *, size_t *, void *, char *));
int sysctl_rdstruct __P((void *, size_t *, void *, void *, int));
int sysctl_struct __P((void *, size_t *, void *, size_t, void *, int));
int sysctl_file __P((char *, size_t *));
int sysctl_doeproc __P((int *, u_int, char *, size_t *));
struct radix_node;
struct walkarg;
int sysctl_dumpentry __P((struct radix_node *, void *));
int sysctl_iflist __P((int, struct walkarg *, int));
int sysctl_rtable __P((int *, u_int, void *, size_t *, void *, size_t));
int sysctl_clockrate __P((char *, size_t *));
int sysctl_rdstring __P((void *, size_t *, void *, char *));
int sysctl_rdstruct __P((void *, size_t *, void *, void *, int));
int sysctl_vnode __P((char *, size_t *, struct proc *));
int sysctl_ntptime __P((char *, size_t *));
#ifdef GPROF
int sysctl_doprof __P((int *, u_int, void *, size_t *, void *, size_t));
#endif
int sysctl_dombuf __P((int *, u_int, void *, size_t *, void *, size_t));

void fill_eproc __P((struct proc *, struct eproc *));

int kern_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		     struct proc *));
int hw_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		   struct proc *));
int proc_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		     struct proc *));
#ifdef DEBUG
int debug_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		      struct proc *));
#endif
int vm_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		   struct proc *));
int fs_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		   struct proc *));
int net_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		    struct proc *));
int cpu_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		    struct proc *));

/* ddb_sysctl() declared in ddb_var.h */

#else	/* !_KERNEL */
#include <sys/cdefs.h>

__BEGIN_DECLS
int	sysctl __P((int *, u_int, void *, size_t *, const void *, size_t));
__END_DECLS
#endif	/* _KERNEL */
#endif	/* !_SYS_SYSCTL_H_ */
