/*	$NetBSD: exec.h,v 1.93 2003/02/20 22:16:07 atatat Exp $	*/

/*-
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1993 Theo de Raadt
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)exec.h	8.4 (Berkeley) 2/19/95
 */

#ifndef _SYS_EXEC_H_
#define _SYS_EXEC_H_

/*
 * The following structure is found at the top of the user stack of each
 * user process. The ps program uses it to locate argv and environment
 * strings. Programs that wish ps to display other information may modify
 * it; normally ps_argvstr points to argv[0], and ps_nargvstr is the same
 * as the program's argc. The fields ps_envstr and ps_nenvstr are the
 * equivalent for the environment.
 */
struct ps_strings {
	char	**ps_argvstr;	/* first of 0 or more argument strings */
	int	ps_nargvstr;	/* the number of argument strings */
	char	**ps_envstr;	/* first of 0 or more environment strings */
	int	ps_nenvstr;	/* the number of environment strings */
};

/*
 * Below the ps_strings and sigtramp, we may require a gap on the stack
 * (used to copyin/copyout various emulation data structures).
 */
#define	STACKGAPLEN	512	/* plenty enough for now */

/*
 * the following structures allow execve() to put together processes
 * in a more extensible and cleaner way.
 *
 * the exec_package struct defines an executable being execve()'d.
 * it contains the header, the vmspace-building commands, the vnode
 * information, and the arguments associated with the newly-execve'd
 * process.
 *
 * the exec_vmcmd struct defines a command description to be used
 * in creating the new process's vmspace.
 */

struct lwp;
struct proc;
struct exec_package;
struct vnode;
struct ucred;

typedef int (*exec_makecmds_fcn) __P((struct proc *, struct exec_package *));

struct execsw {
	u_int	es_hdrsz;		/* size of header for this format */
	exec_makecmds_fcn es_check;	/* function to check exec format */
	union {				/* probe function */
		int (*elf_probe_func) __P((struct proc *,
			struct exec_package *, void *, char *, vaddr_t *));
		int (*ecoff_probe_func) __P((struct proc *,
			struct exec_package *));
		int (*mach_probe_func) __P((char **));
	} u;
	const struct  emul *es_emul;	/* os emulation */
	int	es_prio;		/* entry priority */
	int	es_arglen;		/* Extra argument size in words */
					/* Copy arguments on the new stack */
	int	(*es_copyargs) __P((struct proc *, struct exec_package *,
			struct ps_strings *, char **, void *));
					/* Set registers before execution */
	void	(*es_setregs) __P((struct lwp *, struct exec_package *,
				   u_long));
					/* Dump core */
	int	(*es_coredump) __P((struct lwp *, struct vnode *,
				    struct ucred *));
};

#define EXECSW_PRIO_ANY		0x000	/* default, no preference */
#define EXECSW_PRIO_FIRST	0x001	/* this should be among first */
#define EXECSW_PRIO_LAST	0x002	/* this should be among last */

/* exec vmspace-creation command set; see below */
struct exec_vmcmd_set {
	u_int	evs_cnt;
	u_int	evs_used;
	struct	exec_vmcmd *evs_cmds;
};

#define	EXEC_DEFAULT_VMCMD_SETSIZE	9	/* # of cmds in set to start */

struct exec_package {
	const char *ep_name;		/* file's name */
	void	*ep_hdr;		/* file's exec header */
	u_int	ep_hdrlen;		/* length of ep_hdr */
	u_int	ep_hdrvalid;		/* bytes of ep_hdr that are valid */
	struct nameidata *ep_ndp;	/* namei data pointer for lookups */
	struct	exec_vmcmd_set ep_vmcmds;  /* vmcmds used to build vmspace */
	struct	vnode *ep_vp;		/* executable's vnode */
	struct	vattr *ep_vap;		/* executable's attributes */
	u_long	ep_taddr;		/* process's text address */
	u_long	ep_tsize;		/* size of process's text */
	u_long	ep_daddr;		/* process's data(+bss) address */
	u_long	ep_dsize;		/* size of process's data(+bss) */
	u_long	ep_maxsaddr;		/* proc's max stack addr ("top") */
	u_long	ep_minsaddr;		/* proc's min stack addr ("bottom") */
	u_long	ep_ssize;		/* size of process's stack */
	u_long	ep_entry;		/* process's entry point */
	vaddr_t	ep_vm_minaddr;		/* bottom of process address space */
	vaddr_t	ep_vm_maxaddr;		/* top of process address space */
	u_int	ep_flags;		/* flags; see below. */
	char	**ep_fa;		/* a fake args vector for scripts */
	int	ep_fd;			/* a file descriptor we're holding */
	void	*ep_emul_arg;		/* emulation argument */
	const struct	execsw *ep_es;	/* appropriate execsw entry */
	const struct	execsw *ep_esch;/* checked execsw entry */
};
#define	EXEC_INDIR	0x0001		/* script handling already done */
#define	EXEC_HASFD	0x0002		/* holding a shell script */
#define	EXEC_HASARGL	0x0004		/* has fake args vector */
#define	EXEC_SKIPARG	0x0008		/* don't copy user-supplied argv[0] */
#define	EXEC_DESTR	0x0010		/* destructive ops performed */
#define	EXEC_32		0x0020		/* 32-bit binary emulation */
#define	EXEC_HASES	0x0040		/* don't update exec switch pointer */

struct exec_vmcmd {
	int	(*ev_proc) __P((struct proc *p, struct exec_vmcmd *cmd));
				/* procedure to run for region of vmspace */
	u_long	ev_len;		/* length of the segment to map */
	u_long	ev_addr;	/* address in the vmspace to place it at */
	struct	vnode *ev_vp;	/* vnode pointer for the file w/the data */
	u_long	ev_offset;	/* offset in the file for the data */
	u_int	ev_prot;	/* protections for segment */
	int	ev_flags;
#define	VMCMD_RELATIVE	0x0001	/* ev_addr is relative to base entry */
#define	VMCMD_BASE	0x0002	/* marks a base entry */
#define	VMCMD_FIXED	0x0004	/* entry must be mapped at ev_addr */
#define	VMCMD_TOPDOWN	0x0008	/* shift back base for relative mappings */
};

#ifdef _KERNEL
#include <sys/mallocvar.h>

MALLOC_DECLARE(M_EXEC);

/*
 * funtions used either by execve() or the various cpu-dependent execve()
 * hooks.
 */
void	kill_vmcmd		__P((struct exec_vmcmd **));
int	exec_makecmds		__P((struct proc *, struct exec_package *));
int	exec_runcmds		__P((struct proc *, struct exec_package *));
void	vmcmdset_extend		__P((struct exec_vmcmd_set *));
void	kill_vmcmds		__P((struct exec_vmcmd_set *evsp));
int	vmcmd_map_pagedvn	__P((struct proc *, struct exec_vmcmd *));
int	vmcmd_map_readvn	__P((struct proc *, struct exec_vmcmd *));
int	vmcmd_readvn		__P((struct proc *, struct exec_vmcmd *));
int	vmcmd_map_zero		__P((struct proc *, struct exec_vmcmd *));
int	copyargs		__P((struct proc *, struct exec_package *,
    struct ps_strings *, char **, void *));
void	setregs			__P((struct lwp *, struct exec_package *,
				     u_long));
#ifdef VERIFIED_EXEC
int	check_veriexec		__P((struct proc *, struct vnode *,
				     struct exec_package *, int));
int	check_exec		__P((struct proc *, struct exec_package *,
				     int));
#else
int	check_exec		__P((struct proc *, struct exec_package *));
#endif
int	exec_init		__P((int));
int	exec_read_from		__P((struct proc *, struct vnode *, u_long off,
    void *, size_t));

#ifdef LKM
int	emul_register		__P((const struct emul *, int));
int	emul_unregister		__P((const char *));
const struct emul *emul_search	__P((const char *));

int	exec_add		__P((struct execsw *, const char *));
int	exec_remove		__P((const struct execsw *));
#endif /* LKM */

#ifdef DEBUG
void	new_vmcmd __P((struct exec_vmcmd_set *evsp,
		    int (*proc) __P((struct proc *p, struct exec_vmcmd *)),
		    u_long len, u_long addr, struct vnode *vp, u_long offset,
		    u_int prot, int flags));
#define	NEW_VMCMD(evsp,proc,len,addr,vp,offset,prot) \
	new_vmcmd(evsp,proc,len,addr,vp,offset,prot,0)
#define	NEW_VMCMD2(evsp,proc,len,addr,vp,offset,prot,flags) \
	new_vmcmd(evsp,proc,len,addr,vp,offset,prot,flags)
#else	/* DEBUG */
#define	NEW_VMCMD(evsp,proc,len,addr,vp,offset,prot) \
	NEW_VMCMD2(evsp,proc,len,addr,vp,offset,prot,0)
#define	NEW_VMCMD2(evsp,proc,len,addr,vp,offset,prot,flags) do { \
	struct exec_vmcmd *vcp; \
	if ((evsp)->evs_used >= (evsp)->evs_cnt) \
		vmcmdset_extend(evsp); \
	vcp = &(evsp)->evs_cmds[(evsp)->evs_used++]; \
	vcp->ev_proc = (proc); \
	vcp->ev_len = (len); \
	vcp->ev_addr = (addr); \
	if ((vcp->ev_vp = (vp)) != NULLVP) \
		VREF(vp); \
	vcp->ev_offset = (offset); \
	vcp->ev_prot = (prot); \
	vcp->ev_flags = (flags); \
	if (((flags) & (VMCMD_TOPDOWN|VMCMD_RELATIVE)) == \
	    (VMCMD_TOPDOWN|VMCMD_RELATIVE)) { \
		int i = (evsp)->evs_used - 2; \
		while (i >= 0) { \
			vcp = &(evsp)->evs_cmds[i--]; \
			if (vcp->ev_flags & VMCMD_BASE) { \
				if ((vcp->ev_flags & \
				    (VMCMD_TOPDOWN|VMCMD_FIXED)) == \
				    (VMCMD_TOPDOWN)) \
					vcp->ev_addr -= round_page(len); \
				break; \
			} \
		} \
	} \
} while (/* CONSTCOND */ 0)
#endif /* DEBUG */

#endif /* _KERNEL */

#include <sys/exec_aout.h>

#endif /* !_SYS_EXEC_H_ */
