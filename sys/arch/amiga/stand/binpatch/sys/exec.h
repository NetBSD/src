/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
 * All rights reserved.
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
 *	from: @(#)exec.h	7.5 (Berkeley) 2/15/91
 *	$Id: exec.h,v 1.2 1993/10/30 23:42:07 mw Exp $
 */

#ifndef	_SYS_EXEC_H_
#define	_SYS_EXEC_H_

#include	<sys/param.h>
#include	<sys/cdefs.h>

/*
 * Header prepended to each a.out file.
 * only manipulate the a_midmag field via the
 * N_SETMAGIC/N_GET{MAGIC,MID,FLAG} macros in a.out.h
 */
struct exec {
	unsigned long	a_midmag;	/* htonl(flags<<26 | mid<<16 | magic) */
	unsigned long	a_text;		/* text segment size */
	unsigned long	a_data;		/* initialized data size */
	unsigned long	a_bss;		/* uninitialized data size */
	unsigned long	a_syms;		/* symbol table size */
	unsigned long	a_entry;	/* entry point */
	unsigned long	a_trsize;	/* text relocation size */
	unsigned long	a_drsize;	/* data relocation size */
};

/* a_magic */
#define	OMAGIC		0407	/* old impure format */
#define	NMAGIC		0410	/* read-only text */
#define	ZMAGIC		0413	/* demand load format */
#define QMAGIC		0314	/* "compact" demand load format -- DEPRICATE */

/*
 * a_mid - keep sorted in numerical order for sanity's sake
 * ensure that: 0 < mid < 0x3ff
 */
#define	MID_ZERO	0	/* unknown - implementation dependent */
#define	MID_SUN010	1	/* sun 68010/68020 binary */
#define	MID_SUN020	2	/* sun 68020-only binary */
#define MID_AMIGA030	64	/* amiga (68020/68030) 6888[12] fpu */
#define MID_AMIGA040	65	/* amiga 68040 (w/ 68040 fpu only) */
#define MID_PC386	100	/* 386 PC binary. (so quoth BFD) */
#define	MID_HP200	200	/* hp200 (68010) BSD binary */
#define MID_I386	134	/* i386 BSD binary */
#define	MID_HP300	300	/* hp300 (68020+68881) BSD binary */
#define	MID_HPUX	0x20C	/* hp200/300 HP-UX binary */
#define	MID_HPUX800     0x20B   /* hp800 HP-UX binary */

/*
 * Magic number/exec function table... (execution class loader)
 */
struct execsw {
	int	magic;		/* magic number this loader sees*/
	int	m_size;		/* size of magic number (significant bytes)*/
	int	(*load)();	/* loader function*/
};

extern struct execsw execsw[];
extern int	nexecs;

/*
 * i (cgd) give up; it's damned impossible to find out where a process's
 * args are.  4.4 actually puts pointers in the process address space,
 * above the stack.  this is reasonable, and also allows the process to
 * set its ps-visible arguments.
 *
 * PS_STRINGS defines the location of a process's ps_strings structure.
 */

struct ps_strings {
  char *ps_argvstr;       /* the argv strings */
  int  ps_nargvstr;       /* number of argv strings */
  char *ps_envstr;        /* the environment strings */
  int  ps_nenvstr;        /* number of environment strings */
};
#define PS_STRINGS \
        ((struct ps_strings *)(USRSTACK - sizeof(struct ps_strings)))

/*
 * the following structures allow exec to put together processes
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

struct exec_package {
  struct exec *ep_execp;      /* file's exec header */
  struct exec_vmcmd *ep_vcp;  /* exec_vmcmds used to build the vmspace */
  struct vnode *ep_vp;        /* executable's vnode */
  struct vattr *ep_vap;       /* executable's attributes */
  unsigned long ep_taddr;     /* process's text address */
  unsigned long ep_tsize;     /* size of process's text */
  unsigned long ep_daddr;     /* process's data(+bss) address */
  unsigned long ep_dsize;     /* size of process's data(+bss) */
  unsigned long ep_maxsaddr;  /* process's max stack address ("top") */
  unsigned long ep_minsaddr;  /* process's min stack address ("bottom") */
  unsigned long ep_ssize;     /* size of process's stack */
  unsigned long ep_entry;     /* process's entry point */
#ifdef notdef
  char   *ep_argbuf;          /* argument buffer. generally same as argv */
  int    ep_argc;             /* count of process arguments */
  char   *ep_argv;            /* process arguments, null seperated */
  int    ep_envc;             /* count of process environment strings */
  char   *ep_env;             /* process enviornment strings, null seperated */
#endif
};

struct proc;

struct exec_vmcmd {
  struct exec_vmcmd *ev_next;   /* next command on the chain */
                                /* procedure to run */
  int	            (*ev_proc) __P((struct proc *p, struct exec_vmcmd *cmd));
  unsigned long     ev_len;     /* length of the segment to map */
  unsigned long     ev_addr;    /* address in the vmspace to place it at */
  struct vnode      *ev_vp;     /* vnode pointer for the file w/the data */
  unsigned long     ev_offset;  /* offset in the file for the data */
  unsigned          ev_prot;    /* protections for segment */
};

#ifdef KERNEL
struct exec_vmcmd *new_vmcmd __P(( \
     int (*proc) __P((struct proc *p, struct exec_vmcmd *)), \
     unsigned long len, \
     unsigned long addr, \
     struct vnode *vp, \
     unsigned long offset, \
     unsigned long prot));
void kill_vmcmd __P((struct exec_vmcmd **));
int exec_makecmds __P((struct proc *, struct exec_package *));
int exec_runcmds __P((struct proc *, struct exec_package *));
int exec_prep_qmagic __P((struct proc *, struct exec_package *));
int exec_prep_zmagic __P((struct proc *, struct exec_package *));
int vmcmd_map_pagedvn __P((struct proc *, struct exec_vmcmd *));
int vmcmd_map_zero __P((struct proc *, struct exec_vmcmd *));
#endif

#endif /* !_SYS_EXEC_H_ */

