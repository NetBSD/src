/* $NetBSD: osf1_misc.c,v 1.38 1999/05/01 02:57:10 cgd Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/socketvar.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>
#include <vm/vm.h>				/* XXX UVM headers are Cool */
#include <uvm/uvm.h>				/* XXX see mmap emulation */

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscall.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/osf1/osf1_util.h>
#include <compat/osf1/osf1_cvt.h>

#include <vm/vm.h>

void cvtstat2osf1 __P((struct stat *, struct osf1_stat *));
void cvtrusage2osf1 __P((struct rusage *, struct osf1_rusage *));

#ifdef SYSCALL_DEBUG
extern int scdebug;
#endif

const char osf1_emul_path[] = "/emul/osf1";

int
osf1_sys_open(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_open_args *uap = v;
	struct sys_open_args a;
	const char *path;
	caddr_t sg;
	unsigned long leftovers;
#ifdef SYSCALL_DEBUG
	char pnbuf[1024];

	if (scdebug &&
	    copyinstr(SCARG(uap, path), pnbuf, sizeof pnbuf, NULL) == 0)
		printf("osf1_open: open: %s\n", pnbuf);
#endif

	sg = stackgap_init(p->p_emul);

	/* translate flags */
	SCARG(&a, flags) = emul_flags_translate(osf1_open_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	/* copy mode, no translation necessary */
	SCARG(&a, mode) = SCARG(uap, mode);

	/* pick appropriate path */
	path = SCARG(uap, path);
	if (SCARG(&a, flags) & O_CREAT)
		OSF1_CHECK_ALT_CREAT(p, &sg, path);
	else
		OSF1_CHECK_ALT_EXIST(p, &sg, path);
	SCARG(&a, path) = path;

	return sys_open(p, &a, retval);
}

int
osf1_sys_setsysinfo(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	/* XXX */
	return (0);
}

int
osf1_sys_getrlimit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_getrlimit_args *uap = v;
	struct sys_getrlimit_args a;

	switch (SCARG(uap, which)) {
	case OSF1_RLIMIT_CPU:
		SCARG(&a, which) = RLIMIT_CPU;
		break;
	case OSF1_RLIMIT_FSIZE:
		SCARG(&a, which) = RLIMIT_FSIZE;
		break;
	case OSF1_RLIMIT_DATA:
		SCARG(&a, which) = RLIMIT_DATA;
		break;
	case OSF1_RLIMIT_STACK:
		SCARG(&a, which) = RLIMIT_STACK;
		break;
	case OSF1_RLIMIT_CORE:
		SCARG(&a, which) = RLIMIT_CORE;
		break;
	case OSF1_RLIMIT_RSS:
		SCARG(&a, which) = RLIMIT_RSS;
		break;
	case OSF1_RLIMIT_NOFILE:
		SCARG(&a, which) = RLIMIT_NOFILE;
		break;
	case OSF1_RLIMIT_AS:		/* unhandled */
	default:
		return (EINVAL);
	}

	/* XXX should translate */
	SCARG(&a, rlp) = SCARG(uap, rlp);

	return sys_getrlimit(p, &a, retval);
}

int
osf1_sys_setrlimit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_setrlimit_args *uap = v;
	struct sys_setrlimit_args a;

	switch (SCARG(uap, which)) {
	case OSF1_RLIMIT_CPU:
		SCARG(&a, which) = RLIMIT_CPU;
		break;
	case OSF1_RLIMIT_FSIZE:
		SCARG(&a, which) = RLIMIT_FSIZE;
		break;
	case OSF1_RLIMIT_DATA:
		SCARG(&a, which) = RLIMIT_DATA;
		break;
	case OSF1_RLIMIT_STACK:
		SCARG(&a, which) = RLIMIT_STACK;
		break;
	case OSF1_RLIMIT_CORE:
		SCARG(&a, which) = RLIMIT_CORE;
		break;
	case OSF1_RLIMIT_RSS:
		SCARG(&a, which) = RLIMIT_RSS;
		break;
	case OSF1_RLIMIT_NOFILE:
		SCARG(&a, which) = RLIMIT_NOFILE;
		break;
	case OSF1_RLIMIT_AS:		/* unhandled */
	default:
		return (EINVAL);
	}

	/* XXX should translate */
	SCARG(&a, rlp) = SCARG(uap, rlp);

	return sys_setrlimit(p, &a, retval);
}

int
osf1_sys_mmap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_mmap_args *uap = v;
	struct sys_mmap_args a;
	unsigned long leftovers;

	SCARG(&a, addr) = SCARG(uap, addr);
	SCARG(&a, len) = SCARG(uap, len);
	SCARG(&a, fd) = SCARG(uap, fd);
	SCARG(&a, pad) = 0;
	SCARG(&a, pos) = SCARG(uap, pos);

	/* translate prot */
	SCARG(&a, prot) = emul_flags_translate(osf1_mmap_prot_xtab,
	    SCARG(uap, prot), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	/* translate flags */
	SCARG(&a, flags) = emul_flags_translate(osf1_mmap_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	/*
	 * XXX The following code is evil.
	 *
	 * The OSF/1 mmap() function attempts to map non-fixed entries
	 * near the address that the user specified.  Therefore, for
	 * non-fixed entires we try to find space in the address space
	 * starting at that address.  If the user specified zero, we
	 * start looking at at least NBPG, so that programs can't
	 * accidentally live through deferencing NULL.
	 *
	 * The need for this kludgery is increased by the fact that
	 * the loader data segment is mapped at
	 * (end of user address space) - 1G, MAXDSIZ is 1G, and
	 * the VM system tries allocate non-fixed mappings _AFTER_
	 * (start of data) + MAXDSIZ.  With the loader, of course,
	 * that means that it'll start trying at
	 * (end of user address space), and will never succeed!
	 *
	 * Notes:
	 *
	 * * Though we find space here, if something else (e.g. a second
	 *   thread) were mucking with the address space the mapping
	 *   we found might be used by another mmap(), and this call
	 *   would clobber that region.
	 *
	 * * In general, tricks like this only work for MAP_ANON mappings,
	 *   because of sharing/cache issues.  That's not a problem on
	 *   the Alpha, and though it's not good style to abuse that fact,
	 *   there's little choice.
	 *
	 * * In order for this to be done right, the VM system should
	 *   really try to use the requested 'addr' passed in to mmap()
	 *   as a hint, even if non-fixed.  If it's passed as zero,
	 *   _maybe_ then try (start of data) + MAXDSIZ, or maybe
	 *   provide a better way to avoid the data region altogether.
	 */
	if ((SCARG(&a, flags) & MAP_FIXED) == 0) {
		vaddr_t addr = round_page(SCARG(&a, addr));
		vsize_t size = round_page(SCARG(&a, len));
		int fixed = 0;

		vm_map_lock(&p->p_vmspace->vm_map);

		/* if non-NULL address given, start looking there */
		if (addr != 0 && uvm_map_findspace(&p->p_vmspace->vm_map,
		    addr, size, &addr, NULL, 0, 0) != NULL) {
			fixed = 1;
			goto done;
		}

		/* didn't find anything.  take it again from the top. */
		if (uvm_map_findspace(&p->p_vmspace->vm_map, NBPG, size, &addr,
		    NULL, 0, 0) != NULL) {
			fixed = 1;
			goto done;
		}

done:
		vm_map_unlock(&p->p_vmspace->vm_map);
		if (fixed) {
			SCARG(&a, flags) |= MAP_FIXED;
			SCARG(&a, addr) = (void *)addr;
		}
	}

	return sys_mmap(p, &a, retval);
}

int
osf1_sys_mprotect(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_mprotect_args *uap = v;
	struct sys_mprotect_args a;
	unsigned long leftovers;

	SCARG(&a, addr) = SCARG(uap, addr);
	SCARG(&a, len) = SCARG(uap, len);

	/* translate prot */
	SCARG(&a, prot) = emul_flags_translate(osf1_mmap_prot_xtab,
	    SCARG(uap, prot), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	return sys_mprotect(p, &a, retval);
}

int
osf1_sys_usleep_thread(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_usleep_thread_args *uap = v;
	struct osf1_timeval otv, endotv;
	struct timeval tv, endtv;
	u_long ticks;
	int error, s;

	if ((error = copyin(SCARG(uap, sleep), &otv, sizeof otv)))
		return (error);
	tv.tv_sec = otv.tv_sec;
	tv.tv_usec = otv.tv_usec;

	ticks = ((u_long)tv.tv_sec * 1000000 + tv.tv_usec) / tick;
	s = splclock();
	tv = time;
	splx(s);

	tsleep(p, PUSER|PCATCH, "OSF/1", ticks);	/* XXX */

	if (SCARG(uap, slept) != NULL) {
		s = splclock();
		timersub(&time, &tv, &endtv);
		splx(s);
		if (endtv.tv_sec < 0 || endtv.tv_usec < 0)
			endtv.tv_sec = endtv.tv_usec = 0;

		endotv.tv_sec = endtv.tv_sec;
		endotv.tv_usec = endtv.tv_usec;
		error = copyout(&endotv, SCARG(uap, slept), sizeof endotv);
	}
	return (error);
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
osf1_sys_stat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_stat_args *uap = v;
	struct stat sb;
	struct osf1_stat osb;
	int error;
	struct nameidata nd;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);
	OSF1_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	cvtstat2osf1(&sb, &osb);
	error = copyout((caddr_t)&osb, (caddr_t)SCARG(uap, ub), sizeof (osb));
	return (error);
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
osf1_sys_lstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_lstat_args *uap = v;
	struct stat sb;
	struct osf1_stat osb;
	int error;
	struct nameidata nd;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);
	OSF1_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	cvtstat2osf1(&sb, &osb);
	error = copyout((caddr_t)&osb, (caddr_t)SCARG(uap, ub), sizeof (osb));
	return (error);
}

/*
 * Return status information about a file descriptor.
 */
int
osf1_sys_fstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_fstat_args *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct stat ub;
	struct osf1_stat oub;
	int error;

	if ((unsigned)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL)
		return (EBADF);
	switch (fp->f_type) {

	case DTYPE_VNODE:
		error = vn_stat((struct vnode *)fp->f_data, &ub, p);
		break;

	case DTYPE_SOCKET:
		error = soo_stat((struct socket *)fp->f_data, &ub);
		break;

	default:
		panic("ofstat");
		/*NOTREACHED*/
	}
	cvtstat2osf1(&ub, &oub);
	if (error == 0)
		error = copyout((caddr_t)&oub, (caddr_t)SCARG(uap, sb),
		    sizeof (oub));
	return (error);
}

#define	bsd2osf_dev(dev)	osf1_makedev(major(dev), minor(dev))
#define	osf2bsd_dev(dev)	makedev(osf1_major(dev), osf1_minor(dev))

/*
 * Convert from a stat structure to an osf1 stat structure.
 */
void
cvtstat2osf1(st, ost)
	struct stat *st;
	struct osf1_stat *ost;
{

	ost->st_dev = bsd2osf_dev(st->st_dev);
	ost->st_ino = st->st_ino;
	ost->st_mode = st->st_mode;
	ost->st_nlink = st->st_nlink;
	ost->st_uid = st->st_uid == -2 ? (u_int16_t) -2 : st->st_uid;
	ost->st_gid = st->st_gid == -2 ? (u_int16_t) -2 : st->st_gid;
	ost->st_rdev = bsd2osf_dev(st->st_rdev);
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

int
osf1_sys_mknod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_mknod_args *uap = v;
	struct sys_mknod_args a;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);
	OSF1_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&a, path) = SCARG(uap, path);
	SCARG(&a, mode) = SCARG(uap, mode);
	SCARG(&a, dev) = osf2bsd_dev(SCARG(uap, dev));

	return sys_mknod(p, &a, retval);
}

int
osf1_sys_fcntl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_fcntl_args *uap = v;
	struct sys_fcntl_args a;
	unsigned long xfl, leftovers;
	int error;

	SCARG(&a, fd) = SCARG(uap, fd);

	leftovers = 0;
	switch (SCARG(uap, cmd)) {
	case OSF1_F_DUPFD:
		SCARG(&a, cmd) = F_DUPFD;
		SCARG(&a, arg) = SCARG(uap, arg);
		break;

	case OSF1_F_GETFD:
		SCARG(&a, cmd) = F_GETFD;
		SCARG(&a, arg) = 0;		/* ignored */
		break;

	case OSF1_F_SETFD:
		SCARG(&a, cmd) = F_SETFD;
		SCARG(&a, arg) = (void *)emul_flags_translate(
		    osf1_fcntl_getsetfd_flags_xtab,
		    (unsigned long)SCARG(uap, arg), &leftovers);
		break;

	case OSF1_F_GETFL:
		SCARG(&a, cmd) = F_GETFL;
		SCARG(&a, arg) = 0;		/* ignored */
		break;

	case OSF1_F_SETFL:
		SCARG(&a, cmd) = F_SETFL;
		xfl = emul_flags_translate(osf1_open_flags_xtab,
		    (unsigned long)SCARG(uap, arg), &leftovers);
		xfl |= emul_flags_translate(osf1_fcntl_getsetfl_flags_xtab,
		    leftovers, &leftovers);
		SCARG(&a, arg) = (void *)xfl;
		break;

	case OSF1_F_GETOWN:		/* XXX not yet supported */
	case OSF1_F_SETOWN:		/* XXX not yet supported */
	case OSF1_F_GETLK:		/* XXX not yet supported */
	case OSF1_F_SETLK:		/* XXX not yet supported */
	case OSF1_F_SETLKW:		/* XXX not yet supported */
		/* XXX translate. */
		return (EINVAL);
		
	case OSF1_F_RGETLK:		/* [lock mgr op] XXX not supported */
	case OSF1_F_RSETLK:		/* [lock mgr op] XXX not supported */
	case OSF1_F_CNVT:		/* [lock mgr op] XXX not supported */
	case OSF1_F_RSETLKW:		/* [lock mgr op] XXX not supported */
	case OSF1_F_PURGEFS:		/* [lock mgr op] XXX not supported */
	case OSF1_F_PURGENFS:		/* [DECsafe op] XXX not supported */
	default:
		/* XXX syslog? */
		return (EINVAL);
	}
	if (leftovers != 0)
		return (EINVAL);

	error = sys_fcntl(p, &a, retval);

	if (error)
		return error;

	switch (SCARG(uap, cmd)) {
	case OSF1_F_GETFD:
		retval[0] = emul_flags_translate(
		    osf1_fcntl_getsetfd_flags_rxtab, retval[0], NULL);
		break;

	case OSF1_F_GETFL:
		xfl = emul_flags_translate(osf1_open_flags_rxtab,
		    retval[0], &leftovers);
		xfl |= emul_flags_translate(osf1_fcntl_getsetfl_flags_rxtab,
		    leftovers, NULL);
		retval[0] = xfl;
		break;
	}

	return error;
}

int
osf1_sys_socket(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_socket_args *uap = v;
	struct sys_socket_args a;

	/* XXX TRANSLATE */

	if (SCARG(uap, type) > AF_LINK)
		return (EINVAL);	/* XXX After AF_LINK, divergence. */

	SCARG(&a, domain) = SCARG(uap, domain);
	SCARG(&a, type) = SCARG(uap, type);
	SCARG(&a, protocol) = SCARG(uap, protocol);

	return sys_socket(p, &a, retval);
}

int
osf1_sys_sendto(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_sendto_args *uap = v;
	struct sys_sendto_args a;
	unsigned long leftovers;

	SCARG(&a, s) = SCARG(uap, s);
	SCARG(&a, buf) = SCARG(uap, buf);
	SCARG(&a, len) = SCARG(uap, len);
	SCARG(&a, to) = SCARG(uap, to);
	SCARG(&a, tolen) = SCARG(uap, tolen);

	/* translate flags */
	SCARG(&a, flags) = emul_flags_translate(osf1_sendrecv_msg_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	return sys_sendto(p, &a, retval);
}

int
osf1_sys_reboot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_reboot_args *uap = v;
	struct sys_reboot_args a;
	unsigned long leftovers;

	/* translate opt */
	SCARG(&a, opt) = emul_flags_translate(osf1_reboot_opt_xtab,
	    SCARG(uap, opt), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	SCARG(&a, bootstr) = NULL;

	return sys_reboot(p, &a, retval);
}

int
osf1_sys_lseek(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_lseek_args *uap = v;
	struct sys_lseek_args a;

	SCARG(&a, fd) = SCARG(uap, fd);
	SCARG(&a, pad) = 0;
	SCARG(&a, offset) = SCARG(uap, offset);
	SCARG(&a, whence) = SCARG(uap, whence);

	return sys_lseek(p, &a, retval);
}

/*
 * OSF/1 defines _POSIX_SAVED_IDS, which means that our normal
 * setuid() won't work.
 *
 * Instead, by P1003.1b-1993, setuid() is supposed to work like:
 *	If the process has appropriate [super-user] priviledges, the
 *	    setuid() function sets the real user ID, effective user
 *	    ID, and the saved set-user-ID to uid.
 *	If the process does not have appropriate priviledges, but uid
 *	    is equal to the real user ID or the saved set-user-ID, the
 *	    setuid() function sets the effective user ID to uid; the
 *	    real user ID and saved set-user-ID remain unchanged by
 *	    this function call.
 */
int
osf1_sys_setuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_setuid_args *uap = v;
	struct pcred *pc = p->p_cred;
	uid_t uid = SCARG(uap, uid);
	int error;

	if ((error = suser(pc->pc_ucred, &p->p_acflag)) != 0 &&
	    uid != pc->p_ruid && uid != pc->p_svuid)
		return (error);

	pc->pc_ucred = crcopy(pc->pc_ucred);
	pc->pc_ucred->cr_uid = uid;
	if (error == 0) {
	        (void)chgproccnt(pc->p_ruid, -1);
	        (void)chgproccnt(uid, 1);
		pc->p_ruid = uid;
		pc->p_svuid = uid;
	}
	p->p_flag |= P_SUGID;
	return (0);
}

/*
 * OSF/1 defines _POSIX_SAVED_IDS, which means that our normal
 * setgid() won't work.
 *
 * If you change "uid" to "gid" in the discussion, above, about
 * setuid(), you'll get a correct description of setgid().
 */
int
osf1_sys_setgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_setgid_args *uap = v;
	struct pcred *pc = p->p_cred;
	gid_t gid = SCARG(uap, gid);
	int error;

	if ((error = suser(pc->pc_ucred, &p->p_acflag)) != 0 &&
	    gid != pc->p_rgid && gid != pc->p_svgid)
		return (error);

	pc->pc_ucred = crcopy(pc->pc_ucred);
	pc->pc_ucred->cr_gid = gid;
	if (error == 0) {
		pc->p_rgid = gid;
		pc->p_svgid = gid;
	}
	p->p_flag |= P_SUGID;
	return (0);
}

/*
 * The structures end up being the same... but we can't be sure that
 * the other word of our iov_len is zero!
 */
int
osf1_sys_readv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_readv_args *uap = v;
	struct sys_readv_args a;
	struct osf1_iovec *oio;
	struct iovec *nio;
	caddr_t sg = stackgap_init(p->p_emul);
	int error, osize, nsize, i;

	if (SCARG(uap, iovcnt) > (STACKGAPLEN / sizeof (struct iovec)))
		return (EINVAL);

	osize = SCARG(uap, iovcnt) * sizeof (struct osf1_iovec);
	nsize = SCARG(uap, iovcnt) * sizeof (struct iovec);

	oio = malloc(osize, M_TEMP, M_WAITOK);
	nio = malloc(nsize, M_TEMP, M_WAITOK);

	error = 0;
	if ((error = copyin(SCARG(uap, iovp), oio, osize)))
		goto punt;
	for (i = 0; i < SCARG(uap, iovcnt); i++) {
		nio[i].iov_base = oio[i].iov_base;
		nio[i].iov_len = oio[i].iov_len;
	}

	SCARG(&a, fd) = SCARG(uap, fd);
	SCARG(&a, iovp) = stackgap_alloc(&sg, nsize);
	SCARG(&a, iovcnt) = SCARG(uap, iovcnt);

	if ((error = copyout(nio, (caddr_t)SCARG(&a, iovp), nsize)))
		goto punt;
	error = sys_readv(p, &a, retval);

punt:
	free(oio, M_TEMP);
	free(nio, M_TEMP);
	return (error);
}

int
osf1_sys_writev(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_writev_args *uap = v;
	struct sys_writev_args a;
	struct osf1_iovec *oio;
	struct iovec *nio;
	caddr_t sg = stackgap_init(p->p_emul);
	int error, osize, nsize, i;

	if (SCARG(uap, iovcnt) > (STACKGAPLEN / sizeof (struct iovec)))
		return (EINVAL);

	osize = SCARG(uap, iovcnt) * sizeof (struct osf1_iovec);
	nsize = SCARG(uap, iovcnt) * sizeof (struct iovec);

	oio = malloc(osize, M_TEMP, M_WAITOK);
	nio = malloc(nsize, M_TEMP, M_WAITOK);

	error = 0;
	if ((error = copyin(SCARG(uap, iovp), oio, osize)))
		goto punt;
	for (i = 0; i < SCARG(uap, iovcnt); i++) {
		nio[i].iov_base = oio[i].iov_base;
		nio[i].iov_len = oio[i].iov_len;
	}

	SCARG(&a, fd) = SCARG(uap, fd);
	SCARG(&a, iovp) = stackgap_alloc(&sg, nsize);
	SCARG(&a, iovcnt) = SCARG(uap, iovcnt);

	if ((error = copyout(nio, (caddr_t)SCARG(&a, iovp), nsize)))
		goto punt;
	error = sys_writev(p, &a, retval);

punt:
	free(oio, M_TEMP);
	free(nio, M_TEMP);
	return (error);
}

/* More of the stupid off_t padding! */
int
osf1_sys_truncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_truncate_args *uap = v;
	struct sys_truncate_args a;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);
	OSF1_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&a, path) = SCARG(uap, path);
	SCARG(&a, pad) = 0;
	SCARG(&a, length) = SCARG(uap, length);

	return sys_truncate(p, &a, retval);
}

int
osf1_sys_ftruncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_ftruncate_args *uap = v;
	struct sys_ftruncate_args a;

	SCARG(&a, fd) = SCARG(uap, fd);
	SCARG(&a, pad) = 0;
	SCARG(&a, length) = SCARG(uap, length);

	return sys_ftruncate(p, &a, retval);
}

int
osf1_sys_getrusage(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_getrusage_args *uap = v;
	struct sys_getrusage_args a;
	struct osf1_rusage osf1_rusage;
	struct rusage netbsd_rusage;
	caddr_t sg;
	int error;

	switch (SCARG(uap, who)) {
	case OSF1_RUSAGE_SELF:
		SCARG(&a, who) = RUSAGE_SELF;
		break;

	case OSF1_RUSAGE_CHILDREN:
		SCARG(&a, who) = RUSAGE_CHILDREN;
		break;

	case OSF1_RUSAGE_THREAD:		/* XXX not supported */
	default:
		return (EINVAL);
	}

	sg = stackgap_init(p->p_emul);
	SCARG(&a, rusage) = stackgap_alloc(&sg, sizeof netbsd_rusage);

	error = sys_getrusage(p, &a, retval);
	if (error == 0)
                error = copyin((caddr_t)SCARG(&a, rusage),
		    (caddr_t)&netbsd_rusage, sizeof netbsd_rusage);
	if (error == 0) {
		cvtrusage2osf1(&netbsd_rusage, &osf1_rusage);
                error = copyout((caddr_t)&osf1_rusage,
		    (caddr_t)SCARG(uap, rusage), sizeof osf1_rusage);
	}

	return (error);
}

/*
 * Convert from as rusage structure to an osf1 rusage structure.
 */
void
cvtrusage2osf1(ru, oru)
	struct rusage *ru;
	struct osf1_rusage *oru;
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

int
osf1_sys_madvise(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_madvise_args *uap = v;
	struct sys_madvise_args a;
	int error;

	SCARG(&a, addr) = SCARG(uap, addr);
	SCARG(&a, len) = SCARG(uap, len);

	error = 0;
	switch (SCARG(uap, behav)) {
	case OSF1_MADV_NORMAL:
		SCARG(&a, behav) = MADV_NORMAL;
		break;

	case OSF1_MADV_RANDOM:
		SCARG(&a, behav) = MADV_RANDOM;
		break;

	case OSF1_MADV_SEQUENTIAL:
		SCARG(&a, behav) = MADV_SEQUENTIAL;
		break;

	case OSF1_MADV_WILLNEED:
		SCARG(&a, behav) = MADV_WILLNEED;
		break;

	case OSF1_MADV_DONTNEED_COMPAT:
		SCARG(&a, behav) = MADV_DONTNEED;
		break;

	case OSF1_MADV_SPACEAVAIL:
		SCARG(&a, behav) = MADV_SPACEAVAIL;
		break;

	case OSF1_MADV_DONTNEED:
		/*
		 * XXX not supported.  In Digital UNIX, this flushes all
		 * XXX data in the region and replaces it with ZFOD pages.
		 */
		error = EINVAL;
		break;

	default:
		error = EINVAL;
		break;
	}

	if (error == 0) {
		error = sys_madvise(p, &a, retval);

		/*
		 * NetBSD madvise() currently always returns ENOSYS.
		 * Digital UNIX says that non-operational requests (i.e.
		 * valid, but unimplemented 'behav') will return success.
		 */
		if (error == ENOSYS)
			error = 0;
	}
	return (error);
}

int
osf1_sys_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_execve_args *uap = v;
	struct sys_execve_args ap;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);
	OSF1_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(p, &ap, retval);
}

int
osf1_sys_uname(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_uname_args *uap = v;
        struct osf1_utsname u;
        char *cp, *dp, *ep;
        extern char ostype[], osrelease[];

	/* XXX would use stackgap, but our struct utsname is too big! */

        strncpy(u.sysname, ostype, sizeof(u.sysname));
        strncpy(u.nodename, hostname, sizeof(u.nodename));
        strncpy(u.release, osrelease, sizeof(u.release));
        dp = u.version;
        ep = &u.version[sizeof(u.version) - 1];
        for (cp = version; *cp && *cp != '('; cp++)
                ;
        for (cp++; *cp && *cp != ')' && dp < ep; cp++)
                *dp++ = *cp;
        for (; *cp && *cp != '#'; cp++)
                ;
        for (; *cp && *cp != ':' && dp < ep; cp++)
                *dp++ = *cp;
        *dp = '\0';
        strncpy(u.machine, MACHINE, sizeof(u.machine));
        return (copyout((caddr_t)&u, (caddr_t)SCARG(uap, name), sizeof u));
}

int
osf1_sys_gettimeofday(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_gettimeofday_args *uap = v;
	struct sys_gettimeofday_args a;
	struct osf1_timeval otv;
	struct osf1_timezone otz;
	struct timeval tv;
	struct timezone tz;
	int error;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);
	if (SCARG(uap, tp) == NULL)
		SCARG(&a, tp) = NULL;
	else
		SCARG(&a, tp) = stackgap_alloc(&sg, sizeof tv);
	if (SCARG(uap, tzp) == NULL)
		SCARG(&a, tzp) = NULL;
	else
		SCARG(&a, tzp) = stackgap_alloc(&sg, sizeof tz);

	error = sys_gettimeofday(p, &a, retval);

	if (error == 0 && SCARG(uap, tp) != NULL) {
		error = copyin((caddr_t)SCARG(&a, tp),
		    (caddr_t)&tv, sizeof tv);
		if (error == 0) {
			memset(&otv, 0, sizeof otv);
			otv.tv_sec = tv.tv_sec;
			otv.tv_usec = tv.tv_usec;

			error = copyout((caddr_t)&otv,
			    (caddr_t)SCARG(uap, tp), sizeof otv);
		}
	}
	if (error == 0 && SCARG(uap, tzp) != NULL) {
		error = copyin((caddr_t)SCARG(&a, tzp),
		    (caddr_t)&tz, sizeof tz);
		if (error == 0) {
			memset(&otz, 0, sizeof otz);
			otz.tz_minuteswest = tz.tz_minuteswest;
			otz.tz_dsttime = tz.tz_dsttime;

			error = copyout((caddr_t)&otz,
			    (caddr_t)SCARG(uap, tzp), sizeof otz);
		}
	}
	return (error);
}

int
osf1_sys_select(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_select_args *uap = v;
	struct sys_select_args a;
	struct osf1_timeval otv;
	struct timeval tv;
	int error;
	caddr_t sg;

	SCARG(&a, nd) = SCARG(uap, nd);
	SCARG(&a, in) = SCARG(uap, in);
	SCARG(&a, ou) = SCARG(uap, ou);
	SCARG(&a, ex) = SCARG(uap, ex);

	error = 0;
	if (SCARG(uap, tv) == NULL)
		SCARG(&a, tv) = NULL;
	else {
		sg = stackgap_init(p->p_emul);
		SCARG(&a, tv) = stackgap_alloc(&sg, sizeof tv);

		/* get the OSF/1 timeval argument */
		error = copyin((caddr_t)SCARG(uap, tv),
		    (caddr_t)&otv, sizeof otv);
		if (error == 0) {

			/* fill in and copy out the NetBSD timeval */
			memset(&tv, 0, sizeof tv);
			tv.tv_sec = otv.tv_sec;
			tv.tv_usec = otv.tv_usec;

			error = copyout((caddr_t)&tv,
			    (caddr_t)SCARG(&a, tv), sizeof tv);
		}
	}

	if (error == 0)
		error = sys_select(p, &a, retval);

	return (error);
}

int
osf1_sys_utimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_utimes_args *uap = v;
	struct sys_utimes_args a;
	struct osf1_timeval otv;
	struct timeval tv;
	caddr_t sg;
	int error;

	sg = stackgap_init(p->p_emul);

	OSF1_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	SCARG(&a, path) = SCARG(uap, path);

	error = 0;
	if (SCARG(uap, tptr) == NULL)
		SCARG(&a, tptr) = NULL;
	else {
		SCARG(&a, tptr) = stackgap_alloc(&sg, sizeof tv);

		/* get the OSF/1 timeval argument */
		error = copyin((caddr_t)SCARG(uap, tptr),
		    (caddr_t)&otv, sizeof otv);
		if (error == 0) {

			/* fill in and copy out the NetBSD timeval */
			memset(&tv, 0, sizeof tv);
			tv.tv_sec = otv.tv_sec;
			tv.tv_usec = otv.tv_usec;

			error = copyout((caddr_t)&tv,
			    (caddr_t)SCARG(&a, tptr), sizeof tv);
		}
	}

	if (error == 0)
		error = sys_utimes(p, &a, retval);

	return (error);
}

int
osf1_sys_settimeofday(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_settimeofday_args *uap = v;
	struct sys_settimeofday_args a;
	struct osf1_timeval otv;
	struct osf1_timezone otz;
	struct timeval tv;
	struct timezone tz;
	int error;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);
	if (SCARG(uap, tv) == NULL)
		SCARG(&a, tv) = NULL;
	else {
		SCARG(&a, tv) = stackgap_alloc(&sg, sizeof tv);

		/* get the OSF/1 timeval argument */
		error = copyin((caddr_t)SCARG(uap, tv),
		    (caddr_t)&otv, sizeof otv);
		if (error == 0) {

			/* fill in and copy out the NetBSD timeval */
			memset(&tv, 0, sizeof tv);
			tv.tv_sec = otv.tv_sec;
			tv.tv_usec = otv.tv_usec;

			error = copyout((caddr_t)&tv,
			    (caddr_t)SCARG(&a, tv), sizeof tv);
		}
	}

	if (SCARG(uap, tzp) == NULL)
		SCARG(&a, tzp) = NULL;
	else {
		SCARG(&a, tzp) = stackgap_alloc(&sg, sizeof tz);

		/* get the OSF/1 timeval argument */
		error = copyin((caddr_t)SCARG(uap, tzp),
		    (caddr_t)&otz, sizeof otz);
		if (error == 0) {

			/* fill in and copy out the NetBSD timezone */
			memset(&tz, 0, sizeof tz);
			tz.tz_minuteswest = otz.tz_minuteswest;
			tz.tz_dsttime = otz.tz_dsttime;

			error = copyout((caddr_t)&tz,
			    (caddr_t)SCARG(&a, tzp), sizeof tz);
		}
	}

	if (error == 0)
		error = sys_settimeofday(p, &a, retval);

	return (error);
}

int
osf1_sys_set_program_attributes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_set_program_attributes_args *uap = v;
	segsz_t tsize, dsize;

	tsize = btoc(SCARG(uap, tsize));
	dsize = btoc(SCARG(uap, dsize));

	if (dsize > p->p_rlimit[RLIMIT_DATA].rlim_cur)
		return (ENOMEM);
	if (tsize > MAXTSIZ)
		return (ENOMEM);

	p->p_vmspace->vm_taddr = SCARG(uap, taddr);
	p->p_vmspace->vm_tsize = tsize;
	p->p_vmspace->vm_daddr = SCARG(uap, daddr);
	p->p_vmspace->vm_dsize = dsize;

	return (0);
}

int
osf1_sys_access(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_access_args *uap = v;
	struct sys_access_args a;
	unsigned long leftovers;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);
	OSF1_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&a, path) = SCARG(uap, path);

	/* translate flags */
	SCARG(&a, flags) = emul_flags_translate(osf1_access_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	return sys_access(p, &a, retval);
}

int
osf1_sys_setitimer(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_setitimer_args *uap = v;
	struct sys_setitimer_args a;
	struct osf1_itimerval o_itv, o_oitv;
	struct itimerval b_itv, b_oitv;
	caddr_t sg;
	int error;

	switch (SCARG(uap, which)) {
	case OSF1_ITIMER_REAL:
		SCARG(&a, which) = ITIMER_REAL;
		break;

	case OSF1_ITIMER_VIRTUAL:
		SCARG(&a, which) = ITIMER_VIRTUAL;
		break;

	case OSF1_ITIMER_PROF:
		SCARG(&a, which) = ITIMER_PROF;
		break;

	default:
		return (EINVAL);
	}

	sg = stackgap_init(p->p_emul);

	SCARG(&a, itv) = stackgap_alloc(&sg, sizeof b_itv);

	/* get the OSF/1 itimerval argument */
	error = copyin((caddr_t)SCARG(uap, itv), (caddr_t)&o_itv,
	    sizeof o_itv);
	if (error == 0) {

		/* fill in and copy out the NetBSD timeval */
		memset(&b_itv, 0, sizeof b_itv);
		b_itv.it_interval.tv_sec = o_itv.it_interval.tv_sec;
		b_itv.it_interval.tv_usec = o_itv.it_interval.tv_usec;
		b_itv.it_value.tv_sec = o_itv.it_value.tv_sec;
		b_itv.it_value.tv_usec = o_itv.it_value.tv_usec;

		error = copyout((caddr_t)&b_itv,
		    (caddr_t)SCARG(&a, itv), sizeof b_itv);
	}

	if (SCARG(uap, oitv) == NULL)
		SCARG(&a, oitv) = NULL;
	else
		SCARG(&a, oitv) = stackgap_alloc(&sg, sizeof b_oitv);

	if (error == 0)
		error = sys_setitimer(p, &a, retval);

	if (error == 0 && SCARG(uap, oitv) != NULL) {
		/* get the NetBSD itimerval return value */
		error = copyin((caddr_t)SCARG(&a, oitv), (caddr_t)&b_oitv,
		    sizeof b_oitv);
		if (error == 0) {
	
			/* fill in and copy out the NetBSD timeval */
			memset(&o_oitv, 0, sizeof o_oitv);
			o_oitv.it_interval.tv_sec = b_oitv.it_interval.tv_sec;
			o_oitv.it_interval.tv_usec = b_oitv.it_interval.tv_usec;
			o_oitv.it_value.tv_sec = b_oitv.it_value.tv_sec;
			o_oitv.it_value.tv_usec = b_oitv.it_value.tv_usec;
	
			error = copyout((caddr_t)&o_oitv,
			    (caddr_t)SCARG(uap, oitv), sizeof o_oitv);
		}
	}

	return (error);
}

int
osf1_sys_sysinfo(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_sysinfo_args *uap = v;
	const char *string;
	int error;
        extern char ostype[], osrelease[];

	error = 0;
	switch (SCARG(uap, cmd)) {
	case OSF1_SI_SYSNAME:
		string = ostype;
		break;

	case OSF1_SI_HOSTNAME:
		string = hostname;
		break;

	case OSF1_SI_RELEASE:
		string = osrelease;
		break;

	case OSF1_SI_VERSION:
		goto should_handle;

	case OSF1_SI_MACHINE:
		string = MACHINE;
		break;

	case OSF1_SI_ARCHITECTURE:
		string = MACHINE_ARCH;
		break;

	case OSF1_SI_HW_SERIAL:
		string = "666";			/* OSF/1 emulation?  YES! */
		break;

	case OSF1_SI_HW_PROVIDER:
		string = "unknown";
		break;

	case OSF1_SI_SRPC_DOMAIN:
		goto dont_care;

	case OSF1_SI_SET_HOSTNAME:
		goto should_handle;

	case OSF1_SI_SET_SYSNAME:
		goto should_handle;

	case OSF1_SI_SET_SRPC_DOMAIN:
		goto dont_care;

	default:
should_handle:
		printf("osf1_sys_sysinfo(%d, %p, 0x%lx)\n", SCARG(uap, cmd),
		    SCARG(uap, buf), SCARG(uap,len));
dont_care:
		error = EINVAL;
		break;
	};

	if (error == 0)
		error = copyoutstr(string, SCARG(uap, buf), SCARG(uap, len),
		    NULL);

	return (error);
}

int
osf1_sys_wait4(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_wait4_args *uap = v;
	struct sys_wait4_args a;
	struct osf1_rusage osf1_rusage;
	struct rusage netbsd_rusage;
	unsigned long leftovers;
	caddr_t sg;
	int error;

	SCARG(&a, pid) = SCARG(uap, pid);
	SCARG(&a, status) = SCARG(uap, status);

	/* translate options */
	SCARG(&a, options) = emul_flags_translate(osf1_wait_options_xtab,
	    SCARG(uap, options), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	if (SCARG(uap, rusage) == NULL)
		SCARG(&a, rusage) = NULL;
	else {
		sg = stackgap_init(p->p_emul);
		SCARG(&a, rusage) = stackgap_alloc(&sg, sizeof netbsd_rusage);
	}

	error = sys_wait4(p, &a, retval);

	if (error == 0 && SCARG(&a, rusage) != NULL) {
		error = copyin((caddr_t)SCARG(&a, rusage),
		    (caddr_t)&netbsd_rusage, sizeof netbsd_rusage);
		if (error == 0) {
			cvtrusage2osf1(&netbsd_rusage, &osf1_rusage);
			error = copyout((caddr_t)&osf1_rusage,
			    (caddr_t)SCARG(uap, rusage), sizeof osf1_rusage);
		}
	}

	return (error);
}
