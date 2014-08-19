/* $NetBSD: osf1_cvt.c,v 1.27.18.1 2014/08/20 00:03:33 tls Exp $ */

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

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_cvt.c,v 1.27.18.1 2014/08/20 00:03:33 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>
#include <uvm/uvm.h>				/* XXX see mmap emulation */

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

#include <compat/osf1/osf1.h>
#include <compat/common/compat_util.h>
#include <compat/osf1/osf1_cvt.h>

const struct emul_flags_xtab osf1_access_flags_xtab[] = {
#if 0 /* pseudo-flag */
    {	OSF1_F_OK,		OSF1_F_OK,		F_OK		},
#endif
    {	OSF1_X_OK,		OSF1_X_OK,		X_OK		},
    {	OSF1_W_OK,		OSF1_W_OK,		W_OK		},
    {	OSF1_R_OK,		OSF1_R_OK,		R_OK		},
    {	0								}
};

const struct emul_flags_xtab osf1_fcntl_getsetfd_flags_rxtab[] = {
    {	FD_CLOEXEC,		FD_CLOEXEC,		OSF1_FD_CLOEXEC	},
    {	0								}
};

const struct emul_flags_xtab osf1_fcntl_getsetfd_flags_xtab[] = {
    {	OSF1_FD_CLOEXEC,	OSF1_FD_CLOEXEC,	FD_CLOEXEC	},
    {	0								}
};

/* flags specific to GETFL/SETFL; also uses open rxtab */
const struct emul_flags_xtab osf1_fcntl_getsetfl_flags_rxtab[] = {
    {	FASYNC,			FASYNC,			OSF1_FASYNC	},
    {	0								}
};

/* flags specific to GETFL/SETFL; also uses open xtab */
const struct emul_flags_xtab osf1_fcntl_getsetfl_flags_xtab[] = {
    {	OSF1_FASYNC,		OSF1_FASYNC,		FASYNC		},
    {	0								}
};

const struct emul_flags_xtab osf1_mmap_flags_xtab[] = {
    {	OSF1_MAP_SHARED,	OSF1_MAP_SHARED,	MAP_SHARED	},
    {	OSF1_MAP_PRIVATE,	OSF1_MAP_PRIVATE,	MAP_PRIVATE	},
    {	OSF1_MAP_TYPE,		OSF1_MAP_FILE,		MAP_FILE	},
    {	OSF1_MAP_TYPE,		OSF1_MAP_ANON,		MAP_ANON	},
    {	OSF1_MAP_FIXED,		OSF1_MAP_FIXED,		MAP_FIXED	},
#if 0 /* pseudo-flag, and the default */
    {	OSF1_MAP_VARIABLE,	OSF1_MAP_VARIABLE,	0		},
#endif
    {	OSF1_MAP_HASSEMAPHORE,	OSF1_MAP_HASSEMAPHORE,	MAP_HASSEMAPHORE },
    {	OSF1_MAP_INHERIT,	OSF1_MAP_INHERIT,	MAP_INHERIT	},
#if 0 /* no equivalent +++ */
    {	OSF1_MAP_UNALIGNED,	OSF1_MAP_UNALIGNED,	???		},
#endif
    {	0								}
};

const struct emul_flags_xtab osf1_mmap_prot_xtab[] = {
#if 0 /* pseudo-flag */
    {	OSF1_PROT_NONE,		OSF1_PROT_NONE,		PROT_NONE	},
#endif
    {	OSF1_PROT_READ,		OSF1_PROT_READ,		PROT_READ	},
    {	OSF1_PROT_WRITE,	OSF1_PROT_WRITE,	PROT_READ|PROT_WRITE },
    {	OSF1_PROT_EXEC,		OSF1_PROT_EXEC,		PROT_READ|PROT_EXEC },
    {	0								}
};

const struct emul_flags_xtab osf1_nfs_mount_flags_xtab[] = {
    {	OSF1_NFSMNT_SOFT,	OSF1_NFSMNT_SOFT,	NFSMNT_SOFT,	},
    {	OSF1_NFSMNT_WSIZE,	OSF1_NFSMNT_WSIZE,	NFSMNT_WSIZE,	},
    {	OSF1_NFSMNT_RSIZE,	OSF1_NFSMNT_RSIZE,	NFSMNT_RSIZE,	},
    {	OSF1_NFSMNT_TIMEO,	OSF1_NFSMNT_TIMEO,	NFSMNT_TIMEO,	},
    {	OSF1_NFSMNT_RETRANS,	OSF1_NFSMNT_RETRANS,	NFSMNT_RETRANS,	},
#if 0 /* no equivalent; needs special handling, see below */
    {	OSF1_NFSMNT_HOSTNAME,	OSF1_NFSMNT_HOSTNAME,	???,		},
#endif
    {	OSF1_NFSMNT_INT,	OSF1_NFSMNT_INT,	NFSMNT_INT,	},
    {	OSF1_NFSMNT_NOCONN,	OSF1_NFSMNT_NOCONN,	NFSMNT_NOCONN,	},
#if 0 /* no equivalents */
    {	OSF1_NFSMNT_NOAC,	OSF1_NFSMNT_NOAC,	???,		},
    {	OSF1_NFSMNT_ACREGMIN,	OSF1_NFSMNT_ACREGMIN,	???,		},
    {	OSF1_NFSMNT_ACREGMAX,	OSF1_NFSMNT_ACREGMAX,	???,		},
    {	OSF1_NFSMNT_ACDIRMIN,	OSF1_NFSMNT_ACDIRMIN,	???,		},
    {	OSF1_NFSMNT_ACDIRMAX,	OSF1_NFSMNT_ACDIRMAX,	???,		},
    {	OSF1_NFSMNT_NOCTO,	OSF1_NFSMNT_NOCTO,	???,		},
    {	OSF1_NFSMNT_POSIX,	OSF1_NFSMNT_POSIX,	???,		},
    {	OSF1_NFSMNT_AUTO,	OSF1_NFSMNT_AUTO,	???,		},
    {	OSF1_NFSMNT_SEC,	OSF1_NFSMNT_SEC,	???,		},
    {	OSF1_NFSMNT_TCP,	OSF1_NFSMNT_TCP,	???,		},
    {	OSF1_NFSMNT_PROPLIST,	OSF1_NFSMNT_PROPLIST,	???,		},
#endif
    {	0								}
};

const struct emul_flags_xtab osf1_open_flags_rxtab[] = {
    {	O_ACCMODE,		O_RDONLY,		OSF1_O_RDONLY	},
    {	O_ACCMODE,		O_WRONLY,		OSF1_O_WRONLY	},
    {	O_ACCMODE,		O_RDWR,			OSF1_O_RDWR	},
    {	O_NONBLOCK,		O_NONBLOCK,		OSF1_O_NONBLOCK	},
    {	O_APPEND,		O_APPEND,		OSF1_O_APPEND	},
#if 0 /* no equivalent +++ */
    {	???,			???,			O_DEFER		},
#endif
    {	O_CREAT,		O_CREAT,		OSF1_O_CREAT	},
    {	O_TRUNC,		O_TRUNC,		OSF1_O_TRUNC	},
    {	O_EXCL,			O_EXCL,			OSF1_O_EXCL	},
    {	O_NOCTTY,		O_NOCTTY,		OSF1_O_NOCTTY	},
    {	O_SYNC,			O_SYNC,			OSF1_O_SYNC	},
    {	O_NDELAY,		O_NDELAY,		OSF1_O_NDELAY	},
#if 0 /* no equivalent, also same value as O_NDELAY! */
    {	???,			???,			O_DRD		},
#endif
    {	O_DSYNC,		O_DSYNC,		OSF1_O_DSYNC	},
    {	O_RSYNC,		O_RSYNC,		OSF1_O_RSYNC	},
    {	0								}
};

const struct emul_flags_xtab osf1_open_flags_xtab[] = {
    {	OSF1_O_ACCMODE,		OSF1_O_RDONLY,		O_RDONLY	},
    {	OSF1_O_ACCMODE,		OSF1_O_WRONLY,		O_WRONLY	},
    {	OSF1_O_ACCMODE,		OSF1_O_RDWR,		O_RDWR		},
    {	OSF1_O_NONBLOCK,	OSF1_O_NONBLOCK,	O_NONBLOCK	},
    {	OSF1_O_APPEND,		OSF1_O_APPEND,		O_APPEND	},
#if 0 /* no equivalent +++ */
    {	OSF1_O_DEFER,		OSF1_O_DEFER,		???		},
#endif
    {	OSF1_O_CREAT,		OSF1_O_CREAT,		O_CREAT		},
    {	OSF1_O_TRUNC,		OSF1_O_TRUNC,		O_TRUNC		},
    {	OSF1_O_EXCL,		OSF1_O_EXCL,		O_EXCL		},
    {	OSF1_O_NOCTTY,		OSF1_O_NOCTTY,		O_NOCTTY	},
    {	OSF1_O_SYNC,		OSF1_O_SYNC,		O_SYNC		},
    {	OSF1_O_NDELAY,		OSF1_O_NDELAY,		O_NDELAY	},
#if 0 /* no equivalent, also same value as O_NDELAY! */
    {	OSF1_O_DRD,		OSF1_O_DRD,		???		},
#endif
    {	OSF1_O_DSYNC,		OSF1_O_DSYNC,		O_DSYNC		},
    {	OSF1_O_RSYNC,		OSF1_O_RSYNC,		O_RSYNC		},
    {	0								}
};

const struct emul_flags_xtab osf1_reboot_opt_xtab[] = {
#if 0 /* pseudo-flag */
    {	OSF1_RB_AUTOBOOT,	OSF1_RB_AUTOBOOT,	RB_AUTOBOOT	},
#endif
    {	OSF1_RB_ASKNAME,	OSF1_RB_ASKNAME,	RB_ASKNAME	},
    {	OSF1_RB_SINGLE,		OSF1_RB_SINGLE,		RB_SINGLE	},
    {	OSF1_RB_NOSYNC,		OSF1_RB_NOSYNC,		RB_NOSYNC	},
#if 0 /* same value as O_NDELAY, only used at boot time? */
    {	OSF1_RB_KDB,		OSF1_RB_KDB,		RB_KDB		},
#endif
    {	OSF1_RB_HALT,		OSF1_RB_HALT,		RB_HALT		},
    {	OSF1_RB_INITNAME,	OSF1_RB_INITNAME,	RB_INITNAME	},
#if 0 /* no equivalents +++ */
    {	OSF1_RB_DFLTROOT,	OSF1_RB_DFLTROOT,	???		},
    {	OSF1_RB_ALTBOOT,	OSF1_RB_ALTBOOT,	???		},
    {	OSF1_RB_UNIPROC,	OSF1_RB_UNIPROC,	???		},
    {	OSF1_RB_PARAM,		OSF1_RB_PARAM,		???		},
#endif
    {	OSF1_RB_DUMP,		OSF1_RB_DUMP,		RB_DUMP		},
    {	0								}
};

const struct emul_flags_xtab osf1_sendrecv_msg_flags_xtab[] = {
    {	OSF1_MSG_OOB,		OSF1_MSG_OOB,		MSG_OOB		},
    {	OSF1_MSG_PEEK,		OSF1_MSG_PEEK,		MSG_PEEK	},
    {	OSF1_MSG_DONTROUTE,	OSF1_MSG_DONTROUTE,	MSG_DONTROUTE	},
    {	OSF1_MSG_EOR,		OSF1_MSG_EOR,		MSG_EOR		},
    {	OSF1_MSG_TRUNC,		OSF1_MSG_TRUNC,		MSG_TRUNC	},
    {	OSF1_MSG_CTRUNC,	OSF1_MSG_CTRUNC,	MSG_CTRUNC	},
    {	OSF1_MSG_WAITALL,	OSF1_MSG_WAITALL,	MSG_WAITALL	},
    {	0								}
};

const struct emul_flags_xtab osf1_sigaction_flags_rxtab[] = {
    {	SA_ONSTACK,		SA_ONSTACK,		OSF1_SA_ONSTACK	},
    {	SA_RESTART,		SA_RESTART,		OSF1_SA_RESTART	},
    {	SA_NOCLDSTOP,		SA_NOCLDSTOP,		OSF1_SA_NOCLDSTOP },
    {	SA_NODEFER,		SA_NODEFER,		OSF1_SA_NODEFER	},
    {	SA_RESETHAND,		SA_RESETHAND,		OSF1_SA_RESETHAND },
    {	SA_NOCLDWAIT,		SA_NOCLDWAIT,		OSF1_SA_NOCLDWAIT },
#if 0 /* XXX not yet */
    {	SA_SIGINFO,		SA_SIGINFO,		OSF1_SA_SIGINFO	},
#endif
    {	0								},
};

const struct emul_flags_xtab osf1_sigaction_flags_xtab[] = {
    {	OSF1_SA_ONSTACK,	OSF1_SA_ONSTACK,	SA_ONSTACK	},
    {	OSF1_SA_RESTART,	OSF1_SA_RESTART,	SA_RESTART	},
    {	OSF1_SA_NOCLDSTOP,	OSF1_SA_NOCLDSTOP,	SA_NOCLDSTOP	},
    {	OSF1_SA_NODEFER,	OSF1_SA_NODEFER,	SA_NODEFER	},
    {	OSF1_SA_RESETHAND,	OSF1_SA_RESETHAND,	SA_RESETHAND	},
    {	OSF1_SA_NOCLDWAIT,	OSF1_SA_NOCLDWAIT,	SA_NOCLDWAIT	},
#if 0 /* XXX not yet */
    {	OSF1_SA_SIGINFO,	OSF1_SA_SIGINFO,	SA_SIGINFO	},
#endif
    {	0								},
};

const struct emul_flags_xtab osf1_wait_options_xtab[] = {
    {	OSF1_WNOHANG,		OSF1_WNOHANG,		WNOHANG		},
    {	OSF1_WUNTRACED,		OSF1_WUNTRACED,		WUNTRACED	},
    {	0								}
};

void
osf1_cvt_flock_from_native(const struct flock *nf, struct osf1_flock *of)
{

	memset(of, 0, sizeof(*of));

	of->l_start = nf->l_start;
	of->l_len = nf->l_len;
	of->l_pid = nf->l_pid;

	switch (nf->l_type) {
	case F_RDLCK:
		of->l_type = OSF1_F_RDLCK;
		break;

	case F_WRLCK:
		of->l_type = OSF1_F_WRLCK;
		break;

	case F_UNLCK:
		of->l_type = OSF1_F_UNLCK;
		break;
	}

	switch (nf->l_whence) {
	case SEEK_SET:
		of->l_whence = OSF1_SEEK_SET;
		break;

	case SEEK_CUR:
		of->l_whence = OSF1_SEEK_CUR;
		break;

	case SEEK_END:
		of->l_whence = OSF1_SEEK_END;
		break;
	}
}

int
osf1_cvt_flock_to_native(const struct osf1_flock *of, struct flock *nf)
{

	memset(nf, 0, sizeof(*nf));

	nf->l_start = of->l_start;
	nf->l_len = of->l_len;
	nf->l_pid = of->l_pid;

	switch (of->l_type) {
	case OSF1_F_RDLCK:
		nf->l_type = F_RDLCK;
		break;

	case OSF1_F_WRLCK:
		nf->l_type = F_WRLCK;
		break;

	case OSF1_F_UNLCK:
		nf->l_type = F_UNLCK;
		break;

	default:
		return (EINVAL);
	}

	switch (of->l_whence) {
	case OSF1_SEEK_SET:
		nf->l_whence = SEEK_SET;
		break;

	case OSF1_SEEK_CUR:
		nf->l_whence = SEEK_CUR;
		break;

	case OSF1_SEEK_END:
		nf->l_whence = SEEK_END;
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

int
osf1_cvt_msghdr_xopen_to_native(const struct osf1_msghdr_xopen *omh, struct msghdr *bmh)
{
	unsigned long leftovers;

	memset(bmh, 0, sizeof(*bmh));
	bmh->msg_name = omh->msg_name;		/* XXX sockaddr translation */
	bmh->msg_namelen = omh->msg_namelen;
	bmh->msg_iov = NULL;			/* iovec xlation separate */
	bmh->msg_iovlen = omh->msg_iovlen;

	/* XXX we don't translate control messages (yet) */
	if (bmh->msg_control != NULL || bmh->msg_controllen != 0)
{
printf("osf1_cvt_msghdr_xopen_to_native: control\n");
		return (EINVAL);
}

        /* translate flags */
        bmh->msg_flags = emul_flags_translate(osf1_sendrecv_msg_flags_xtab,
            omh->msg_flags, &leftovers);
        if (leftovers != 0)
{
printf("osf1_cvt_msghdr_xopen_to_native: leftovers 0x%lx\n", leftovers);
                return (EINVAL);
}

	return (0);
}

int
osf1_cvt_pathconf_name_to_native(int oname, int *bnamep)
{
	int error;

	error  = 0;
	switch (oname) {
	case OSF1__PC_CHOWN_RESTRICTED:
		*bnamep = _PC_CHOWN_RESTRICTED;
		break;

	case OSF1__PC_LINK_MAX:
		*bnamep = _PC_LINK_MAX;
		break;

	case OSF1__PC_MAX_CANON:
		*bnamep = _PC_MAX_CANON;
		break;

	case OSF1__PC_MAX_INPUT:
		*bnamep = _PC_MAX_INPUT;
		break;

	case OSF1__PC_NAME_MAX:
		*bnamep = _PC_NAME_MAX;
		break;

	case OSF1__PC_NO_TRUNC:
		*bnamep = _PC_NO_TRUNC;
		break;

	case OSF1__PC_PATH_MAX:
		*bnamep = _PC_PATH_MAX;
		break;

	case OSF1__PC_PIPE_BUF:
		*bnamep = _PC_PIPE_BUF;
		break;

	case OSF1__PC_VDISABLE:
		*bnamep = _PC_VDISABLE;
		break;

	default:
		error = EINVAL;
		break;
	}

	return (error);
}

/*
 * Convert from as rusage structure to an osf1 rusage structure.
 */
void
osf1_cvt_rusage_from_native(const struct rusage *ru, struct osf1_rusage *oru)
{

	oru->ru_utime.tv_sec = ru->ru_utime.tv_sec;
	oru->ru_utime.tv_usec = ru->ru_utime.tv_usec;

	oru->ru_stime.tv_sec = ru->ru_stime.tv_sec;
	oru->ru_stime.tv_usec = ru->ru_stime.tv_usec;

	oru->ru_maxrss = ru->ru_maxrss;
	oru->ru_ixrss = ru->ru_ixrss;
	oru->ru_idrss = ru->ru_idrss;
	oru->ru_isrss = ru->ru_isrss;
	oru->ru_minflt = ru->ru_minflt;
	oru->ru_majflt = ru->ru_majflt;
	oru->ru_nswap = ru->ru_nswap;
	oru->ru_inblock = ru->ru_inblock;
	oru->ru_oublock = ru->ru_oublock;
	oru->ru_msgsnd = ru->ru_msgsnd;
	oru->ru_msgrcv = ru->ru_msgrcv;
	oru->ru_nsignals = ru->ru_nsignals;
	oru->ru_nvcsw = ru->ru_nvcsw;
	oru->ru_nivcsw = ru->ru_nivcsw;
}

/*
 * XXX: Only a subset of the flags is currently implemented.
 */
void
osf1_cvt_sigaction_from_native(const struct sigaction *bsa, struct osf1_sigaction *osa)
{

	osa->osf1_sa_handler = bsa->sa_handler;
	osf1_cvt_sigset_from_native(&bsa->sa_mask, &osa->osf1_sa_mask);

        /* translate flags */
	osa->osf1_sa_flags = emul_flags_translate(osf1_sigaction_flags_rxtab,
            bsa->sa_flags, NULL);
}

int
osf1_cvt_sigaction_to_native(const struct osf1_sigaction *osa, struct sigaction *bsa)
{

	bsa->sa_handler = osa->osf1_sa_handler;
	osf1_cvt_sigset_to_native(&osa->osf1_sa_mask, &bsa->sa_mask);

        /* translate flags */
	bsa->sa_flags = emul_flags_translate(osf1_sigaction_flags_xtab,
            osa->osf1_sa_flags, NULL);
	/* XXX error if we can't translate */

	return (0);
}

void
osf1_cvt_sigset_from_native(const sigset_t *bss, osf1_sigset_t *oss)
{
	int i, newsig;

	osf1_sigemptyset(oss);
	for (i = 1; i < NSIG; i++) {
		if (sigismember(bss, i)) {
			newsig = native_to_osf1_signo[i];
			if (newsig)
				osf1_sigaddset(oss, newsig);
		}
	}
}

int
osf1_cvt_sigset_to_native(const osf1_sigset_t *oss, sigset_t *bss)
{
	int i, newsig;

	sigemptyset(bss);
	for (i = 1; i < OSF1_NSIG; i++) {
		if (osf1_sigismember(oss, i)) {
			newsig = osf1_to_native_signo[i];
			if (newsig)
				sigaddset(bss, newsig);
		}
	}
	return (0);
}

/*
 * Convert from a stat structure to an osf1 stat structure.
 */
void
osf1_cvt_stat_from_native(const struct stat *st, struct osf1_stat *ost)
{

	ost->st_dev = osf1_cvt_dev_from_native(st->st_dev);
	ost->st_ino = st->st_ino;
	ost->st_mode = st->st_mode;
	ost->st_nlink = st->st_nlink;
	ost->st_uid = st->st_uid == -2 ? (u_int16_t) -2 : st->st_uid;
	ost->st_gid = st->st_gid == -2 ? (u_int16_t) -2 : st->st_gid;
	ost->st_rdev = osf1_cvt_dev_from_native(st->st_rdev);
	ost->st_size = st->st_size;
	ost->st_atime_sec = st->st_atime;
	ost->st_spare1 = 0;
	ost->st_mtime_sec = st->st_mtime;
	ost->st_spare2 = 0;
	ost->st_ctime_sec = st->st_ctime;
	ost->st_spare3 = 0;
	ost->st_blksize = st->st_blksize;
	ost->st_blocks = st->st_blocks;
	ost->st_flags = st->st_flags;
	ost->st_gen = st->st_gen;
}

/*
 * Convert from a stat structure to an osf1 stat structure.
 */
void
osf1_cvt_stat2_from_native(const struct stat *st, struct osf1_stat2 *ost)
{

	memset(ost, 0, sizeof *ost);
	ost->st_dev = osf1_cvt_dev_from_native(st->st_dev);
	ost->st_ino = st->st_ino;
	ost->st_mode = st->st_mode;
	ost->st_nlink = st->st_nlink;
	ost->st_uid = st->st_uid == -2 ? (u_int16_t) -2 : st->st_uid;
	ost->st_gid = st->st_gid == -2 ? (u_int16_t) -2 : st->st_gid;
	ost->st_rdev = osf1_cvt_dev_from_native(st->st_rdev);
	ost->st_size = st->st_size;
	ost->st_atime_sec = st->st_atime;
	ost->st_mtime_sec = st->st_mtime;
	ost->st_ctime_sec = st->st_ctime;
	ost->st_blocksize = st->st_blksize;
	ost->st_blocks = st->st_blocks;
	ost->st_flags = st->st_flags;
	ost->st_generation = st->st_gen;
}

void
osf1_cvt_statfs_from_native(const struct statvfs *bsfs, struct osf1_statfs *osfs)
{

	memset(osfs, 0, sizeof (struct osf1_statfs));
	if (!strncmp(MOUNT_FFS, bsfs->f_fstypename, sizeof(bsfs->f_fstypename)))
		osfs->f_type = OSF1_MOUNT_UFS;
	else if (!strncmp(MOUNT_NFS, bsfs->f_fstypename, sizeof(bsfs->f_fstypename)))
		osfs->f_type = OSF1_MOUNT_NFS;
	else if (!strncmp(MOUNT_MFS, bsfs->f_fstypename, sizeof(bsfs->f_fstypename)))
		osfs->f_type = OSF1_MOUNT_MFS;
	else
		/* uh oh...  XXX = PC, CDFS, PROCFS, etc. */
		osfs->f_type = OSF1_MOUNT_ADDON;
	osfs->f_flags = bsfs->f_flag;		/* XXX translate */
	osfs->f_fsize = bsfs->f_frsize;
	osfs->f_bsize = bsfs->f_bsize;
	osfs->f_blocks = bsfs->f_blocks;
	osfs->f_bfree = bsfs->f_bfree;
	osfs->f_bavail = bsfs->f_bavail;
	osfs->f_files = bsfs->f_files;
	osfs->f_ffree = bsfs->f_ffree;
	memcpy(&osfs->f_fsid, &bsfs->f_fsidx, sizeof osfs->f_fsid);
	/* osfs->f_spare zeroed above */
	memcpy(osfs->f_mntonname, bsfs->f_mntonname, sizeof osfs->f_mntonname);
	memcpy(osfs->f_mntfromname, bsfs->f_mntfromname,
	    sizeof osfs->f_mntfromname);
	/* XXX osfs->f_xxx should be filled in... */
}
