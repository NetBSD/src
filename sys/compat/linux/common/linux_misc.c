/*	$NetBSD: linux_misc.c,v 1.174 2007/05/08 20:54:15 dsl Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 1999 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_misc.c,v 1.174 2007/05/08 20:54:15 dsl Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ptrace.h"
#endif

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
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
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

#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/linux_syscallargs.h>

#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_mmap.h>
#include <compat/linux/common/linux_dirent.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_misc.h>
#ifndef COMPAT_LINUX32
#include <compat/linux/common/linux_statfs.h>
#include <compat/linux/common/linux_limit.h>
#endif
#include <compat/linux/common/linux_ptrace.h>
#include <compat/linux/common/linux_reboot.h>
#include <compat/linux/common/linux_emuldata.h>

#ifndef COMPAT_LINUX32
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
	{ MOUNT_PORTAL,		LINUX_DEFAULT_SUPER_MAGIC	},
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
	{ MOUNT_TMPFS,		LINUX_DEFAULT_SUPER_MAGIC	}
};
const int linux_fstypes_cnt = sizeof(linux_fstypes) / sizeof(linux_fstypes[0]);

# ifdef DEBUG_LINUX
#define DPRINTF(a)	uprintf a
# else
#define DPRINTF(a)
# endif

/* Local linux_misc.c functions: */
static void linux_to_bsd_mmap_args __P((struct sys_mmap_args *,
    const struct linux_sys_mmap_args *));
static int linux_mmap __P((struct lwp *, struct linux_sys_mmap_args *,
    register_t *, off_t));


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
linux_sys_wait4(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_wait4_args /* {
		syscallarg(int) pid;
		syscallarg(int *) status;
		syscallarg(int) options;
		syscallarg(struct rusage *) rusage;
	} */ *uap = v;
	int error, status, options, linux_options, was_zombie;
	struct rusage ru;

	linux_options = SCARG(uap, options);
	options = WOPTSCHECKED;
	if (linux_options & ~(LINUX_WAIT4_KNOWNFLAGS))
		return (EINVAL);

	if (linux_options & LINUX_WAIT4_WNOHANG)
		options |= WNOHANG;
	if (linux_options & LINUX_WAIT4_WUNTRACED)
		options |= WUNTRACED;
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

	error = do_sys_wait(l, &SCARG(uap, pid), &status, options,
	    SCARG(uap, rusage) != NULL ? &ru : NULL, &was_zombie);

	retval[0] = SCARG(uap, pid);
	if (SCARG(uap, pid) == 0)
		return error;

	sigdelset(&l->l_proc->p_sigpend.sp_set, SIGCHLD);	/* XXXAD ksiginfo leak */

	if (SCARG(uap, rusage) != NULL)
		error = copyout(&ru, SCARG(uap, rusage), sizeof(ru));

	if (error == 0 && SCARG(uap, status) != NULL) {
		status = bsd_to_linux_wstat(status);
		error = copyout(&status, SCARG(uap, status), sizeof status);
	}

	return error;
}

/*
 * Linux brk(2). The check if the new address is >= the old one is
 * done in the kernel in Linux. NetBSD does it in the library.
 */
int
linux_sys_brk(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_brk_args /* {
		syscallarg(char *) nsize;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	char *nbrk = SCARG(uap, nsize);
	struct sys_obreak_args oba;
	struct vmspace *vm = p->p_vmspace;
	struct linux_emuldata *ed = (struct linux_emuldata*)p->p_emuldata;

	SCARG(&oba, nsize) = nbrk;

	if ((void *) nbrk > vm->vm_daddr && sys_obreak(l, &oba, retval) == 0)
		ed->s->p_break = (char*)nbrk;
	else
		nbrk = ed->s->p_break;

	retval[0] = (register_t)nbrk;

	return 0;
}

/*
 * Implement the fs stat functions. Straightforward.
 */
int
linux_sys_statfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_statfs_args /* {
		syscallarg(const char *) path;
		syscallarg(struct linux_statfs *) sp;
	} */ *uap = v;
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
linux_sys_fstatfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(struct linux_statfs *) sp;
	} */ *uap = v;
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
linux_sys_uname(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_uname_args /* {
		syscallarg(struct linux_utsname *) up;
	} */ *uap = v;
	struct linux_utsname luts;

	strncpy(luts.l_sysname, linux_sysname, sizeof(luts.l_sysname));
	strncpy(luts.l_nodename, hostname, sizeof(luts.l_nodename));
	strncpy(luts.l_release, linux_release, sizeof(luts.l_release));
	strncpy(luts.l_version, linux_version, sizeof(luts.l_version));
	strncpy(luts.l_machine, linux_machine, sizeof(luts.l_machine));
	strncpy(luts.l_domainname, domainname, sizeof(luts.l_domainname));

	return copyout(&luts, SCARG(uap, up), sizeof(luts));
}

/* Used directly on: alpha, mips, ppc, sparc, sparc64 */
/* Used indirectly on: arm, i386, m68k */

/*
 * New type Linux mmap call.
 * Only called directly on machines with >= 6 free regs.
 */
int
linux_sys_mmap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_mmap_args /* {
		syscallarg(unsigned long) addr;
		syscallarg(size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(linux_off_t) offset;
	} */ *uap = v;

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
linux_sys_mmap2(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_mmap2_args /* {
		syscallarg(unsigned long) addr;
		syscallarg(size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(linux_off_t) offset;
	} */ *uap = v;

	return linux_mmap(l, uap, retval,
	    ((off_t)SCARG(uap, offset)) << PAGE_SHIFT);
}

/*
 * Massage arguments and call system mmap(2).
 */
static int
linux_mmap(l, uap, retval, offset)
	struct lwp *l;
	struct linux_sys_mmap_args *uap;
	register_t *retval;
	off_t offset;
{
	struct sys_mmap_args cma;
	int error;
	size_t mmoff=0;

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

		if (SCARG(uap, len) < ssl) {
			/* Compute the address offset */
			mmoff = round_page(ssl) - SCARG(uap, len);

			if (SCARG(uap, addr))
				SCARG(uap, addr) -= mmoff;

			SCARG(uap, len) = (size_t) ssl;
		}
	}

	linux_to_bsd_mmap_args(&cma, uap);
	SCARG(&cma, pos) = offset;

	error = sys_mmap(l, &cma, retval);
	if (error)
		return (error);

	/* Shift the returned address for stack-like segment if necessary */
	if (SCARG(uap, flags) & LINUX_MAP_GROWSDOWN && mmoff)
		retval[0] += mmoff;

	return (0);
}

static void
linux_to_bsd_mmap_args(cma, uap)
	struct sys_mmap_args *cma;
	const struct linux_sys_mmap_args *uap;
{
	int flags = MAP_TRYFIXED, fl = SCARG(uap, flags);

	flags |= cvtto_bsd_mask(fl, LINUX_MAP_SHARED, MAP_SHARED);
	flags |= cvtto_bsd_mask(fl, LINUX_MAP_PRIVATE, MAP_PRIVATE);
	flags |= cvtto_bsd_mask(fl, LINUX_MAP_FIXED, MAP_FIXED);
	flags |= cvtto_bsd_mask(fl, LINUX_MAP_ANON, MAP_ANON);
	/* XXX XAX ERH: Any other flags here?  There are more defined... */

	SCARG(cma, addr) = (void *)SCARG(uap, addr);
	SCARG(cma, len) = SCARG(uap, len);
	SCARG(cma, prot) = SCARG(uap, prot);
	if (SCARG(cma, prot) & VM_PROT_WRITE) /* XXX */
		SCARG(cma, prot) |= VM_PROT_READ;
	SCARG(cma, flags) = flags;
	SCARG(cma, fd) = flags & MAP_ANON ? -1 : SCARG(uap, fd);
	SCARG(cma, pad) = 0;
}

#define	LINUX_MREMAP_MAYMOVE	1
#define	LINUX_MREMAP_FIXED	2

int
linux_sys_mremap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_mremap_args /* {
		syscallarg(void *) old_address;
		syscallarg(size_t) old_size;
		syscallarg(size_t) new_size;
		syscallarg(u_long) flags;
	} */ *uap = v;

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
		uvmflags = UVM_MREMAP_FIXED;
#else /* notyet */
		error = EOPNOTSUPP;
		goto done;
#endif /* notyet */
	} else if ((flags & LINUX_MREMAP_MAYMOVE) != 0) {
		uvmflags = 0;
	} else {
		newva = oldva;
		uvmflags = UVM_MREMAP_FIXED;
	}
	p = l->l_proc;
	map = &p->p_vmspace->vm_map;
	error = uvm_mremap(map, oldva, oldsize, map, &newva, newsize, p,
	    uvmflags);

done:
	*retval = (error != 0) ? 0 : (register_t)newva;
	return error;
}

int
linux_sys_msync(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_msync_args /* {
		syscallarg(void *) addr;
		syscallarg(int) len;
		syscallarg(int) fl;
	} */ *uap = v;

	struct sys___msync13_args bma;

	/* flags are ignored */
	SCARG(&bma, addr) = SCARG(uap, addr);
	SCARG(&bma, len) = SCARG(uap, len);
	SCARG(&bma, flags) = SCARG(uap, fl);

	return sys___msync13(l, &bma, retval);
}

int
linux_sys_mprotect(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_mprotect_args /* {
		syscallarg(const void *) start;
		syscallarg(unsigned long) len;
		syscallarg(int) prot;
	} */ *uap = v;
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

/*
 * This code is partly stolen from src/lib/libc/compat-43/times.c
 */

#define	CONVTCK(r)	(r.tv_sec * hz + r.tv_usec / (1000000 / hz))

int
linux_sys_times(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_times_args /* {
		syscallarg(struct times *) tms;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct timeval t;
	int error;

	if (SCARG(uap, tms)) {
		struct linux_tms ltms;
		struct rusage ru;

		mutex_enter(&p->p_smutex);
		calcru(p, &ru.ru_utime, &ru.ru_stime, NULL, NULL);
		ltms.ltms_utime = CONVTCK(ru.ru_utime);
		ltms.ltms_stime = CONVTCK(ru.ru_stime);
		ltms.ltms_cutime = CONVTCK(p->p_stats->p_cru.ru_utime);
		ltms.ltms_cstime = CONVTCK(p->p_stats->p_cru.ru_stime);
		mutex_exit(&p->p_smutex);

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
linux_sys_getdents(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_getdents_args /* {
		syscallarg(int) fd;
		syscallarg(struct linux_dirent *) dent;
		syscallarg(unsigned int) count;
	} */ *uap = v;
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

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(l->l_proc->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out1;
	}

	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VDIR) {
		error = EINVAL;
		goto out1;
	}

	if ((error = VOP_GETATTR(vp, &va, l->l_cred, l)))
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
	if (outp == (void *)SCARG(uap, dent))
		goto again;
	fp->f_offset = off;	/* update the vnode offset */

	if (oldcall)
		nbytes = resid + linux_reclen;

eof:
	*retval = nbytes - resid;
out:
	VOP_UNLOCK(vp, 0);
	if (cookiebuf)
		free(cookiebuf, M_TEMP);
	free(tbuf, M_TEMP);
out1:
	FILE_UNUSE(fp, l);
	return error;
}

/*
 * Even when just using registers to pass arguments to syscalls you can
 * have 5 of them on the i386. So this newer version of select() does
 * this.
 */
int
linux_sys_select(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_select_args /* {
		syscallarg(int) nfds;
		syscallarg(fd_set *) readfds;
		syscallarg(fd_set *) writefds;
		syscallarg(fd_set *) exceptfds;
		syscallarg(struct timeval *) timeout;
	} */ *uap = v;

	return linux_select1(l, retval, SCARG(uap, nfds), SCARG(uap, readfds),
	    SCARG(uap, writefds), SCARG(uap, exceptfds), SCARG(uap, timeout));
}

/*
 * Common code for the old and new versions of select(). A couple of
 * things are important:
 * 1) return the amount of time left in the 'timeout' parameter
 * 2) select never returns ERESTART on Linux, always return EINTR
 */
int
linux_select1(l, retval, nfds, readfds, writefds, exceptfds, timeout)
	struct lwp *l;
	register_t *retval;
	int nfds;
	fd_set *readfds, *writefds, *exceptfds;
	struct timeval *timeout;
{
	struct sys_select_args bsa;
	struct proc *p = l->l_proc;
	struct timeval tv0, tv1, utv, *tvp;
	void *sg;
	int error;

	SCARG(&bsa, nd) = nfds;
	SCARG(&bsa, in) = readfds;
	SCARG(&bsa, ou) = writefds;
	SCARG(&bsa, ex) = exceptfds;
	SCARG(&bsa, tv) = timeout;

	/*
	 * Store current time for computation of the amount of
	 * time left.
	 */
	if (timeout) {
		if ((error = copyin(timeout, &utv, sizeof(utv))))
			return error;
		if (itimerfix(&utv)) {
			/*
			 * The timeval was invalid.  Convert it to something
			 * valid that will act as it does under Linux.
			 */
			sg = stackgap_init(p, 0);
			tvp = stackgap_alloc(p, &sg, sizeof(utv));
			utv.tv_sec += utv.tv_usec / 1000000;
			utv.tv_usec %= 1000000;
			if (utv.tv_usec < 0) {
				utv.tv_sec -= 1;
				utv.tv_usec += 1000000;
			}
			if (utv.tv_sec < 0)
				timerclear(&utv);
			if ((error = copyout(&utv, tvp, sizeof(utv))))
				return error;
			SCARG(&bsa, tv) = tvp;
		}
		microtime(&tv0);
	}

	error = sys_select(l, &bsa, retval);
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
			microtime(&tv1);
			timersub(&tv1, &tv0, &tv1);
			timersub(&utv, &tv1, &utv);
			if (utv.tv_sec < 0)
				timerclear(&utv);
		} else
			timerclear(&utv);
		if ((error = copyout(&utv, timeout, sizeof(utv))))
			return error;
	}

	return 0;
}

/*
 * Get the process group of a certain process. Look it up
 * and return the value.
 */
int
linux_sys_getpgid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_getpgid_args /* {
		syscallarg(int) pid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct proc *targp;

	if (SCARG(uap, pid) != 0 && SCARG(uap, pid) != p->p_pid) {
		if ((targp = pfind(SCARG(uap, pid))) == 0)
			return ESRCH;
	}
	else
		targp = p;

	retval[0] = targp->p_pgid;
	return 0;
}

/*
 * Set the 'personality' (emulation mode) for the current process. Only
 * accept the Linux personality here (0). This call is needed because
 * the Linux ELF crt0 issues it in an ugly kludge to make sure that
 * ELF binaries run in Linux mode, not SVR4 mode.
 */
int
linux_sys_personality(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_personality_args /* {
		syscallarg(int) per;
	} */ *uap = v;

	if (SCARG(uap, per) != 0)
		return EINVAL;
	retval[0] = 0;
	return 0;
}
#endif /* !COMPAT_LINUX32 */

#if defined(__i386__) || defined(__m68k__) || defined(COMPAT_LINUX32)
/*
 * The calls are here because of type conversions.
 */
int
linux_sys_setreuid16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_setreuid16_args /* {
		syscallarg(int) ruid;
		syscallarg(int) euid;
	} */ *uap = v;
	struct sys_setreuid_args bsa;

	SCARG(&bsa, ruid) = ((linux_uid_t)SCARG(uap, ruid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, ruid);
	SCARG(&bsa, euid) = ((linux_uid_t)SCARG(uap, euid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, euid);

	return sys_setreuid(l, &bsa, retval);
}

int
linux_sys_setregid16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_setregid16_args /* {
		syscallarg(int) rgid;
		syscallarg(int) egid;
	} */ *uap = v;
	struct sys_setregid_args bsa;

	SCARG(&bsa, rgid) = ((linux_gid_t)SCARG(uap, rgid) == (linux_gid_t)-1) ?
		(uid_t)-1 : SCARG(uap, rgid);
	SCARG(&bsa, egid) = ((linux_gid_t)SCARG(uap, egid) == (linux_gid_t)-1) ?
		(uid_t)-1 : SCARG(uap, egid);

	return sys_setregid(l, &bsa, retval);
}

int
linux_sys_setresuid16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_setresuid16_args /* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
		syscallarg(uid_t) suid;
	} */ *uap = v;
	struct linux_sys_setresuid16_args lsa;

	SCARG(&lsa, ruid) = ((linux_uid_t)SCARG(uap, ruid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, ruid);
	SCARG(&lsa, euid) = ((linux_uid_t)SCARG(uap, euid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, euid);
	SCARG(&lsa, suid) = ((linux_uid_t)SCARG(uap, suid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, suid);

	return linux_sys_setresuid(l, &lsa, retval);
}

int
linux_sys_setresgid16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_setresgid16_args /* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
		syscallarg(gid_t) sgid;
	} */ *uap = v;
	struct linux_sys_setresgid16_args lsa;

	SCARG(&lsa, rgid) = ((linux_gid_t)SCARG(uap, rgid) == (linux_gid_t)-1) ?
		(gid_t)-1 : SCARG(uap, rgid);
	SCARG(&lsa, egid) = ((linux_gid_t)SCARG(uap, egid) == (linux_gid_t)-1) ?
		(gid_t)-1 : SCARG(uap, egid);
	SCARG(&lsa, sgid) = ((linux_gid_t)SCARG(uap, sgid) == (linux_gid_t)-1) ?
		(gid_t)-1 : SCARG(uap, sgid);

	return linux_sys_setresgid(l, &lsa, retval);
}

int
linux_sys_getgroups16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_getgroups16_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(linux_gid_t *) gidset;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	void *sg;
	int n, error, i;
	struct sys_getgroups_args bsa;
	gid_t *bset, *kbset;
	linux_gid_t *lset;
	kauth_cred_t pc = l->l_cred;

	n = SCARG(uap, gidsetsize);
	if (n < 0)
		return EINVAL;
	error = 0;
	bset = kbset = NULL;
	lset = NULL;
	if (n > 0) {
		n = min(kauth_cred_ngroups(pc), n);
		sg = stackgap_init(p, 0);
		bset = stackgap_alloc(p, &sg, n * sizeof (gid_t));
		kbset = malloc(n * sizeof (gid_t), M_TEMP, M_WAITOK);
		lset = malloc(n * sizeof (linux_gid_t), M_TEMP, M_WAITOK);
		if (bset == NULL || kbset == NULL || lset == NULL)
		{
			error = ENOMEM;
			goto out;
		}
		SCARG(&bsa, gidsetsize) = n;
		SCARG(&bsa, gidset) = bset;
		error = sys_getgroups(l, &bsa, retval);
		if (error != 0)
			goto out;
		error = copyin(bset, kbset, n * sizeof (gid_t));
		if (error != 0)
			goto out;
		for (i = 0; i < n; i++)
			lset[i] = (linux_gid_t)kbset[i];
		error = copyout(lset, SCARG(uap, gidset),
		    n * sizeof (linux_gid_t));
	} else
		*retval = kauth_cred_ngroups(pc);
out:
	if (kbset != NULL)
		free(kbset, M_TEMP);
	if (lset != NULL)
		free(lset, M_TEMP);
	return error;
}

int
linux_sys_setgroups16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_setgroups16_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(linux_gid_t *) gidset;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	void *sg;
	int n;
	int error, i;
	struct sys_setgroups_args bsa;
	gid_t *bset, *kbset;
	linux_gid_t *lset;

	n = SCARG(uap, gidsetsize);
	if (n < 0 || n > NGROUPS)
		return EINVAL;
	sg = stackgap_init(p, 0);
	bset = stackgap_alloc(p, &sg, n * sizeof (gid_t));
	lset = malloc(n * sizeof (linux_gid_t), M_TEMP, M_WAITOK);
	kbset = malloc(n * sizeof (gid_t), M_TEMP, M_WAITOK);
	if (bset == NULL || kbset == NULL || lset == NULL)
	{
		error = ENOMEM;
		goto out;
	}
	error = copyin(SCARG(uap, gidset), lset, n * sizeof (linux_gid_t));
	if (error != 0)
		goto out;
	for (i = 0; i < n; i++)
		kbset[i] = (gid_t)lset[i];
	error = copyout(kbset, bset, n * sizeof (gid_t));
	if (error != 0)
		goto out;
	SCARG(&bsa, gidsetsize) = n;
	SCARG(&bsa, gidset) = bset;
	error = sys_setgroups(l, &bsa, retval);

out:
	if (lset != NULL)
		free(lset, M_TEMP);
	if (kbset != NULL)
		free(kbset, M_TEMP);

	return error;
}

#endif /* __i386__ || __m68k__ || COMPAT_LINUX32 */

#ifndef COMPAT_LINUX32
/*
 * We have nonexistent fsuid equal to uid.
 * If modification is requested, refuse.
 */
int
linux_sys_setfsuid(l, v, retval)
	 struct lwp *l;
	 void *v;
	 register_t *retval;
{
	 struct linux_sys_setfsuid_args /* {
		 syscallarg(uid_t) uid;
	 } */ *uap = v;
	 uid_t uid;

	 uid = SCARG(uap, uid);
	 if (kauth_cred_getuid(l->l_cred) != uid)
		 return sys_nosys(l, v, retval);
	 else
		 return (0);
}

/* XXX XXX XXX */
# ifndef alpha
int
linux_sys_getfsuid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	return sys_getuid(l, v, retval);
}
# endif

int
linux_sys_setresuid(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_setresuid_args /* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
		syscallarg(uid_t) suid;
	} */ *uap = v;

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
linux_sys_getresuid(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_getresuid_args /* {
		syscallarg(uid_t *) ruid;
		syscallarg(uid_t *) euid;
		syscallarg(uid_t *) suid;
	} */ *uap = v;
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
linux_sys_ptrace(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
#if defined(PTRACE) || defined(_LKM)
	struct linux_sys_ptrace_args /* {
		i386, m68k, powerpc: T=int
		alpha, amd64: T=long
		syscallarg(T) request;
		syscallarg(T) pid;
		syscallarg(T) addr;
		syscallarg(T) data;
	} */ *uap = v;
	const int *ptr;
	int request;
	int error;
#ifdef _LKM
#define sys_ptrace (*sysent[SYS_ptrace].sy_call)
#endif

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

			error = sys_ptrace(l, &pta, retval);
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
#else
	return ENOSYS;
#endif /* PTRACE || _LKM */
}

int
linux_sys_reboot(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_reboot_args /* {
		syscallarg(int) magic1;
		syscallarg(int) magic2;
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */ *uap = v;
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

	switch (SCARG(uap, cmd)) {
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
linux_sys_swapon(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_swapctl_args ua;
	struct linux_sys_swapon_args /* {
		syscallarg(const char *) name;
	} */ *uap = v;

	SCARG(&ua, cmd) = SWAP_ON;
	SCARG(&ua, arg) = (void *)__UNCONST(SCARG(uap, name));
	SCARG(&ua, misc) = 0;	/* priority */
	return (sys_swapctl(l, &ua, retval));
}

/*
 * Stop swapping to the file or block device specified by path.
 */
int
linux_sys_swapoff(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_swapctl_args ua;
	struct linux_sys_swapoff_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;

	SCARG(&ua, cmd) = SWAP_OFF;
	SCARG(&ua, arg) = __UNCONST(SCARG(uap, path)); /*XXXUNCONST*/
	return (sys_swapctl(l, &ua, retval));
}

/*
 * Copy of compat_09_sys_setdomainname()
 */
/* ARGSUSED */
int
linux_sys_setdomainname(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_setdomainname_args /* {
		syscallarg(char *) domainname;
		syscallarg(int) len;
	} */ *uap = v;
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
linux_sys_sysinfo(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_sysinfo_args /* {
		syscallarg(struct linux_sysinfo *) arg;
	} */ *uap = v;
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
linux_sys_getrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_getrlimit_args /* {
		syscallarg(int) which;
# ifdef LINUX_LARGEFILE64
		syscallarg(struct rlimit *) rlp;
# else
		syscallarg(struct orlimit *) rlp;
# endif
	} */ *uap = v;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sys_getrlimit_args ap;
	struct rlimit rl;
# ifdef LINUX_LARGEFILE64
	struct rlimit orl;
# else
	struct orlimit orl;
# endif
	int error;

	SCARG(&ap, which) = linux_to_bsd_limit(SCARG(uap, which));
	if ((error = SCARG(&ap, which)) < 0)
		return -error;
	SCARG(&ap, rlp) = stackgap_alloc(p, &sg, sizeof rl);
	if ((error = sys_getrlimit(l, &ap, retval)) != 0)
		return error;
	if ((error = copyin(SCARG(&ap, rlp), &rl, sizeof(rl))) != 0)
		return error;
	bsd_to_linux_rlimit(&orl, &rl);

	return copyout(&orl, SCARG(uap, rlp), sizeof(orl));
}

int
linux_sys_setrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_setrlimit_args /* {
		syscallarg(int) which;
# ifdef LINUX_LARGEFILE64
		syscallarg(struct rlimit *) rlp;
# else
		syscallarg(struct orlimit *) rlp;
# endif
	} */ *uap = v;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sys_getrlimit_args ap;
	struct rlimit rl;
# ifdef LINUX_LARGEFILE64
	struct rlimit orl;
# else
	struct orlimit orl;
# endif
	int error;

	SCARG(&ap, which) = linux_to_bsd_limit(SCARG(uap, which));
	SCARG(&ap, rlp) = stackgap_alloc(p, &sg, sizeof rl);
	if ((error = SCARG(&ap, which)) < 0)
		return -error;
	if ((error = copyin(SCARG(uap, rlp), &orl, sizeof(orl))) != 0)
		return error;
	linux_to_bsd_rlimit(&rl, &orl);
	if ((error = copyout(&rl, SCARG(&ap, rlp), sizeof(rl))) != 0)
		return error;
	return sys_setrlimit(l, &ap, retval);
}

# if !defined(__mips__) && !defined(__amd64__)
/* XXX: this doesn't look 100% common, at least mips doesn't have it */
int
linux_sys_ugetrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	return linux_sys_getrlimit(l, v, retval);
}
# endif

/*
 * This gets called for unsupported syscalls. The difference to sys_nosys()
 * is that process does not get SIGSYS, the call just returns with ENOSYS.
 * This is the way Linux does it and glibc depends on this behaviour.
 */
int
linux_sys_nosys(struct lwp *l, void *v,
    register_t *retval)
{
	return (ENOSYS);
}

int
linux_sys_getpriority(l, v, retval)
        struct lwp *l;
        void *v;
        register_t *retval;
{
        struct linux_sys_getpriority_args /* {
                syscallarg(int) which;
                syscallarg(int) who;
        } */ *uap = v;
        struct sys_getpriority_args bsa;
        int error;

        SCARG(&bsa, which) = SCARG(uap, which);
        SCARG(&bsa, who) = SCARG(uap, who);

        if ((error = sys_getpriority(l, &bsa, retval)))
                return error;

        *retval = NZERO - *retval;

        return 0;
}

#endif /* !COMPAT_LINUX32 */
