/*	$NetBSD: linux_misc.c,v 1.230.2.2 2016/10/05 20:55:38 skrll Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 1999, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz; by Jason R. Thorpe
 * of the Numerical Aerospace Simulation Facility, NASA Ames Research Center.
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

/*
 * Linux compatibility module. Try to deal with various Linux system calls.
 */

/*
 * These functions have been moved to multiarch to allow
 * selection of which machines include them to be
 * determined by the individual files.linux_<arch> files.
 *
 * Function in multiarch:
 *	linux_sys_break			: linux_break.c
 *	linux_sys_alarm			: linux_misc_notalpha.c
 *	linux_sys_getresgid		: linux_misc_notalpha.c
 *	linux_sys_nice			: linux_misc_notalpha.c
 *	linux_sys_readdir		: linux_misc_notalpha.c
 *	linux_sys_setresgid		: linux_misc_notalpha.c
 *	linux_sys_time			: linux_misc_notalpha.c
 *	linux_sys_utime			: linux_misc_notalpha.c
 *	linux_sys_waitpid		: linux_misc_notalpha.c
 *	linux_sys_old_mmap		: linux_oldmmap.c
 *	linux_sys_oldolduname		: linux_oldolduname.c
 *	linux_sys_oldselect		: linux_oldselect.c
 *	linux_sys_olduname		: linux_olduname.c
 *	linux_sys_pipe			: linux_pipe.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_misc.c,v 1.230.2.2 2016/10/05 20:55:38 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/poll.h>
#include <sys/prot.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>
#include <sys/vfs_syscalls.h>
#include <sys/swap.h>		/* for SWAP_ON */
#include <sys/sysctl.h>		/* for KERN_DOMAINNAME */
#include <sys/kauth.h>

#include <sys/ptrace.h>
#include <machine/ptrace.h>

#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include <compat/sys/resource.h>

#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_mmap.h>
#include <compat/linux/common/linux_dirent.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_statfs.h>
#include <compat/linux/common/linux_limit.h>
#include <compat/linux/common/linux_ptrace.h>
#include <compat/linux/common/linux_reboot.h>
#include <compat/linux/common/linux_emuldata.h>
#include <compat/linux/common/linux_sched.h>

#include <compat/linux/linux_syscallargs.h>

const int linux_ptrace_request_map[] = {
	LINUX_PTRACE_TRACEME,	PT_TRACE_ME,
	LINUX_PTRACE_PEEKTEXT,	PT_READ_I,
	LINUX_PTRACE_PEEKDATA,	PT_READ_D,
	LINUX_PTRACE_POKETEXT,	PT_WRITE_I,
	LINUX_PTRACE_POKEDATA,	PT_WRITE_D,
	LINUX_PTRACE_CONT,	PT_CONTINUE,
	LINUX_PTRACE_KILL,	PT_KILL,
	LINUX_PTRACE_ATTACH,	PT_ATTACH,
	LINUX_PTRACE_DETACH,	PT_DETACH,
# ifdef PT_STEP
	LINUX_PTRACE_SINGLESTEP,	PT_STEP,
# endif
	LINUX_PTRACE_SYSCALL,	PT_SYSCALL,
	-1
};

const struct linux_mnttypes linux_fstypes[] = {
	{ MOUNT_FFS,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_NFS,		LINUX_NFS_SUPER_MAGIC 		},
	{ MOUNT_MFS,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_MSDOS,		LINUX_MSDOS_SUPER_MAGIC		},
	{ MOUNT_LFS,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_FDESC,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_NULL,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_OVERLAY,	LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_UMAP,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_KERNFS,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_PROCFS,		LINUX_PROC_SUPER_MAGIC		},
	{ MOUNT_AFS,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_CD9660,		LINUX_ISOFS_SUPER_MAGIC		},
	{ MOUNT_UNION,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_ADOSFS,		LINUX_ADFS_SUPER_MAGIC		},
	{ MOUNT_EXT2FS,		LINUX_EXT2_SUPER_MAGIC		},
	{ MOUNT_CFS,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_CODA,		LINUX_CODA_SUPER_MAGIC		},
	{ MOUNT_FILECORE,	LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_NTFS,		LINUX_DEFAULT_SUPER_MAGIC	},
	{ MOUNT_SMBFS,		LINUX_SMB_SUPER_MAGIC		},
	{ MOUNT_PTYFS,		LINUX_DEVPTS_SUPER_MAGIC	},
	{ MOUNT_TMPFS,		LINUX_TMPFS_SUPER_MAGIC		}
};
const int linux_fstypes_cnt = sizeof(linux_fstypes) / sizeof(linux_fstypes[0]);

# ifdef DEBUG_LINUX
#define DPRINTF(a)	uprintf a
# else
#define DPRINTF(a)
# endif

/* Local linux_misc.c functions: */
static void linux_to_bsd_mmap_args(struct sys_mmap_args *,
    const struct linux_sys_mmap_args *);
static int linux_mmap(struct lwp *, const struct linux_sys_mmap_args *,
    register_t *, off_t);


/*
 * The information on a terminated (or stopped) process needs
 * to be converted in order for Linux binaries to get a valid signal
 * number out of it.
 */
int
bsd_to_linux_wstat(int st)
{

	int sig;

	if (WIFSIGNALED(st)) {
		sig = WTERMSIG(st);
		if (sig >= 0 && sig < NSIG)
			st= (st & ~0177) | native_to_linux_signo[sig];
	} else if (WIFSTOPPED(st)) {
		sig = WSTOPSIG(st);
		if (sig >= 0 && sig < NSIG)
			st = (st & ~0xff00) |
			    (native_to_linux_signo[sig] << 8);
	}
	return st;
}

/*
 * wait4(2).  Passed on to the NetBSD call, surrounded by code to
 * reserve some space for a NetBSD-style wait status, and converting
 * it to what Linux wants.
 */
int
linux_sys_wait4(struct lwp *l, const struct linux_sys_wait4_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(int *) status;
		syscallarg(int) options;
		syscallarg(struct rusage50 *) rusage;
	} */
	int error, status, options, linux_options, pid = SCARG(uap, pid);
	struct rusage50 ru50;
	struct rusage ru;
	proc_t *p;

	linux_options = SCARG(uap, options);
	options = WOPTSCHECKED;
	if (linux_options & ~(LINUX_WAIT4_KNOWNFLAGS))
		return (EINVAL);

	if (linux_options & LINUX_WAIT4_WNOHANG)
		options |= WNOHANG;
	if (linux_options & LINUX_WAIT4_WUNTRACED)
		options |= WUNTRACED;
	if (linux_options & LINUX_WAIT4_WCONTINUED)
		options |= WCONTINUED;
	if (linux_options & LINUX_WAIT4_WALL)
		options |= WALLSIG;
	if (linux_options & LINUX_WAIT4_WCLONE)
		options |= WALTSIG;
# ifdef DIAGNOSTIC
	if (linux_options & LINUX_WAIT4_WNOTHREAD)
		printf("WARNING: %s: linux process %d.%d called "
		       "waitpid with __WNOTHREAD set!",
		       __FILE__, l->l_proc->p_pid, l->l_lid);

# endif

	error = do_sys_wait(&pid, &status, options,
	    SCARG(uap, rusage) != NULL ? &ru : NULL);

	retval[0] = pid;
	if (pid == 0)
		return error;

	p = curproc;
	mutex_enter(p->p_lock);
	sigdelset(&p->p_sigpend.sp_set, SIGCHLD); /* XXXAD ksiginfo leak */
	mutex_exit(p->p_lock);

	if (SCARG(uap, rusage) != NULL) {
		rusage_to_rusage50(&ru, &ru50);
		error = copyout(&ru, SCARG(uap, rusage), sizeof(ru));
	}

	if (error == 0 && SCARG(uap, status) != NULL) {
		status = bsd_to_linux_wstat(status);
		error = copyout(&status, SCARG(uap, status), sizeof status);
	}

	return error;
}

/*
 * Linux brk(2).  Like native, but always return the new break value.
 */
int
linux_sys_brk(struct lwp *l, const struct linux_sys_brk_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) nsize;
	} */
	struct proc *p = l->l_proc;
	struct vmspace *vm = p->p_vmspace;
	struct sys_obreak_args oba;

	SCARG(&oba, nsize) = SCARG(uap, nsize);

	(void) sys_obreak(l, &oba, retval);
	retval[0] = (register_t)((char *)vm->vm_daddr + ptoa(vm->vm_dsize));
	return 0;
}

/*
 * Implement the fs stat functions. Straightforward.
 */
int
linux_sys_statfs(struct lwp *l, const struct linux_sys_statfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct linux_statfs *) sp;
	} */
	struct statvfs *sb;
	struct linux_statfs ltmp;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_pstatvfs(l, SCARG(uap, path), ST_WAIT, sb);
	if (error == 0) {
		bsd_to_linux_statfs(sb, &ltmp);
		error = copyout(&ltmp, SCARG(uap, sp), sizeof ltmp);
	}
	STATVFSBUF_PUT(sb);

	return error;
}

int
linux_sys_fstatfs(struct lwp *l, const struct linux_sys_fstatfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct linux_statfs *) sp;
	} */
	struct statvfs *sb;
	struct linux_statfs ltmp;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_fstatvfs(l, SCARG(uap, fd), ST_WAIT, sb);
	if (error == 0) {
		bsd_to_linux_statfs(sb, &ltmp);
		error = copyout(&ltmp, SCARG(uap, sp), sizeof ltmp);
	}
	STATVFSBUF_PUT(sb);

	return error;
}

/*
 * uname(). Just copy the info from the various strings stored in the
 * kernel, and put it in the Linux utsname structure. That structure
 * is almost the same as the NetBSD one, only it has fields 65 characters
 * long, and an extra domainname field.
 */
int
linux_sys_uname(struct lwp *l, const struct linux_sys_uname_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct linux_utsname *) up;
	} */
	struct linux_utsname luts;

	strlcpy(luts.l_sysname, linux_sysname, sizeof(luts.l_sysname));
	strlcpy(luts.l_nodename, hostname, sizeof(luts.l_nodename));
	strlcpy(luts.l_release, linux_release, sizeof(luts.l_release));
	strlcpy(luts.l_version, linux_version, sizeof(luts.l_version));
	strlcpy(luts.l_machine, LINUX_UNAME_ARCH, sizeof(luts.l_machine));
	strlcpy(luts.l_domainname, domainname, sizeof(luts.l_domainname));

	return copyout(&luts, SCARG(uap, up), sizeof(luts));
}

/* Used directly on: alpha, mips, ppc, sparc, sparc64 */
/* Used indirectly on: arm, i386, m68k */

/*
 * New type Linux mmap call.
 * Only called directly on machines with >= 6 free regs.
 */
int
linux_sys_mmap(struct lwp *l, const struct linux_sys_mmap_args *uap, register_t *retval)
{
	/* {
		syscallarg(unsigned long) addr;
		syscallarg(size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(linux_off_t) offset;
	} */

	if (SCARG(uap, offset) & PAGE_MASK)
		return EINVAL;

	return linux_mmap(l, uap, retval, SCARG(uap, offset));
}

/*
 * Guts of most architectures' mmap64() implementations.  This shares
 * its list of arguments with linux_sys_mmap().
 *
 * The difference in linux_sys_mmap2() is that "offset" is actually
 * (offset / pagesize), not an absolute byte count.  This translation
 * to pagesize offsets is done inside glibc between the mmap64() call
 * point, and the actual syscall.
 */
int
linux_sys_mmap2(struct lwp *l, const struct linux_sys_mmap2_args *uap, register_t *retval)
{
	/* {
		syscallarg(unsigned long) addr;
		syscallarg(size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(linux_off_t) offset;
	} */

	return linux_mmap(l, uap, retval,
	    ((off_t)SCARG(uap, offset)) << PAGE_SHIFT);
}

/*
 * Massage arguments and call system mmap(2).
 */
static int
linux_mmap(struct lwp *l, const struct linux_sys_mmap_args *uap, register_t *retval, off_t offset)
{
	struct sys_mmap_args cma;
	int error;
	size_t mmoff=0;

	linux_to_bsd_mmap_args(&cma, uap);
	SCARG(&cma, pos) = offset;

	if (SCARG(uap, flags) & LINUX_MAP_GROWSDOWN) {
		/*
		 * Request for stack-like memory segment. On linux, this
		 * works by mmap()ping (small) segment, which is automatically
		 * extended when page fault happens below the currently
		 * allocated area. We emulate this by allocating (typically
		 * bigger) segment sized at current stack size limit, and
		 * offsetting the requested and returned address accordingly.
		 * Since physical pages are only allocated on-demand, this
		 * is effectively identical.
		 */
		rlim_t ssl = l->l_proc->p_rlimit[RLIMIT_STACK].rlim_cur;

		if (SCARG(&cma, len) < ssl) {
			/* Compute the address offset */
			mmoff = round_page(ssl) - SCARG(uap, len);

			if (SCARG(&cma, addr))
				SCARG(&cma, addr) = (char *)SCARG(&cma, addr) - mmoff;

			SCARG(&cma, len) = (size_t) ssl;
		}
	}

	error = sys_mmap(l, &cma, retval);
	if (error)
		return (error);

	/* Shift the returned address for stack-like segment if necessary */
	retval[0] += mmoff;

	return (0);
}

static void
linux_to_bsd_mmap_args(struct sys_mmap_args *cma, const struct linux_sys_mmap_args *uap)
{
	int flags = MAP_TRYFIXED, fl = SCARG(uap, flags);

	flags |= cvtto_bsd_mask(fl, LINUX_MAP_SHARED, MAP_SHARED);
	flags |= cvtto_bsd_mask(fl, LINUX_MAP_PRIVATE, MAP_PRIVATE);
	flags |= cvtto_bsd_mask(fl, LINUX_MAP_FIXED, MAP_FIXED);
	flags |= cvtto_bsd_mask(fl, LINUX_MAP_ANON, MAP_ANON);
	flags |= cvtto_bsd_mask(fl, LINUX_MAP_LOCKED, MAP_WIRED);
	/* XXX XAX ERH: Any other flags here?  There are more defined... */

	SCARG(cma, addr) = (void *)SCARG(uap, addr);
	SCARG(cma, len) = SCARG(uap, len);
	SCARG(cma, prot) = SCARG(uap, prot);
	if (SCARG(cma, prot) & VM_PROT_WRITE) /* XXX */
		SCARG(cma, prot) |= VM_PROT_READ;
	SCARG(cma, flags) = flags;
	SCARG(cma, fd) = flags & MAP_ANON ? -1 : SCARG(uap, fd);
	SCARG(cma, PAD) = 0;
}

#define	LINUX_MREMAP_MAYMOVE	1
#define	LINUX_MREMAP_FIXED	2

int
linux_sys_mremap(struct lwp *l, const struct linux_sys_mremap_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) old_address;
		syscallarg(size_t) old_size;
		syscallarg(size_t) new_size;
		syscallarg(u_long) flags;
	} */

	struct proc *p;
	struct vm_map *map;
	vaddr_t oldva;
	vaddr_t newva;
	size_t oldsize;
	size_t newsize;
	int flags;
	int uvmflags;
	int error;

	flags = SCARG(uap, flags);
	oldva = (vaddr_t)SCARG(uap, old_address);
	oldsize = round_page(SCARG(uap, old_size));
	newsize = round_page(SCARG(uap, new_size));
	if ((flags & ~(LINUX_MREMAP_FIXED|LINUX_MREMAP_MAYMOVE)) != 0) {
		error = EINVAL;
		goto done;
	}
	if ((flags & LINUX_MREMAP_FIXED) != 0) {
		if ((flags & LINUX_MREMAP_MAYMOVE) == 0) {
			error = EINVAL;
			goto done;
		}
#if 0 /* notyet */
		newva = SCARG(uap, new_address);
		uvmflags = MAP_FIXED;
#else /* notyet */
		error = EOPNOTSUPP;
		goto done;
#endif /* notyet */
	} else if ((flags & LINUX_MREMAP_MAYMOVE) != 0) {
		uvmflags = 0;
	} else {
		newva = oldva;
		uvmflags = MAP_FIXED;
	}
	p = l->l_proc;
	map = &p->p_vmspace->vm_map;
	error = uvm_mremap(map, oldva, oldsize, map, &newva, newsize, p,
	    uvmflags);

done:
	*retval = (error != 0) ? 0 : (register_t)newva;
	return error;
}

#ifdef USRSTACK
int
linux_sys_mprotect(struct lwp *l, const struct linux_sys_mprotect_args *uap, register_t *retval)
{
	/* {
		syscallarg(const void *) start;
		syscallarg(unsigned long) len;
		syscallarg(int) prot;
	} */
	struct vm_map_entry *entry;
	struct vm_map *map;
	struct proc *p;
	vaddr_t end, start, len, stacklim;
	int prot, grows;

	start = (vaddr_t)SCARG(uap, start);
	len = round_page(SCARG(uap, len));
	prot = SCARG(uap, prot);
	grows = prot & (LINUX_PROT_GROWSDOWN | LINUX_PROT_GROWSUP);
	prot &= ~grows;
	end = start + len;

	if (start & PAGE_MASK)
		return EINVAL;
	if (end < start)
		return EINVAL;
	if (end == start)
		return 0;

	if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
		return EINVAL;
	if (grows == (LINUX_PROT_GROWSDOWN | LINUX_PROT_GROWSUP))
		return EINVAL;

	p = l->l_proc;
	map = &p->p_vmspace->vm_map;
	vm_map_lock(map);
# ifdef notdef
	VM_MAP_RANGE_CHECK(map, start, end);
# endif
	if (!uvm_map_lookup_entry(map, start, &entry) || entry->start > start) {
		vm_map_unlock(map);
		return ENOMEM;
	}

	/*
	 * Approximate the behaviour of PROT_GROWS{DOWN,UP}.
	 */

	stacklim = (vaddr_t)p->p_limit->pl_rlimit[RLIMIT_STACK].rlim_cur;
	if (grows & LINUX_PROT_GROWSDOWN) {
		if (USRSTACK - stacklim <= start && start < USRSTACK) {
			start = USRSTACK - stacklim;
		} else {
			start = entry->start;
		}
	} else if (grows & LINUX_PROT_GROWSUP) {
		if (USRSTACK <= end && end < USRSTACK + stacklim) {
			end = USRSTACK + stacklim;
		} else {
			end = entry->end;
		}
	}
	vm_map_unlock(map);
	return uvm_map_protect(map, start, end, prot, FALSE);
}
#endif /* USRSTACK */

/*
 * This code is partly stolen from src/lib/libc/compat-43/times.c
 */

#define	CONVTCK(r)	(r.tv_sec * hz + r.tv_usec / (1000000 / hz))

int
linux_sys_times(struct lwp *l, const struct linux_sys_times_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct times *) tms;
	} */
	struct proc *p = l->l_proc;
	struct timeval t;
	int error;

	if (SCARG(uap, tms)) {
		struct linux_tms ltms;
		struct rusage ru;

		mutex_enter(p->p_lock);
		calcru(p, &ru.ru_utime, &ru.ru_stime, NULL, NULL);
		ltms.ltms_utime = CONVTCK(ru.ru_utime);
		ltms.ltms_stime = CONVTCK(ru.ru_stime);
		ltms.ltms_cutime = CONVTCK(p->p_stats->p_cru.ru_utime);
		ltms.ltms_cstime = CONVTCK(p->p_stats->p_cru.ru_stime);
		mutex_exit(p->p_lock);

		if ((error = copyout(&ltms, SCARG(uap, tms), sizeof ltms)))
			return error;
	}

	getmicrouptime(&t);

	retval[0] = ((linux_clock_t)(CONVTCK(t)));
	return 0;
}

#undef CONVTCK

/*
 * Linux 'readdir' call. This code is mostly taken from the
 * SunOS getdents call (see compat/sunos/sunos_misc.c), though
 * an attempt has been made to keep it a little cleaner (failing
 * miserably, because of the cruft needed if count 1 is passed).
 *
 * The d_off field should contain the offset of the next valid entry,
 * but in Linux it has the offset of the entry itself. We emulate
 * that bug here.
 *
 * Read in BSD-style entries, convert them, and copy them out.
 *
 * Note that this doesn't handle union-mounted filesystems.
 */
int
linux_sys_getdents(struct lwp *l, const struct linux_sys_getdents_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct linux_dirent *) dent;
		syscallarg(unsigned int) count;
	} */
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *tbuf;		/* BSD-format */
	int len, reclen;		/* BSD-format */
	char *outp;			/* Linux-format */
	int resid, linux_reclen = 0;	/* Linux-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct linux_dirent idb;
	off_t off;		/* true file offset */
	int buflen, error, eofflag, nbytes, oldcall;
	struct vattr va;
	off_t *cookiebuf = NULL, *cookie;
	int ncookies;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);

	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out1;
	}

	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out1;
	}

	vn_lock(vp, LK_SHARED | LK_RETRY);
	error = VOP_GETATTR(vp, &va, l->l_cred);
	VOP_UNLOCK(vp);
	if (error)
		goto out1;

	nbytes = SCARG(uap, count);
	if (nbytes == 1) {	/* emulating old, broken behaviour */
		nbytes = sizeof (idb);
		buflen = max(va.va_blocksize, nbytes);
		oldcall = 1;
	} else {
		buflen = min(MAXBSIZE, nbytes);
		if (buflen < va.va_blocksize)
			buflen = va.va_blocksize;
		oldcall = 0;
	}
	tbuf = malloc(buflen, M_TEMP, M_WAITOK);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = fp->f_offset;
again:
	aiov.iov_base = tbuf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	UIO_SETUP_SYSSPACE(&auio);
	/*
         * First we read into the malloc'ed buffer, then
         * we massage it into user space, one record at a time.
         */
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, &cookiebuf,
	    &ncookies);
	if (error)
		goto out;

	inp = tbuf;
	outp = (void *)SCARG(uap, dent);
	resid = nbytes;
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("linux_readdir");
		if (bdp->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			if (cookie)
				off = *cookie++;
			else
				off += reclen;
			continue;
		}
		linux_reclen = LINUX_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < linux_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/*
		 * Massage in place to make a Linux-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_ino = bdp->d_fileno;
		/*
		 * The old readdir() call misuses the offset and reclen fields.
		 */
		if (oldcall) {
			idb.d_off = (linux_off_t)linux_reclen;
			idb.d_reclen = (u_short)bdp->d_namlen;
		} else {
			if (sizeof (idb.d_off) <= 4 && (off >> 32) != 0) {
				compat_offseterr(vp, "linux_getdents");
				error = EINVAL;
				goto out;
			}
			idb.d_off = (linux_off_t)off;
			idb.d_reclen = (u_short)linux_reclen;
			/* Linux puts d_type at the end of each record */
			*((char *)&idb + idb.d_reclen - 1) = bdp->d_type;
		}
		strcpy(idb.d_name, bdp->d_name);
		if ((error = copyout((void *)&idb, outp, linux_reclen)))
			goto out;
		/* advance past this real entry */
		inp += reclen;
		if (cookie)
			off = *cookie++; /* each entry points to itself */
		else
			off += reclen;
		/* advance output past Linux-shaped entry */
		outp += linux_reclen;
		resid -= linux_reclen;
		if (oldcall)
			break;
	}

	/* if we squished out the whole block, try again */
	if (outp == (void *)SCARG(uap, dent)) {
		if (cookiebuf)
			free(cookiebuf, M_TEMP);
		cookiebuf = NULL;
		goto again;
	}
	fp->f_offset = off;	/* update the vnode offset */

	if (oldcall)
		nbytes = resid + linux_reclen;

eof:
	*retval = nbytes - resid;
out:
	VOP_UNLOCK(vp);
	if (cookiebuf)
		free(cookiebuf, M_TEMP);
	free(tbuf, M_TEMP);
out1:
	fd_putfile(SCARG(uap, fd));
	return error;
}

/*
 * Even when just using registers to pass arguments to syscalls you can
 * have 5 of them on the i386. So this newer version of select() does
 * this.
 */
int
linux_sys_select(struct lwp *l, const struct linux_sys_select_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) nfds;
		syscallarg(fd_set *) readfds;
		syscallarg(fd_set *) writefds;
		syscallarg(fd_set *) exceptfds;
		syscallarg(struct timeval50 *) timeout;
	} */

	return linux_select1(l, retval, SCARG(uap, nfds), SCARG(uap, readfds),
	    SCARG(uap, writefds), SCARG(uap, exceptfds),
	    (struct linux_timeval *)SCARG(uap, timeout));
}

/*
 * Common code for the old and new versions of select(). A couple of
 * things are important:
 * 1) return the amount of time left in the 'timeout' parameter
 * 2) select never returns ERESTART on Linux, always return EINTR
 */
int
linux_select1(struct lwp *l, register_t *retval, int nfds, fd_set *readfds,
    fd_set *writefds, fd_set *exceptfds, struct linux_timeval *timeout)
{
	struct timespec ts0, ts1, uts, *ts = NULL;
	struct linux_timeval ltv;
	int error;

	/*
	 * Store current time for computation of the amount of
	 * time left.
	 */
	if (timeout) {
		if ((error = copyin(timeout, &ltv, sizeof(ltv))))
			return error;
		uts.tv_sec = ltv.tv_sec;
		uts.tv_nsec = ltv.tv_usec * 1000;
		if (itimespecfix(&uts)) {
			/*
			 * The timeval was invalid.  Convert it to something
			 * valid that will act as it does under Linux.
			 */
			uts.tv_sec += uts.tv_nsec / 1000000000;
			uts.tv_nsec %= 1000000000;
			if (uts.tv_nsec < 0) {
				uts.tv_sec -= 1;
				uts.tv_nsec += 1000000000;
			}
			if (uts.tv_sec < 0)
				timespecclear(&uts);
		}
		ts = &uts;
		nanotime(&ts0);
	}

	error = selcommon(retval, nfds, readfds, writefds, exceptfds, ts, NULL);

	if (error) {
		/*
		 * See fs/select.c in the Linux kernel.  Without this,
		 * Maelstrom doesn't work.
		 */
		if (error == ERESTART)
			error = EINTR;
		return error;
	}

	if (timeout) {
		if (*retval) {
			/*
			 * Compute how much time was left of the timeout,
			 * by subtracting the current time and the time
			 * before we started the call, and subtracting
			 * that result from the user-supplied value.
			 */
			nanotime(&ts1);
			timespecsub(&ts1, &ts0, &ts1);
			timespecsub(&uts, &ts1, &uts);
			if (uts.tv_sec < 0)
				timespecclear(&uts);
		} else
			timespecclear(&uts);
		ltv.tv_sec = uts.tv_sec;
		ltv.tv_usec = uts.tv_nsec / 1000;
		if ((error = copyout(&ltv, timeout, sizeof(ltv))))
			return error;
	}

	return 0;
}

int
linux_sys_ppoll(struct lwp *l,
	const struct linux_sys_ppoll_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct pollfd *) fds;
		syscallarg(u_int) nfds;
		syscallarg(struct linux_timespec *) timeout;
		syscallarg(linux_sigset_t *) sigset;
	} */
	struct linux_timespec lts0, *lts;
	struct timespec ts0, *ts = NULL;
	linux_sigset_t lsigmask0, *lsigmask;
	sigset_t sigmask0, *sigmask = NULL;
	int error;

	lts = SCARG(uap, timeout);
	if (lts) {
		if ((error = copyin(lts, &lts0, sizeof(lts0))) != 0)
			return error;
		linux_to_native_timespec(&ts0, &lts0);
		ts = &ts0;
	}

	lsigmask = SCARG(uap, sigset);
	if (lsigmask) {
		if ((error = copyin(lsigmask, &lsigmask0, sizeof(lsigmask0))))
			return error;
		linux_to_native_sigset(&sigmask0, &lsigmask0);
		sigmask = &sigmask0;
	}

	return pollcommon(retval, SCARG(uap, fds), SCARG(uap, nfds),
	    ts, sigmask);
}

/*
 * Set the 'personality' (emulation mode) for the current process. Only
 * accept the Linux personality here (0). This call is needed because
 * the Linux ELF crt0 issues it in an ugly kludge to make sure that
 * ELF binaries run in Linux mode, not SVR4 mode.
 */
int
linux_sys_personality(struct lwp *l, const struct linux_sys_personality_args *uap, register_t *retval)
{
	/* {
		syscallarg(unsigned long) per;
	} */
	struct linux_emuldata *led;
	int per;

	per = SCARG(uap, per);
	led = l->l_emuldata;
	if (per == LINUX_PER_QUERY) {
		retval[0] = led->led_personality;
		return 0;
	}

	switch (per & LINUX_PER_MASK) {
	case LINUX_PER_LINUX:
	case LINUX_PER_LINUX32:
		led->led_personality = per;
		break;

	default:
		return EINVAL;
	}

	retval[0] = per;
	return 0;
}

/*
 * We have nonexistent fsuid equal to uid.
 * If modification is requested, refuse.
 */
int
linux_sys_setfsuid(struct lwp *l, const struct linux_sys_setfsuid_args *uap, register_t *retval)
{
	 /* {
		 syscallarg(uid_t) uid;
	 } */
	 uid_t uid;

	 uid = SCARG(uap, uid);
	 if (kauth_cred_getuid(l->l_cred) != uid)
		 return sys_nosys(l, uap, retval);

	 *retval = uid;
	 return 0;
}

int
linux_sys_setfsgid(struct lwp *l, const struct linux_sys_setfsgid_args *uap, register_t *retval)
{
	/* {
		syscallarg(gid_t) gid;
	} */
	gid_t gid;

	gid = SCARG(uap, gid);
	if (kauth_cred_getgid(l->l_cred) != gid)
		return sys_nosys(l, uap, retval);

	*retval = gid;
	return 0;
}

int
linux_sys_setresuid(struct lwp *l, const struct linux_sys_setresuid_args *uap, register_t *retval)
{
	/* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
		syscallarg(uid_t) suid;
	} */

	/*
	 * Note: These checks are a little different than the NetBSD
	 * setreuid(2) call performs.  This precisely follows the
	 * behavior of the Linux kernel.
	 */

	return do_setresuid(l, SCARG(uap, ruid), SCARG(uap, euid),
			    SCARG(uap, suid),
			    ID_R_EQ_R | ID_R_EQ_E | ID_R_EQ_S |
			    ID_E_EQ_R | ID_E_EQ_E | ID_E_EQ_S |
			    ID_S_EQ_R | ID_S_EQ_E | ID_S_EQ_S );
}

int
linux_sys_getresuid(struct lwp *l, const struct linux_sys_getresuid_args *uap, register_t *retval)
{
	/* {
		syscallarg(uid_t *) ruid;
		syscallarg(uid_t *) euid;
		syscallarg(uid_t *) suid;
	} */
	kauth_cred_t pc = l->l_cred;
	int error;
	uid_t uid;

	/*
	 * Linux copies these values out to userspace like so:
	 *
	 *	1. Copy out ruid.
	 *	2. If that succeeds, copy out euid.
	 *	3. If both of those succeed, copy out suid.
	 */
	uid = kauth_cred_getuid(pc);
	if ((error = copyout(&uid, SCARG(uap, ruid), sizeof(uid_t))) != 0)
		return (error);

	uid = kauth_cred_geteuid(pc);
	if ((error = copyout(&uid, SCARG(uap, euid), sizeof(uid_t))) != 0)
		return (error);

	uid = kauth_cred_getsvuid(pc);

	return (copyout(&uid, SCARG(uap, suid), sizeof(uid_t)));
}

int
linux_sys_ptrace(struct lwp *l, const struct linux_sys_ptrace_args *uap, register_t *retval)
{
	/* {
		i386, m68k, powerpc: T=int
		alpha, amd64: T=long
		syscallarg(T) request;
		syscallarg(T) pid;
		syscallarg(T) addr;
		syscallarg(T) data;
	} */
	const int *ptr;
	int request;
	int error;

	ptr = linux_ptrace_request_map;
	request = SCARG(uap, request);
	while (*ptr != -1)
		if (*ptr++ == request) {
			struct sys_ptrace_args pta;

			SCARG(&pta, req) = *ptr;
			SCARG(&pta, pid) = SCARG(uap, pid);
			SCARG(&pta, addr) = (void *)SCARG(uap, addr);
			SCARG(&pta, data) = SCARG(uap, data);

			/*
			 * Linux ptrace(PTRACE_CONT, pid, 0, 0) means actually
			 * to continue where the process left off previously.
 			 * The same thing is achieved by addr == (void *) 1
			 * on NetBSD, so rewrite 'addr' appropriately.
			 */
			if (request == LINUX_PTRACE_CONT && SCARG(uap, addr)==0)
				SCARG(&pta, addr) = (void *) 1;

			error = sysent[SYS_ptrace].sy_call(l, &pta, retval);
			if (error)
				return error;
			switch (request) {
			case LINUX_PTRACE_PEEKTEXT:
			case LINUX_PTRACE_PEEKDATA:
				error = copyout (retval,
				    (void *)SCARG(uap, data),
				    sizeof *retval);
				*retval = SCARG(uap, data);
				break;
			default:
				break;
			}
			return error;
		}
		else
			ptr++;

	return LINUX_SYS_PTRACE_ARCH(l, uap, retval);
}

int
linux_sys_reboot(struct lwp *l, const struct linux_sys_reboot_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) magic1;
		syscallarg(int) magic2;
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */
	struct sys_reboot_args /* {
		syscallarg(int) opt;
		syscallarg(char *) bootstr;
	} */ sra;
	int error;

	if ((error = kauth_authorize_system(l->l_cred,
	    KAUTH_SYSTEM_REBOOT, 0, NULL, NULL, NULL)) != 0)
		return(error);

	if (SCARG(uap, magic1) != LINUX_REBOOT_MAGIC1)
		return(EINVAL);
	if (SCARG(uap, magic2) != LINUX_REBOOT_MAGIC2 &&
	    SCARG(uap, magic2) != LINUX_REBOOT_MAGIC2A &&
	    SCARG(uap, magic2) != LINUX_REBOOT_MAGIC2B)
		return(EINVAL);

	switch ((unsigned long)SCARG(uap, cmd)) {
	case LINUX_REBOOT_CMD_RESTART:
		SCARG(&sra, opt) = RB_AUTOBOOT;
		break;
	case LINUX_REBOOT_CMD_HALT:
		SCARG(&sra, opt) = RB_HALT;
		break;
	case LINUX_REBOOT_CMD_POWER_OFF:
		SCARG(&sra, opt) = RB_HALT|RB_POWERDOWN;
		break;
	case LINUX_REBOOT_CMD_RESTART2:
		/* Reboot with an argument. */
		SCARG(&sra, opt) = RB_AUTOBOOT|RB_STRING;
		SCARG(&sra, bootstr) = SCARG(uap, arg);
		break;
	case LINUX_REBOOT_CMD_CAD_ON:
		return(EINVAL);	/* We don't implement ctrl-alt-delete */
	case LINUX_REBOOT_CMD_CAD_OFF:
		return(0);
	default:
		return(EINVAL);
	}

	return(sys_reboot(l, &sra, retval));
}

/*
 * Copy of compat_12_sys_swapon().
 */
int
linux_sys_swapon(struct lwp *l, const struct linux_sys_swapon_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) name;
	} */
	struct sys_swapctl_args ua;

	SCARG(&ua, cmd) = SWAP_ON;
	SCARG(&ua, arg) = (void *)__UNCONST(SCARG(uap, name));
	SCARG(&ua, misc) = 0;	/* priority */
	return (sys_swapctl(l, &ua, retval));
}

/*
 * Stop swapping to the file or block device specified by path.
 */
int
linux_sys_swapoff(struct lwp *l, const struct linux_sys_swapoff_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
	} */
	struct sys_swapctl_args ua;

	SCARG(&ua, cmd) = SWAP_OFF;
	SCARG(&ua, arg) = __UNCONST(SCARG(uap, path)); /*XXXUNCONST*/
	return (sys_swapctl(l, &ua, retval));
}

/*
 * Copy of compat_09_sys_setdomainname()
 */
/* ARGSUSED */
int
linux_sys_setdomainname(struct lwp *l, const struct linux_sys_setdomainname_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) domainname;
		syscallarg(int) len;
	} */
	int name[2];

	name[0] = CTL_KERN;
	name[1] = KERN_DOMAINNAME;
	return (old_sysctl(&name[0], 2, 0, 0, SCARG(uap, domainname),
			    SCARG(uap, len), l));
}

/*
 * sysinfo()
 */
/* ARGSUSED */
int
linux_sys_sysinfo(struct lwp *l, const struct linux_sys_sysinfo_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct linux_sysinfo *) arg;
	} */
	struct linux_sysinfo si;
	struct loadavg *la;

	si.uptime = time_uptime;
	la = &averunnable;
	si.loads[0] = la->ldavg[0] * LINUX_SYSINFO_LOADS_SCALE / la->fscale;
	si.loads[1] = la->ldavg[1] * LINUX_SYSINFO_LOADS_SCALE / la->fscale;
	si.loads[2] = la->ldavg[2] * LINUX_SYSINFO_LOADS_SCALE / la->fscale;
	si.totalram = ctob((u_long)physmem);
	si.freeram = (u_long)uvmexp.free * uvmexp.pagesize;
	si.sharedram = 0;	/* XXX */
	si.bufferram = (u_long)uvmexp.filepages * uvmexp.pagesize;
	si.totalswap = (u_long)uvmexp.swpages * uvmexp.pagesize;
	si.freeswap =
	    (u_long)(uvmexp.swpages - uvmexp.swpginuse) * uvmexp.pagesize;
	si.procs = nprocs;

	/* The following are only present in newer Linux kernels. */
	si.totalbig = 0;
	si.freebig = 0;
	si.mem_unit = 1;

	return (copyout(&si, SCARG(uap, arg), sizeof si));
}

int
linux_sys_getrlimit(struct lwp *l, const struct linux_sys_getrlimit_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
# ifdef LINUX_LARGEFILE64
		syscallarg(struct rlimit *) rlp;
# else
		syscallarg(struct orlimit *) rlp;
# endif
	} */
# ifdef LINUX_LARGEFILE64
	struct rlimit orl;
# else
	struct orlimit orl;
# endif
	int which;

	which = linux_to_bsd_limit(SCARG(uap, which));
	if (which < 0)
		return -which;

	bsd_to_linux_rlimit(&orl, &l->l_proc->p_rlimit[which]);

	return copyout(&orl, SCARG(uap, rlp), sizeof(orl));
}

int
linux_sys_setrlimit(struct lwp *l, const struct linux_sys_setrlimit_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
# ifdef LINUX_LARGEFILE64
		syscallarg(struct rlimit *) rlp;
# else
		syscallarg(struct orlimit *) rlp;
# endif
	} */
	struct rlimit rl;
# ifdef LINUX_LARGEFILE64
	struct rlimit orl;
# else
	struct orlimit orl;
# endif
	int error;
	int which;

	if ((error = copyin(SCARG(uap, rlp), &orl, sizeof(orl))) != 0)
		return error;

	which = linux_to_bsd_limit(SCARG(uap, which));
	if (which < 0)
		return -which;

	linux_to_bsd_rlimit(&rl, &orl);
	return dosetrlimit(l, l->l_proc, which, &rl);
}

# if !defined(__mips__) && !defined(__amd64__)
/* XXX: this doesn't look 100% common, at least mips doesn't have it */
int
linux_sys_ugetrlimit(struct lwp *l, const struct linux_sys_ugetrlimit_args *uap, register_t *retval)
{
	return linux_sys_getrlimit(l, (const void *)uap, retval);
}
# endif

/*
 * This gets called for unsupported syscalls. The difference to sys_nosys()
 * is that process does not get SIGSYS, the call just returns with ENOSYS.
 * This is the way Linux does it and glibc depends on this behaviour.
 */
int
linux_sys_nosys(struct lwp *l, const void *v, register_t *retval)
{
	return (ENOSYS);
}

int
linux_sys_getpriority(struct lwp *l, const struct linux_sys_getpriority_args *uap, register_t *retval)
{
        /* {
                syscallarg(int) which;
                syscallarg(int) who;
        } */
        struct sys_getpriority_args bsa;
        int error;

        SCARG(&bsa, which) = SCARG(uap, which);
        SCARG(&bsa, who) = SCARG(uap, who);

        if ((error = sys_getpriority(l, &bsa, retval)))
                return error;

        *retval = NZERO - *retval;

        return 0;
}

int
linux_do_sys_utimensat(struct lwp *l, int fd, const char *path, struct timespec *tsp, int flags, register_t *retval)
{
	int follow, error;

	follow = (flags & LINUX_AT_SYMLINK_NOFOLLOW) ? NOFOLLOW : FOLLOW;

	if (path == NULL && fd != AT_FDCWD) {
		file_t *fp;

		/* fd_getvnode() will use the descriptor for us */
		if ((error = fd_getvnode(fd, &fp)) != 0)
			return error;
		error = do_sys_utimensat(l, AT_FDCWD, fp->f_data, NULL, 0,
		    tsp, UIO_SYSSPACE);
		fd_putfile(fd);
		return error;
	}

	return do_sys_utimensat(l, fd, NULL, path, follow, tsp, UIO_SYSSPACE);
}

int
linux_sys_utimensat(struct lwp *l, const struct linux_sys_utimensat_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(const struct linux_timespec *) times;
		syscallarg(int) flag;
	} */
	int error;
	struct linux_timespec lts[2];
	struct timespec *tsp = NULL, ts[2];

	if (SCARG(uap, times)) {
		error = copyin(SCARG(uap, times), &lts, sizeof(lts));
		if (error != 0)
			return error;
		linux_to_native_timespec(&ts[0], &lts[0]);
		linux_to_native_timespec(&ts[1], &lts[1]);
		tsp = ts;
	}

	return linux_do_sys_utimensat(l, SCARG(uap, fd), SCARG(uap, path),
	    tsp, SCARG(uap, flag), retval);
}
