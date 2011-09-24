/* Copyright (C) 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* This is somewhat modelled after the file of the same name on SVR4
   systems.  It provides a definition of the core file format for ELF
   used on Linux.  It doesn't have anything to do with the /proc file
   system, even though Linux has one.

   Anyway, the whole purpose of this file is for GDB and GDB only.
   Don't read too much into it.  Don't use it for anything other than
   GDB unless you know what you are doing.  */

#include <features.h>
#include <sys/time.h>
#include <sys/types.h>

/* We define here only the symbols differing from their 64-bit variant.  */
#include <sys/procfs.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
typedef unsigned int uint32_t;
#endif

#undef HAVE_PRPSINFO32_T
#define HAVE_PRPSINFO32_T

#undef HAVE_PRSTATUS32_T
#define HAVE_PRSTATUS32_T

/* These are the 32-bit x86 structures.  */

struct user_fpregs32_struct
{
  int32_t cwd;
  int32_t swd;
  int32_t twd;
  int32_t fip;
  int32_t fcs;
  int32_t foo;
  int32_t fos;
  int32_t st_space [20];
};

struct user_fpxregs32_struct
{
  unsigned short int cwd;
  unsigned short int swd;
  unsigned short int twd;
  unsigned short int fop;
  int32_t fip;
  int32_t fcs;
  int32_t foo;
  int32_t fos;
  int32_t mxcsr;
  int32_t reserved;
  int32_t st_space[32];   /* 8*16 bytes for each FP-reg = 128 bytes */
  int32_t xmm_space[32];  /* 8*16 bytes for each XMM-reg = 128 bytes */
  int32_t padding[56];
};

struct user_regs32_struct
{
  int32_t ebx;
  int32_t ecx;
  int32_t edx;
  int32_t esi;
  int32_t edi;
  int32_t ebp;
  int32_t eax;
  int32_t xds;
  int32_t xes;
  int32_t xfs;
  int32_t xgs;
  int32_t orig_eax;
  int32_t eip;
  int32_t xcs;
  int32_t eflags;
  int32_t esp;
  int32_t xss;
};

struct user32
{
  struct user_regs32_struct	regs;
  int				u_fpvalid;
  struct user_fpregs32_struct	i387;
  uint32_t			u_tsize;
  uint32_t			u_dsize;
  uint32_t			u_ssize;
  uint32_t			start_code;
  uint32_t			start_stack;
  int32_t			signal;
  int				reserved;
  struct user_regs32_struct*	u_ar0;
  struct user_fpregs32_struct*	u_fpstate;
  uint32_t			magic;
  char				u_comm [32];
  int				u_debugreg [8];
};

/* Type for a general-purpose register.  */
typedef unsigned int elf_greg32_t;

/* And the whole bunch of them.  We could have used `struct
   user_regs_struct' directly in the typedef, but tradition says that
   the register set is an array, which does have some peculiar
   semantics, so leave it that way.  */
#define ELF_NGREG32 (sizeof (struct user_regs32_struct) / sizeof(elf_greg32_t))
typedef elf_greg32_t elf_gregset32_t[ELF_NGREG32];

/* Register set for the floating-point registers.  */
typedef struct user_fpregs32_struct elf_fpregset32_t;

/* Register set for the extended floating-point registers.  Includes
   the Pentium III SSE registers in addition to the classic
   floating-point stuff.  */
typedef struct user_fpxregs32_struct elf_fpxregset32_t;


/* Definitions to generate Intel SVR4-like core files.  These mostly
   have the same names as the SVR4 types with "elf_" tacked on the
   front to prevent clashes with Linux definitions, and the typedef
   forms have been avoided.  This is mostly like the SVR4 structure,
   but more Linuxy, with things that Linux does not support and which
   GDB doesn't really use excluded.  */

struct prstatus32_timeval
  {
    int tv_sec;
    int tv_usec;
  };

struct elf_prstatus32
  {
    struct elf_siginfo pr_info;		/* Info associated with signal.  */
    short int pr_cursig;		/* Current signal.  */
    unsigned int pr_sigpend;		/* Set of pending signals.  */
    unsigned int pr_sighold;		/* Set of held signals.  */
    __pid_t pr_pid;
    __pid_t pr_ppid;
    __pid_t pr_pgrp;
    __pid_t pr_sid;
    struct prstatus32_timeval pr_utime;		/* User time.  */
    struct prstatus32_timeval pr_stime;		/* System time.  */
    struct prstatus32_timeval pr_cutime;	/* Cumulative user time.  */
    struct prstatus32_timeval pr_cstime;	/* Cumulative system time.  */
    elf_gregset32_t pr_reg;		/* GP registers.  */
    int pr_fpvalid;			/* True if math copro being used.  */
  };


struct elf_prpsinfo32
  {
    char pr_state;			/* Numeric process state.  */
    char pr_sname;			/* Char for pr_state.  */
    char pr_zomb;			/* Zombie.  */
    char pr_nice;			/* Nice val.  */
    unsigned int pr_flag;		/* Flags.  */
    unsigned short int pr_uid;
    unsigned short int pr_gid;
    int pr_pid, pr_ppid, pr_pgrp, pr_sid;
    /* Lots missing */
    char pr_fname[16];			/* Filename of executable.  */
    char pr_psargs[ELF_PRARGSZ];	/* Initial part of arg list.  */
  };


/* The rest of this file provides the types for emulation of the
   Solaris <proc_service.h> interfaces that should be implemented by
   users of libthread_db.  */

/* Register sets.  Linux has different names.  */
typedef elf_gregset_t prgregset32_t;
typedef elf_fpregset_t prfpregset32_t;

/* Process status and info.  In the end we do provide typedefs for them.  */
typedef struct elf_prstatus32 prstatus32_t;
typedef struct elf_prpsinfo32 prpsinfo32_t;
