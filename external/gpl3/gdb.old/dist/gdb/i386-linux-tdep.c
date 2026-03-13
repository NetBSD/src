/* Target-dependent code for GNU/Linux i386.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "extract-store-integer.h"
#include "gdbcore.h"
#include "frame.h"
#include "value.h"
#include "regcache.h"
#include "regset.h"
#include "inferior.h"
#include "osabi.h"
#include "reggroups.h"
#include "dwarf2/frame.h"
#include "i386-tdep.h"
#include "i386-linux-tdep.h"
#include "linux-tdep.h"
#include "utils.h"
#include "glibc-tdep.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "arch-utils.h"
#include "xml-syscall.h"
#include "infrun.h"

#include "i387-tdep.h"
#include "gdbsupport/x86-xstate.h"
#include "arch/i386-linux-tdesc.h"
#include "arch/x86-linux-tdesc.h"

/* The syscall's XML filename for i386.  */
#define XML_SYSCALL_FILENAME_I386 "syscalls/i386-linux.xml"

#include "record-full.h"
#include "linux-record.h"

#include "arch/i386.h"
#include "target-descriptions.h"

/* Return non-zero, when the register is in the corresponding register
   group.  Put the LINUX_ORIG_EAX register in the system group.  */
static int
i386_linux_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				const struct reggroup *group)
{
  if (regnum == I386_LINUX_ORIG_EAX_REGNUM)
    return (group == system_reggroup
	    || group == save_reggroup
	    || group == restore_reggroup);
  return i386_register_reggroup_p (gdbarch, regnum, group);
}


/* Recognizing signal handler frames.  */

/* GNU/Linux has two flavors of signals.  Normal signal handlers, and
   "realtime" (RT) signals.  The RT signals can provide additional
   information to the signal handler if the SA_SIGINFO flag is set
   when establishing a signal handler using `sigaction'.  It is not
   unlikely that future versions of GNU/Linux will support SA_SIGINFO
   for normal signals too.  */

/* When the i386 Linux kernel calls a signal handler and the
   SA_RESTORER flag isn't set, the return address points to a bit of
   code on the stack.  This function returns whether the PC appears to
   be within this bit of code.

   The instruction sequence for normal signals is
       pop    %eax
       mov    $0x77, %eax
       int    $0x80
   or 0x58 0xb8 0x77 0x00 0x00 0x00 0xcd 0x80.

   Checking for the code sequence should be somewhat reliable, because
   the effect is to call the system call sigreturn.  This is unlikely
   to occur anywhere other than in a signal trampoline.

   It kind of sucks that we have to read memory from the process in
   order to identify a signal trampoline, but there doesn't seem to be
   any other way.  Therefore we only do the memory reads if no
   function name could be identified, which should be the case since
   the code is on the stack.

   Detection of signal trampolines for handlers that set the
   SA_RESTORER flag is in general not possible.  Unfortunately this is
   what the GNU C Library has been doing for quite some time now.
   However, as of version 2.1.2, the GNU C Library uses signal
   trampolines (named __restore and __restore_rt) that are identical
   to the ones used by the kernel.  Therefore, these trampolines are
   supported too.  */

#define LINUX_SIGTRAMP_INSN0	0x58	/* pop %eax */
#define LINUX_SIGTRAMP_OFFSET0	0
#define LINUX_SIGTRAMP_INSN1	0xb8	/* mov $NNNN, %eax */
#define LINUX_SIGTRAMP_OFFSET1	1
#define LINUX_SIGTRAMP_INSN2	0xcd	/* int */
#define LINUX_SIGTRAMP_OFFSET2	6

static const gdb_byte linux_sigtramp_code[] =
{
  LINUX_SIGTRAMP_INSN0,					/* pop %eax */
  LINUX_SIGTRAMP_INSN1, 0x77, 0x00, 0x00, 0x00,		/* mov $0x77, %eax */
  LINUX_SIGTRAMP_INSN2, 0x80				/* int $0x80 */
};

#define LINUX_SIGTRAMP_LEN (sizeof linux_sigtramp_code)

/* If THIS_FRAME is a sigtramp routine, return the address of the
   start of the routine.  Otherwise, return 0.  */

static CORE_ADDR
i386_linux_sigtramp_start (const frame_info_ptr &this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[LINUX_SIGTRAMP_LEN];

  /* We only recognize a signal trampoline if PC is at the start of
     one of the three instructions.  We optimize for finding the PC at
     the start, as will be the case when the trampoline is not the
     first frame on the stack.  We assume that in the case where the
     PC is not at the start of the instruction sequence, there will be
     a few trailing readable bytes on the stack.  */

  if (!safe_frame_unwind_memory (this_frame, pc, buf))
    return 0;

  if (buf[0] != LINUX_SIGTRAMP_INSN0)
    {
      int adjust;

      switch (buf[0])
	{
	case LINUX_SIGTRAMP_INSN1:
	  adjust = LINUX_SIGTRAMP_OFFSET1;
	  break;
	case LINUX_SIGTRAMP_INSN2:
	  adjust = LINUX_SIGTRAMP_OFFSET2;
	  break;
	default:
	  return 0;
	}

      pc -= adjust;

      if (!safe_frame_unwind_memory (this_frame, pc, buf))
	return 0;
    }

  if (memcmp (buf, linux_sigtramp_code, LINUX_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}

/* This function does the same for RT signals.  Here the instruction
   sequence is
       mov    $0xad, %eax
       int    $0x80
   or 0xb8 0xad 0x00 0x00 0x00 0xcd 0x80.

   The effect is to call the system call rt_sigreturn.  */

#define LINUX_RT_SIGTRAMP_INSN0		0xb8 /* mov $NNNN, %eax */
#define LINUX_RT_SIGTRAMP_OFFSET0	0
#define LINUX_RT_SIGTRAMP_INSN1		0xcd /* int */
#define LINUX_RT_SIGTRAMP_OFFSET1	5

static const gdb_byte linux_rt_sigtramp_code[] =
{
  LINUX_RT_SIGTRAMP_INSN0, 0xad, 0x00, 0x00, 0x00,	/* mov $0xad, %eax */
  LINUX_RT_SIGTRAMP_INSN1, 0x80				/* int $0x80 */
};

#define LINUX_RT_SIGTRAMP_LEN (sizeof linux_rt_sigtramp_code)

/* If THIS_FRAME is an RT sigtramp routine, return the address of the
   start of the routine.  Otherwise, return 0.  */

static CORE_ADDR
i386_linux_rt_sigtramp_start (const frame_info_ptr &this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[LINUX_RT_SIGTRAMP_LEN];

  /* We only recognize a signal trampoline if PC is at the start of
     one of the two instructions.  We optimize for finding the PC at
     the start, as will be the case when the trampoline is not the
     first frame on the stack.  We assume that in the case where the
     PC is not at the start of the instruction sequence, there will be
     a few trailing readable bytes on the stack.  */

  if (!safe_frame_unwind_memory (this_frame, pc, buf))
    return 0;

  if (buf[0] != LINUX_RT_SIGTRAMP_INSN0)
    {
      if (buf[0] != LINUX_RT_SIGTRAMP_INSN1)
	return 0;

      pc -= LINUX_RT_SIGTRAMP_OFFSET1;

      if (!safe_frame_unwind_memory (this_frame, pc,
				     buf))
	return 0;
    }

  if (memcmp (buf, linux_rt_sigtramp_code, LINUX_RT_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}

/* Return whether THIS_FRAME corresponds to a GNU/Linux sigtramp
   routine.  */

static int
i386_linux_sigtramp_p (const frame_info_ptr &this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);

  /* If we have NAME, we can optimize the search.  The trampolines are
     named __restore and __restore_rt.  However, they aren't dynamically
     exported from the shared C library, so the trampoline may appear to
     be part of the preceding function.  This should always be sigaction,
     __sigaction, or __libc_sigaction (all aliases to the same function).  */
  if (name == NULL || strstr (name, "sigaction") != NULL)
    return (i386_linux_sigtramp_start (this_frame) != 0
	    || i386_linux_rt_sigtramp_start (this_frame) != 0);

  return (strcmp ("__restore", name) == 0
	  || strcmp ("__restore_rt", name) == 0);
}

/* Return one if the PC of THIS_FRAME is in a signal trampoline which
   may have DWARF-2 CFI.  */

static int
i386_linux_dwarf_signal_frame_p (struct gdbarch *gdbarch,
				 const frame_info_ptr &this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);

  /* If a vsyscall DSO is in use, the signal trampolines may have these
     names.  */
  if (name && (strcmp (name, "__kernel_sigreturn") == 0
	       || strcmp (name, "__kernel_rt_sigreturn") == 0))
    return 1;

  return 0;
}

/* Offset to struct sigcontext in ucontext, from <asm/ucontext.h>.  */
#define I386_LINUX_UCONTEXT_SIGCONTEXT_OFFSET 20

/* Assuming THIS_FRAME is a GNU/Linux sigtramp routine, return the
   address of the associated sigcontext structure.  */

static CORE_ADDR
i386_linux_sigcontext_addr (const frame_info_ptr &this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR pc;
  CORE_ADDR sp;
  gdb_byte buf[4];

  get_frame_register (this_frame, I386_ESP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 4, byte_order);

  pc = i386_linux_sigtramp_start (this_frame);
  if (pc)
    {
      /* The sigcontext structure lives on the stack, right after
	 the signum argument.  We determine the address of the
	 sigcontext structure by looking at the frame's stack
	 pointer.  Keep in mind that the first instruction of the
	 sigtramp code is "pop %eax".  If the PC is after this
	 instruction, adjust the returned value accordingly.  */
      if (pc == get_frame_pc (this_frame))
	return sp + 4;
      return sp;
    }

  pc = i386_linux_rt_sigtramp_start (this_frame);
  if (pc)
    {
      CORE_ADDR ucontext_addr;

      /* The sigcontext structure is part of the user context.  A
	 pointer to the user context is passed as the third argument
	 to the signal handler.  */
      read_memory (sp + 8, buf, 4);
      ucontext_addr = extract_unsigned_integer (buf, 4, byte_order);
      return ucontext_addr + I386_LINUX_UCONTEXT_SIGCONTEXT_OFFSET;
    }

  error (_("Couldn't recognize signal trampoline."));
  return 0;
}

/* Set the program counter for process PTID to PC.  */

static void
i386_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  regcache_cooked_write_unsigned (regcache, I386_EIP_REGNUM, pc);

  /* We must be careful with modifying the program counter.  If we
     just interrupted a system call, the kernel might try to restart
     it when we resume the inferior.  On restarting the system call,
     the kernel will try backing up the program counter even though it
     no longer points at the system call.  This typically results in a
     SIGSEGV or SIGILL.  We can prevent this by writing `-1' in the
     "orig_eax" pseudo-register.

     Note that "orig_eax" is saved when setting up a dummy call frame.
     This means that it is properly restored when that frame is
     popped, and that the interrupted system call will be restarted
     when we resume the inferior on return from a function call from
     within GDB.  In all other cases the system call will not be
     restarted.  */
  regcache_cooked_write_unsigned (regcache, I386_LINUX_ORIG_EAX_REGNUM, -1);
}

/* Record all registers but IP register for process-record.  */

static int
i386_all_but_ip_registers_record (struct regcache *regcache)
{
  if (record_full_arch_list_add_reg (regcache, I386_EAX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_ECX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EDX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EBX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_ESP_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EBP_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_ESI_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EDI_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EFLAGS_REGNUM))
    return -1;

  return 0;
}

enum i386_syscall
{
#define SYSCALL(NUMBER,NAME) \
  i386_sys_ ## NAME = NUMBER,

#include "gdb/i386-syscalls.def"

#undef SYSCALL
};

/* i386_canonicalize_syscall maps from the native i386 Linux set
   of syscall ids into a canonical set of syscall ids used by
   process record (a mostly trivial mapping, since the canonical
   set was originally taken from the i386 set).  */

static enum gdb_syscall
i386_canonicalize_syscall (int syscall)
{
  switch ((enum i386_syscall) syscall)
    {
#define SYSCALL_MAP(SYSCALL)			\
      case i386_sys_ ## SYSCALL:		\
	return gdb_sys_ ## SYSCALL

#define SYSCALL_MAP_RENAME(SYSCALL,GDB_SYSCALL)	\
      case i386_sys_##SYSCALL:			\
	return GDB_SYSCALL;

#define UNSUPPORTED_SYSCALL_MAP(SYSCALL)	\
      case i386_sys_ ## SYSCALL:		\
	return gdb_sys_no_syscall;

      SYSCALL_MAP (restart_syscall);
      SYSCALL_MAP (exit);
      SYSCALL_MAP (fork);
      SYSCALL_MAP (read);
      SYSCALL_MAP (write);
      SYSCALL_MAP (open);
      SYSCALL_MAP (close);
      SYSCALL_MAP (waitpid);
      SYSCALL_MAP (creat);
      SYSCALL_MAP (link);
      SYSCALL_MAP (unlink);
      SYSCALL_MAP (execve);
      SYSCALL_MAP (chdir);
      SYSCALL_MAP (time);
      SYSCALL_MAP (mknod);
      SYSCALL_MAP (chmod);
      SYSCALL_MAP_RENAME (lchown, gdb_sys_lchown16);
      SYSCALL_MAP_RENAME (break, gdb_sys_ni_syscall17);
      SYSCALL_MAP_RENAME (oldstat, gdb_sys_stat);
      SYSCALL_MAP (lseek);
      SYSCALL_MAP (getpid);
      SYSCALL_MAP (mount);
      SYSCALL_MAP_RENAME (umount, gdb_sys_oldumount);
      SYSCALL_MAP_RENAME (setuid, gdb_sys_setuid16);
      SYSCALL_MAP_RENAME (getuid, gdb_sys_getuid16);
      SYSCALL_MAP (stime);
      SYSCALL_MAP (ptrace);
      SYSCALL_MAP (alarm);
      SYSCALL_MAP_RENAME (oldfstat, gdb_sys_fstat);
      SYSCALL_MAP (pause);
      SYSCALL_MAP (utime);
      SYSCALL_MAP_RENAME (stty, gdb_sys_ni_syscall31);
      SYSCALL_MAP_RENAME (gtty, gdb_sys_ni_syscall32);
      SYSCALL_MAP (access);
      SYSCALL_MAP (nice);
      SYSCALL_MAP_RENAME (ftime, gdb_sys_ni_syscall35);
      SYSCALL_MAP (sync);
      SYSCALL_MAP (kill);
      SYSCALL_MAP (rename);
      SYSCALL_MAP (mkdir);
      SYSCALL_MAP (rmdir);
      SYSCALL_MAP (dup);
      SYSCALL_MAP (pipe);
      SYSCALL_MAP (times);
      SYSCALL_MAP_RENAME (prof, gdb_sys_ni_syscall44);
      SYSCALL_MAP (brk);
      SYSCALL_MAP_RENAME (setgid, gdb_sys_setgid16);
      SYSCALL_MAP_RENAME (getgid, gdb_sys_getgid16);
      SYSCALL_MAP (signal);
      SYSCALL_MAP_RENAME (geteuid, gdb_sys_geteuid16);
      SYSCALL_MAP_RENAME (getegid, gdb_sys_getegid16);
      SYSCALL_MAP (acct);
      SYSCALL_MAP_RENAME (umount2, gdb_sys_umount);
      SYSCALL_MAP_RENAME (lock, gdb_sys_ni_syscall53);
      SYSCALL_MAP (ioctl);
      SYSCALL_MAP (fcntl);
      SYSCALL_MAP_RENAME (mpx, gdb_sys_ni_syscall56);
      SYSCALL_MAP (setpgid);
      SYSCALL_MAP_RENAME (ulimit, gdb_sys_ni_syscall58);
      SYSCALL_MAP_RENAME (oldolduname, gdb_sys_olduname);
      SYSCALL_MAP (umask);
      SYSCALL_MAP (chroot);
      SYSCALL_MAP (ustat);
      SYSCALL_MAP (dup2);
      SYSCALL_MAP (getppid);
      SYSCALL_MAP (getpgrp);
      SYSCALL_MAP (setsid);
      SYSCALL_MAP (sigaction);
      SYSCALL_MAP (sgetmask);
      SYSCALL_MAP (ssetmask);
      SYSCALL_MAP_RENAME (setreuid, gdb_sys_setreuid16);
      SYSCALL_MAP_RENAME (setregid, gdb_sys_setregid16);
      SYSCALL_MAP (sigsuspend);
      SYSCALL_MAP (sigpending);
      SYSCALL_MAP (sethostname);
      SYSCALL_MAP (setrlimit);
      SYSCALL_MAP_RENAME (getrlimit, gdb_sys_old_getrlimit);
      SYSCALL_MAP (getrusage);
      SYSCALL_MAP (gettimeofday);
      SYSCALL_MAP (settimeofday);
      SYSCALL_MAP_RENAME (getgroups, gdb_sys_getgroups16);
      SYSCALL_MAP_RENAME (setgroups, gdb_sys_setgroups16);
      SYSCALL_MAP_RENAME (select, gdb_old_select);
      SYSCALL_MAP (symlink);
      SYSCALL_MAP_RENAME (oldlstat, gdb_sys_lstat);
      SYSCALL_MAP (readlink);
      SYSCALL_MAP (uselib);
      SYSCALL_MAP (swapon);
      SYSCALL_MAP (reboot);
      SYSCALL_MAP_RENAME (readdir, gdb_old_readdir);
      SYSCALL_MAP_RENAME (mmap, gdb_old_mmap);
      SYSCALL_MAP (munmap);
      SYSCALL_MAP (truncate);
      SYSCALL_MAP (ftruncate);
      SYSCALL_MAP (fchmod);
      SYSCALL_MAP_RENAME (fchown, gdb_sys_fchown16);
      SYSCALL_MAP (getpriority);
      SYSCALL_MAP (setpriority);
      SYSCALL_MAP_RENAME (profil, gdb_sys_ni_syscall98);
      SYSCALL_MAP (statfs);
      SYSCALL_MAP (fstatfs);
      SYSCALL_MAP (ioperm);
      SYSCALL_MAP (socketcall);
      SYSCALL_MAP (syslog);
      SYSCALL_MAP (setitimer);
      SYSCALL_MAP (getitimer);
      SYSCALL_MAP_RENAME (stat, gdb_sys_newstat);
      SYSCALL_MAP_RENAME (lstat, gdb_sys_newlstat);
      SYSCALL_MAP_RENAME (fstat, gdb_sys_newfstat);
      SYSCALL_MAP_RENAME (olduname, gdb_sys_uname);
      SYSCALL_MAP (iopl);
      SYSCALL_MAP (vhangup);
      SYSCALL_MAP_RENAME (idle, gdb_sys_ni_syscall112);
      SYSCALL_MAP (vm86old);
      SYSCALL_MAP (wait4);
      SYSCALL_MAP (swapoff);
      SYSCALL_MAP (sysinfo);
      SYSCALL_MAP (ipc);
      SYSCALL_MAP (fsync);
      SYSCALL_MAP (sigreturn);
      SYSCALL_MAP (clone);
      SYSCALL_MAP (setdomainname);
      SYSCALL_MAP_RENAME (uname, gdb_sys_newuname);
      SYSCALL_MAP (modify_ldt);
      SYSCALL_MAP (adjtimex);
      SYSCALL_MAP (mprotect);
      SYSCALL_MAP (sigprocmask);
      SYSCALL_MAP_RENAME (create_module, gdb_sys_ni_syscall127);
      SYSCALL_MAP (init_module);
      SYSCALL_MAP (delete_module);
      SYSCALL_MAP_RENAME (get_kernel_syms, gdb_sys_ni_syscall130);
      SYSCALL_MAP (quotactl);
      SYSCALL_MAP (getpgid);
      SYSCALL_MAP (fchdir);
      SYSCALL_MAP (bdflush);
      SYSCALL_MAP (sysfs);
      SYSCALL_MAP (personality);
      SYSCALL_MAP_RENAME (afs_syscall, gdb_sys_ni_syscall137);
      SYSCALL_MAP_RENAME (setfsuid, gdb_sys_setfsuid16);
      SYSCALL_MAP_RENAME (setfsgid, gdb_sys_setfsgid16);
      SYSCALL_MAP_RENAME (_llseek, gdb_sys_llseek);
      SYSCALL_MAP (getdents);
      SYSCALL_MAP_RENAME (_newselect, gdb_sys_select);
      SYSCALL_MAP (flock);
      SYSCALL_MAP (msync);
      SYSCALL_MAP (readv);
      SYSCALL_MAP (writev);
      SYSCALL_MAP (getsid);
      SYSCALL_MAP (fdatasync);
      SYSCALL_MAP_RENAME (_sysctl, gdb_sys_sysctl);
      SYSCALL_MAP (mlock);
      SYSCALL_MAP (munlock);
      SYSCALL_MAP (mlockall);
      SYSCALL_MAP (munlockall);
      SYSCALL_MAP (sched_setparam);
      SYSCALL_MAP (sched_getparam);
      SYSCALL_MAP (sched_setscheduler);
      SYSCALL_MAP (sched_getscheduler);
      SYSCALL_MAP (sched_yield);
      SYSCALL_MAP (sched_get_priority_max);
      SYSCALL_MAP (sched_get_priority_min);
      SYSCALL_MAP (sched_rr_get_interval);
      SYSCALL_MAP (nanosleep);
      SYSCALL_MAP (mremap);
      SYSCALL_MAP_RENAME (setresuid, gdb_sys_setresuid16);
      SYSCALL_MAP_RENAME (getresuid, gdb_sys_getresuid16);
      SYSCALL_MAP (vm86);
      SYSCALL_MAP_RENAME (query_module, gdb_sys_ni_syscall167);
      SYSCALL_MAP (poll);
      SYSCALL_MAP (nfsservctl);
      SYSCALL_MAP_RENAME (setresgid, gdb_sys_setresgid16);
      SYSCALL_MAP_RENAME (getresgid, gdb_sys_getresgid16);
      SYSCALL_MAP (prctl);
      SYSCALL_MAP (rt_sigreturn);
      SYSCALL_MAP (rt_sigaction);
      SYSCALL_MAP (rt_sigprocmask);
      SYSCALL_MAP (rt_sigpending);
      SYSCALL_MAP (rt_sigtimedwait);
      SYSCALL_MAP (rt_sigqueueinfo);
      SYSCALL_MAP (rt_sigsuspend);
      SYSCALL_MAP (pread64);
      SYSCALL_MAP (pwrite64);
      SYSCALL_MAP_RENAME (chown, gdb_sys_chown16);
      SYSCALL_MAP (getcwd);
      SYSCALL_MAP (capget);
      SYSCALL_MAP (capset);
      SYSCALL_MAP (sigaltstack);
      SYSCALL_MAP (sendfile);
      SYSCALL_MAP_RENAME (getpmsg, gdb_sys_ni_syscall188);
      SYSCALL_MAP_RENAME (putpmsg, gdb_sys_ni_syscall189);
      SYSCALL_MAP (vfork);
      SYSCALL_MAP_RENAME (ugetrlimit, gdb_sys_getrlimit);
      SYSCALL_MAP (mmap2);
      SYSCALL_MAP (truncate64);
      SYSCALL_MAP (ftruncate64);
      SYSCALL_MAP (stat64);
      SYSCALL_MAP (lstat64);
      SYSCALL_MAP (fstat64);

      SYSCALL_MAP_RENAME (lchown32, gdb_sys_lchown);
      SYSCALL_MAP_RENAME (getuid32, gdb_sys_getuid);
      SYSCALL_MAP_RENAME (getgid32, gdb_sys_getgid);
      SYSCALL_MAP_RENAME (geteuid32, gdb_sys_geteuid);
      SYSCALL_MAP_RENAME (getegid32, gdb_sys_getegid);
      SYSCALL_MAP_RENAME (setreuid32, gdb_sys_setreuid);
      SYSCALL_MAP_RENAME (setregid32, gdb_sys_setregid);
      SYSCALL_MAP_RENAME (getgroups32, gdb_sys_getgroups);
      SYSCALL_MAP_RENAME (setgroups32, gdb_sys_setgroups);
      SYSCALL_MAP_RENAME (fchown32, gdb_sys_fchown);
      SYSCALL_MAP_RENAME (setresuid32, gdb_sys_setresuid);
      SYSCALL_MAP_RENAME (getresuid32, gdb_sys_getresuid);
      SYSCALL_MAP_RENAME (setresgid32, gdb_sys_setresgid);
      SYSCALL_MAP_RENAME (getresgid32, gdb_sys_getresgid);
      SYSCALL_MAP_RENAME (chown32, gdb_sys_chown);
      SYSCALL_MAP_RENAME (setuid32, gdb_sys_setuid);
      SYSCALL_MAP_RENAME (setgid32, gdb_sys_setgid);
      SYSCALL_MAP_RENAME (setfsuid32, gdb_sys_setfsuid);
      SYSCALL_MAP_RENAME (setfsgid32, gdb_sys_setfsgid);

      SYSCALL_MAP (pivot_root);
      SYSCALL_MAP (mincore);
      SYSCALL_MAP (madvise);
      SYSCALL_MAP (getdents64);
      SYSCALL_MAP (fcntl64);
      SYSCALL_MAP (gettid);
      SYSCALL_MAP (readahead);
      SYSCALL_MAP (setxattr);
      SYSCALL_MAP (lsetxattr);
      SYSCALL_MAP (fsetxattr);
      SYSCALL_MAP (getxattr);
      SYSCALL_MAP (lgetxattr);
      SYSCALL_MAP (fgetxattr);
      SYSCALL_MAP (listxattr);
      SYSCALL_MAP (llistxattr);
      SYSCALL_MAP (flistxattr);
      SYSCALL_MAP (removexattr);
      SYSCALL_MAP (lremovexattr);
      SYSCALL_MAP (fremovexattr);
      SYSCALL_MAP (tkill);
      SYSCALL_MAP (sendfile64);
      SYSCALL_MAP (futex);
      SYSCALL_MAP (sched_setaffinity);
      SYSCALL_MAP (sched_getaffinity);
      SYSCALL_MAP (set_thread_area);
      SYSCALL_MAP (get_thread_area);
      SYSCALL_MAP (io_setup);
      SYSCALL_MAP (io_destroy);
      SYSCALL_MAP (io_getevents);
      SYSCALL_MAP (io_submit);
      SYSCALL_MAP (io_cancel);
      SYSCALL_MAP (fadvise64);
      SYSCALL_MAP (exit_group);
      SYSCALL_MAP (lookup_dcookie);
      SYSCALL_MAP (epoll_create);
      SYSCALL_MAP (epoll_ctl);
      SYSCALL_MAP (epoll_wait);
      SYSCALL_MAP (remap_file_pages);
      SYSCALL_MAP (set_tid_address);
      SYSCALL_MAP (timer_create);
      SYSCALL_MAP (timer_settime);
      SYSCALL_MAP (timer_gettime);
      SYSCALL_MAP (timer_getoverrun);
      SYSCALL_MAP (timer_delete);
      SYSCALL_MAP (clock_settime);
      SYSCALL_MAP (clock_gettime);
      SYSCALL_MAP (clock_getres);
      SYSCALL_MAP (clock_nanosleep);
      SYSCALL_MAP (statfs64);
      SYSCALL_MAP (fstatfs64);
      SYSCALL_MAP (tgkill);
      SYSCALL_MAP (utimes);
      SYSCALL_MAP (fadvise64_64);
      SYSCALL_MAP_RENAME (vserver, gdb_sys_ni_syscall273);
      SYSCALL_MAP (mbind);
      SYSCALL_MAP (get_mempolicy);
      SYSCALL_MAP (set_mempolicy);
      SYSCALL_MAP (mq_open);
      SYSCALL_MAP (mq_unlink);
      SYSCALL_MAP (mq_timedsend);
      SYSCALL_MAP (mq_timedreceive);
      SYSCALL_MAP (mq_notify);
      SYSCALL_MAP (mq_getsetattr);
      SYSCALL_MAP (kexec_load);
      SYSCALL_MAP (waitid);
      SYSCALL_MAP (add_key);
      SYSCALL_MAP (request_key);
      SYSCALL_MAP (keyctl);
      SYSCALL_MAP (ioprio_set);
      SYSCALL_MAP (ioprio_get);
      SYSCALL_MAP (inotify_init);
      SYSCALL_MAP (inotify_add_watch);
      SYSCALL_MAP (inotify_rm_watch);
      SYSCALL_MAP (migrate_pages);
      SYSCALL_MAP (openat);
      SYSCALL_MAP (mkdirat);
      SYSCALL_MAP (mknodat);
      SYSCALL_MAP (fchownat);
      SYSCALL_MAP (futimesat);
      SYSCALL_MAP (fstatat64);
      SYSCALL_MAP (unlinkat);
      SYSCALL_MAP (renameat);
      SYSCALL_MAP (linkat);
      SYSCALL_MAP (symlinkat);
      SYSCALL_MAP (readlinkat);
      SYSCALL_MAP (fchmodat);
      SYSCALL_MAP (faccessat);
      SYSCALL_MAP (pselect6);
      SYSCALL_MAP (ppoll);
      SYSCALL_MAP (unshare);
      SYSCALL_MAP (set_robust_list);
      SYSCALL_MAP (get_robust_list);
      SYSCALL_MAP (splice);
      SYSCALL_MAP (sync_file_range);
      SYSCALL_MAP (tee);
      SYSCALL_MAP (vmsplice);
      SYSCALL_MAP (move_pages);
      SYSCALL_MAP (getcpu);
      SYSCALL_MAP (epoll_pwait);
      UNSUPPORTED_SYSCALL_MAP (utimensat);
      UNSUPPORTED_SYSCALL_MAP (signalfd);
      UNSUPPORTED_SYSCALL_MAP (timerfd_create);
      UNSUPPORTED_SYSCALL_MAP (eventfd);
      SYSCALL_MAP (fallocate);
      UNSUPPORTED_SYSCALL_MAP (timerfd_settime);
      UNSUPPORTED_SYSCALL_MAP (timerfd_gettime);
      UNSUPPORTED_SYSCALL_MAP (signalfd4);
      SYSCALL_MAP (eventfd2);
      SYSCALL_MAP (epoll_create1);
      SYSCALL_MAP (dup3);
      SYSCALL_MAP (pipe2);
      SYSCALL_MAP (inotify_init1);
      UNSUPPORTED_SYSCALL_MAP (preadv);
      UNSUPPORTED_SYSCALL_MAP (pwritev);
      UNSUPPORTED_SYSCALL_MAP (rt_tgsigqueueinfo);
      UNSUPPORTED_SYSCALL_MAP (perf_event_open);
      UNSUPPORTED_SYSCALL_MAP (recvmmsg);
      UNSUPPORTED_SYSCALL_MAP (fanotify_init);
      UNSUPPORTED_SYSCALL_MAP (fanotify_mark);
      UNSUPPORTED_SYSCALL_MAP (prlimit64);
      UNSUPPORTED_SYSCALL_MAP (name_to_handle_at);
      UNSUPPORTED_SYSCALL_MAP (open_by_handle_at);
      UNSUPPORTED_SYSCALL_MAP (clock_adjtime);
      UNSUPPORTED_SYSCALL_MAP (syncfs);
      UNSUPPORTED_SYSCALL_MAP (sendmmsg);
      UNSUPPORTED_SYSCALL_MAP (setns);
      UNSUPPORTED_SYSCALL_MAP (process_vm_readv);
      UNSUPPORTED_SYSCALL_MAP (process_vm_writev);
      UNSUPPORTED_SYSCALL_MAP (kcmp);
      UNSUPPORTED_SYSCALL_MAP (finit_module);
      UNSUPPORTED_SYSCALL_MAP (sched_setattr);
      UNSUPPORTED_SYSCALL_MAP (sched_getattr);
      UNSUPPORTED_SYSCALL_MAP (renameat2);
      UNSUPPORTED_SYSCALL_MAP (seccomp);
      SYSCALL_MAP (getrandom);
      UNSUPPORTED_SYSCALL_MAP (memfd_create);
      UNSUPPORTED_SYSCALL_MAP (bpf);
      UNSUPPORTED_SYSCALL_MAP (execveat);
      SYSCALL_MAP (socket);
      SYSCALL_MAP (socketpair);
      SYSCALL_MAP (bind);
      SYSCALL_MAP (connect);
      SYSCALL_MAP (listen);
      UNSUPPORTED_SYSCALL_MAP (accept4);
      SYSCALL_MAP (getsockopt);
      SYSCALL_MAP (setsockopt);
      SYSCALL_MAP (getsockname);
      SYSCALL_MAP (getpeername);
      SYSCALL_MAP (sendto);
      SYSCALL_MAP (sendmsg);
      SYSCALL_MAP (recvfrom);
      SYSCALL_MAP (recvmsg);
      SYSCALL_MAP (shutdown);
      UNSUPPORTED_SYSCALL_MAP (userfaultfd);
      UNSUPPORTED_SYSCALL_MAP (membarrier);
      UNSUPPORTED_SYSCALL_MAP (mlock2);
      UNSUPPORTED_SYSCALL_MAP (copy_file_range);
      UNSUPPORTED_SYSCALL_MAP (preadv2);
      UNSUPPORTED_SYSCALL_MAP (pwritev2);
      UNSUPPORTED_SYSCALL_MAP (pkey_mprotect);
      UNSUPPORTED_SYSCALL_MAP (pkey_alloc);
      UNSUPPORTED_SYSCALL_MAP (pkey_free);
      SYSCALL_MAP (statx);
      UNSUPPORTED_SYSCALL_MAP (arch_prctl);
      UNSUPPORTED_SYSCALL_MAP (io_pgetevents);
      UNSUPPORTED_SYSCALL_MAP (rseq);
      SYSCALL_MAP (semget);
      SYSCALL_MAP (semctl);
      SYSCALL_MAP (shmget);
      SYSCALL_MAP (shmctl);
      SYSCALL_MAP (shmat);
      SYSCALL_MAP (shmdt);
      SYSCALL_MAP (msgget);
      SYSCALL_MAP (msgsnd);
      SYSCALL_MAP (msgrcv);
      SYSCALL_MAP (msgctl);
      SYSCALL_MAP (clock_gettime64);
      UNSUPPORTED_SYSCALL_MAP (clock_settime64);
      UNSUPPORTED_SYSCALL_MAP (clock_adjtime64);
      UNSUPPORTED_SYSCALL_MAP (clock_getres_time64);
      UNSUPPORTED_SYSCALL_MAP (clock_nanosleep_time64);
      UNSUPPORTED_SYSCALL_MAP (timer_gettime64);
      UNSUPPORTED_SYSCALL_MAP (timer_settime64);
      UNSUPPORTED_SYSCALL_MAP (timerfd_gettime64);
      UNSUPPORTED_SYSCALL_MAP (timerfd_settime64);
      UNSUPPORTED_SYSCALL_MAP (utimensat_time64);
      UNSUPPORTED_SYSCALL_MAP (pselect6_time64);
      UNSUPPORTED_SYSCALL_MAP (ppoll_time64);
      UNSUPPORTED_SYSCALL_MAP (io_pgetevents_time64);
      UNSUPPORTED_SYSCALL_MAP (recvmmsg_time64);
      UNSUPPORTED_SYSCALL_MAP (mq_timedsend_time64);
      UNSUPPORTED_SYSCALL_MAP (mq_timedreceive_time64);
      SYSCALL_MAP_RENAME (semtimedop_time64, gdb_sys_semtimedop);
      UNSUPPORTED_SYSCALL_MAP (rt_sigtimedwait_time64);
      UNSUPPORTED_SYSCALL_MAP (futex_time64);
      UNSUPPORTED_SYSCALL_MAP (sched_rr_get_interval_time64);
      UNSUPPORTED_SYSCALL_MAP (pidfd_send_signal);
      UNSUPPORTED_SYSCALL_MAP (io_uring_setup);
      UNSUPPORTED_SYSCALL_MAP (io_uring_enter);
      UNSUPPORTED_SYSCALL_MAP (io_uring_register);
      UNSUPPORTED_SYSCALL_MAP (open_tree);
      UNSUPPORTED_SYSCALL_MAP (move_mount);
      UNSUPPORTED_SYSCALL_MAP (fsopen);
      UNSUPPORTED_SYSCALL_MAP (fsconfig);
      UNSUPPORTED_SYSCALL_MAP (fsmount);
      UNSUPPORTED_SYSCALL_MAP (fspick);
      UNSUPPORTED_SYSCALL_MAP (pidfd_open);
      UNSUPPORTED_SYSCALL_MAP (clone3);
      UNSUPPORTED_SYSCALL_MAP (close_range);
      UNSUPPORTED_SYSCALL_MAP (openat2);
      UNSUPPORTED_SYSCALL_MAP (pidfd_getfd);
      UNSUPPORTED_SYSCALL_MAP (faccessat2);
      UNSUPPORTED_SYSCALL_MAP (process_madvise);
      UNSUPPORTED_SYSCALL_MAP (epoll_pwait2);
      UNSUPPORTED_SYSCALL_MAP (mount_setattr);
      UNSUPPORTED_SYSCALL_MAP (quotactl_fd);
      UNSUPPORTED_SYSCALL_MAP (landlock_create_ruleset);
      UNSUPPORTED_SYSCALL_MAP (landlock_add_rule);
      UNSUPPORTED_SYSCALL_MAP (landlock_restrict_self);
      UNSUPPORTED_SYSCALL_MAP (memfd_secret);
      UNSUPPORTED_SYSCALL_MAP (process_mrelease);
      UNSUPPORTED_SYSCALL_MAP (futex_waitv);
      UNSUPPORTED_SYSCALL_MAP (set_mempolicy_home_node);
      UNSUPPORTED_SYSCALL_MAP (cachestat);
      UNSUPPORTED_SYSCALL_MAP (fchmodat2);
      UNSUPPORTED_SYSCALL_MAP (map_shadow_stack);
      UNSUPPORTED_SYSCALL_MAP (futex_wake);
      UNSUPPORTED_SYSCALL_MAP (futex_wait);
      UNSUPPORTED_SYSCALL_MAP (futex_requeue);
      UNSUPPORTED_SYSCALL_MAP (statmount);
      UNSUPPORTED_SYSCALL_MAP (listmount);
      UNSUPPORTED_SYSCALL_MAP (lsm_get_self_attr);
      UNSUPPORTED_SYSCALL_MAP (lsm_set_self_attr);
      UNSUPPORTED_SYSCALL_MAP (lsm_list_modules);
      UNSUPPORTED_SYSCALL_MAP (mseal);
      UNSUPPORTED_SYSCALL_MAP (setxattrat);
      UNSUPPORTED_SYSCALL_MAP (getxattrat);
      UNSUPPORTED_SYSCALL_MAP (listxattrat);
      UNSUPPORTED_SYSCALL_MAP (removexattrat);

#undef SYSCALL_MAP
#undef SYSCALL_MAP_RENAME
#undef UNSUPPORTED_SYSCALL_MAP

    default:
      return gdb_sys_no_syscall;
    }
}

/* Value of the sigcode in case of a boundary fault.  */

#define SIG_CODE_BOUNDARY_FAULT 3

/* Parse the arguments of current system call instruction and record
   the values of the registers and memory that will be changed into
   "record_arch_list".  This instruction is "int 0x80" (Linux
   Kernel2.4) or "sysenter" (Linux Kernel 2.6).

   Return -1 if something wrong.  */

static struct linux_record_tdep i386_linux_record_tdep;

static int
i386_linux_intx80_sysenter_syscall_record (struct regcache *regcache)
{
  int ret;
  LONGEST syscall_native;
  enum gdb_syscall syscall_gdb;

  regcache_raw_read_signed (regcache, I386_EAX_REGNUM, &syscall_native);

  syscall_gdb = i386_canonicalize_syscall (syscall_native);

  if (syscall_gdb < 0)
    {
      gdb_printf (gdb_stderr,
		  _("Process record and replay target doesn't "
		    "support syscall number %s\n"), 
		  plongest (syscall_native));
      return -1;
    }

  if (syscall_gdb == gdb_sys_sigreturn
      || syscall_gdb == gdb_sys_rt_sigreturn)
   {
     if (i386_all_but_ip_registers_record (regcache))
       return -1;
     return 0;
   }

  ret = record_linux_system_call (syscall_gdb, regcache,
				  &i386_linux_record_tdep);
  if (ret)
    return ret;

  /* Record the return value of the system call.  */
  if (record_full_arch_list_add_reg (regcache, I386_EAX_REGNUM))
    return -1;

  return 0;
}

#define I386_LINUX_xstate	270
#define I386_LINUX_frame_size	732

static int
i386_linux_record_signal (struct gdbarch *gdbarch,
			  struct regcache *regcache,
			  enum gdb_signal signal)
{
  ULONGEST esp;

  if (i386_all_but_ip_registers_record (regcache))
    return -1;

  if (record_full_arch_list_add_reg (regcache, I386_EIP_REGNUM))
    return -1;

  /* Record the change in the stack.  */
  regcache_raw_read_unsigned (regcache, I386_ESP_REGNUM, &esp);
  /* This is for xstate.
     sp -= sizeof (struct _fpstate);  */
  esp -= I386_LINUX_xstate;
  /* This is for frame_size.
     sp -= sizeof (struct rt_sigframe);  */
  esp -= I386_LINUX_frame_size;
  if (record_full_arch_list_add_mem (esp,
				     I386_LINUX_xstate + I386_LINUX_frame_size))
    return -1;

  if (record_full_arch_list_add_end ())
    return -1;

  return 0;
}


/* Core of the implementation for gdbarch get_syscall_number.  Get pending
   syscall number from REGCACHE.  If there is no pending syscall -1 will be
   returned.  Pending syscall means ptrace has stepped into the syscall but
   another ptrace call will step out.  PC is right after the int $0x80
   / syscall / sysenter instruction in both cases, PC does not change during
   the second ptrace step.  */

static LONGEST
i386_linux_get_syscall_number_from_regcache (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The content of a register.  */
  gdb_byte buf[4];
  /* The result.  */
  LONGEST ret;

  /* Getting the system call number from the register.
     When dealing with x86 architecture, this information
     is stored at %eax register.  */
  regcache->cooked_read (I386_LINUX_ORIG_EAX_REGNUM, buf);

  ret = extract_signed_integer (buf, byte_order);

  return ret;
}

/* Wrapper for i386_linux_get_syscall_number_from_regcache to make it
   compatible with gdbarch get_syscall_number method prototype.  */

static LONGEST
i386_linux_get_syscall_number (struct gdbarch *gdbarch,
			       thread_info *thread)
{
  struct regcache *regcache = get_thread_regcache (thread);

  return i386_linux_get_syscall_number_from_regcache (regcache);
}

/* The register sets used in GNU/Linux ELF core-dumps are identical to
   the register sets in `struct user' that are used for a.out
   core-dumps.  These are also used by ptrace(2).  The corresponding
   types are `elf_gregset_t' for the general-purpose registers (with
   `elf_greg_t' the type of a single GP register) and `elf_fpregset_t'
   for the floating-point registers.

   Those types used to be available under the names `gregset_t' and
   `fpregset_t' too, and GDB used those names in the past.  But those
   names are now used for the register sets used in the `mcontext_t'
   type, which have a different size and layout.  */

/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register cache layout.  */

/* From <sys/reg.h>.  */
int i386_linux_gregset_reg_offset[] =
{
  6 * 4,			/* %eax */
  1 * 4,			/* %ecx */
  2 * 4,			/* %edx */
  0 * 4,			/* %ebx */
  15 * 4,			/* %esp */
  5 * 4,			/* %ebp */
  3 * 4,			/* %esi */
  4 * 4,			/* %edi */
  12 * 4,			/* %eip */
  14 * 4,			/* %eflags */
  13 * 4,			/* %cs */
  16 * 4,			/* %ss */
  7 * 4,			/* %ds */
  8 * 4,			/* %es */
  9 * 4,			/* %fs */
  10 * 4,			/* %gs */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  /* MPX is deprecated.  Yet we keep this to not give the registers below
     a new number.  That could break older gdbservers.  */
  -1, -1, -1, -1,		  /* MPX registers BND0 ... BND3.  */
  -1, -1,			  /* MPX registers BNDCFGU, BNDSTATUS.  */
  -1, -1, -1, -1, -1, -1, -1, -1, /* k0 ... k7 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1, /* zmm0 ... zmm7 (AVX512)  */
  -1,				  /* PKRU register  */
  11 * 4,			  /* "orig_eax"  */
};

/* Mapping between the general-purpose registers in `struct
   sigcontext' format and GDB's register cache layout.  */

/* From <asm/sigcontext.h>.  */
static int i386_linux_sc_reg_offset[] =
{
  11 * 4,			/* %eax */
  10 * 4,			/* %ecx */
  9 * 4,			/* %edx */
  8 * 4,			/* %ebx */
  7 * 4,			/* %esp */
  6 * 4,			/* %ebp */
  5 * 4,			/* %esi */
  4 * 4,			/* %edi */
  14 * 4,			/* %eip */
  16 * 4,			/* %eflags */
  15 * 4,			/* %cs */
  18 * 4,			/* %ss */
  3 * 4,			/* %ds */
  2 * 4,			/* %es */
  1 * 4,			/* %fs */
  0 * 4				/* %gs */
};

/* See i386-linux-tdep.h.  */

uint64_t
i386_linux_core_read_xsave_info (bfd *abfd, x86_xsave_layout &layout)
{
  asection *xstate = bfd_get_section_by_name (abfd, ".reg-xstate");
  if (xstate == nullptr)
    return 0;

  /* Check extended state size.  */
  size_t size = bfd_section_size (xstate);
  if (size < X86_XSTATE_AVX_SIZE)
    return 0;

  char contents[8];
  if (! bfd_get_section_contents (abfd, xstate, contents,
				  I386_LINUX_XSAVE_XCR0_OFFSET, 8))
    {
      warning (_("Couldn't read `xcr0' bytes from "
		 "`.reg-xstate' section in core file."));
      return 0;
    }

  uint64_t xcr0 = bfd_get_64 (abfd, contents);

  if (!i387_guess_xsave_layout (xcr0, size, layout))
    return 0;

  return xcr0;
}

/* See i386-linux-tdep.h.  */

bool
i386_linux_core_read_x86_xsave_layout (struct gdbarch *gdbarch,
				       x86_xsave_layout &layout)
{
  return i386_linux_core_read_xsave_info (current_program_space->core_bfd (),
					  layout) != 0;
}

/* See arch/x86-linux-tdesc.h.  */

void
x86_linux_post_init_tdesc (target_desc *tdesc, bool is_64bit)
{
  /* Nothing.  */
}

/* Get Linux/x86 target description from core dump.  */

static const struct target_desc *
i386_linux_core_read_description (struct gdbarch *gdbarch,
				  struct target_ops *target,
				  bfd *abfd)
{
  /* Linux/i386.  */
  x86_xsave_layout layout;
  uint64_t xcr0 = i386_linux_core_read_xsave_info (abfd, layout);

  if (xcr0 == 0)
    {
      if (bfd_get_section_by_name (abfd, ".reg-xfp") != nullptr)
	xcr0 = X86_XSTATE_SSE_MASK;
      else
	xcr0 = X86_XSTATE_X87_MASK;
    }

  return i386_linux_read_description (xcr0);
}

/* Similar to i386_supply_fpregset, but use XSAVE extended state.  */

static void
i386_linux_supply_xstateregset (const struct regset *regset,
				struct regcache *regcache, int regnum,
				const void *xstateregs, size_t len)
{
  i387_supply_xsave (regcache, regnum, xstateregs);
}

/* Similar to i386_collect_fpregset, but use XSAVE extended state.  */

static void
i386_linux_collect_xstateregset (const struct regset *regset,
				 const struct regcache *regcache,
				 int regnum, void *xstateregs, size_t len)
{
  i387_collect_xsave (regcache, regnum, xstateregs, 1);
}

/* Register set definitions.  */

static const struct regset i386_linux_xstateregset =
  {
    NULL,
    i386_linux_supply_xstateregset,
    i386_linux_collect_xstateregset
  };

/* Iterate over core file register note sections.  */

static void
i386_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  cb (".reg", 68, 68, &i386_gregset, NULL, cb_data);

  if (tdep->xsave_layout.sizeof_xsave != 0)
    cb (".reg-xstate", tdep->xsave_layout.sizeof_xsave,
	tdep->xsave_layout.sizeof_xsave, &i386_linux_xstateregset,
	"XSAVE extended state", cb_data);
  else if (tdep->xcr0 & X86_XSTATE_SSE)
    cb (".reg-xfp", 512, 512, &i386_fpregset, "extended floating-point",
	cb_data);
  else
    cb (".reg2", 108, 108, &i386_fpregset, NULL, cb_data);
}

/* Linux kernel shows PC value after the 'int $0x80' instruction even if
   inferior is still inside the syscall.  On next PTRACE_SINGLESTEP it will
   finish the syscall but PC will not change.
   
   Some vDSOs contain 'int $0x80; ret' and during stepping out of the syscall
   i386_displaced_step_fixup would keep PC at the displaced pad location.
   As PC is pointing to the 'ret' instruction before the step
   i386_displaced_step_fixup would expect inferior has just executed that 'ret'
   and PC should not be adjusted.  In reality it finished syscall instead and
   PC should get relocated back to its vDSO address.  Hide the 'ret'
   instruction by 'nop' so that i386_displaced_step_fixup is not confused.
   
   It is not fully correct as the bytes in struct
   displaced_step_copy_insn_closure will not match the inferior code.  But we
   would need some new flag in displaced_step_copy_insn_closure otherwise to
   keep the state that syscall is finishing for the later
   i386_displaced_step_fixup execution as the syscall execution is already no
   longer detectable there.  The new flag field would mean i386-linux-tdep.c
   needs to wrap all the displacement methods of i386-tdep.c which does not seem
   worth it.  The same effect is achieved by patching that 'nop' instruction
   there instead.  */

static displaced_step_copy_insn_closure_up
i386_linux_displaced_step_copy_insn (struct gdbarch *gdbarch,
				     CORE_ADDR from, CORE_ADDR to,
				     struct regcache *regs)
{
  displaced_step_copy_insn_closure_up closure_
    =  i386_displaced_step_copy_insn (gdbarch, from, to, regs);

  if (i386_linux_get_syscall_number_from_regcache (regs) != -1)
    {
      /* The closure returned by i386_displaced_step_copy_insn is simply a
	 buffer with a copy of the instruction. */
      i386_displaced_step_copy_insn_closure *closure
	= (i386_displaced_step_copy_insn_closure *) closure_.get ();

      /* Fake nop.  */
      closure->buf[0] = 0x90;
    }

  return closure_;
}

static void
i386_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  const struct target_desc *tdesc = info.target_desc;
  struct tdesc_arch_data *tdesc_data = info.tdesc_data;
  const struct tdesc_feature *feature;
  int valid_p;

  gdb_assert (tdesc_data);

  linux_init_abi (info, gdbarch, 1);

  /* GNU/Linux uses ELF.  */
  i386_elf_init_abi (info, gdbarch);

  /* Reserve a number for orig_eax.  */
  set_gdbarch_num_regs (gdbarch, I386_LINUX_NUM_REGS);

  if (! tdesc_has_registers (tdesc))
    tdesc = i386_linux_read_description (X86_XSTATE_SSE_MASK);
  tdep->tdesc = tdesc;

  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.i386.linux");
  if (feature == NULL)
    return;

  valid_p = tdesc_numbered_register (feature, tdesc_data,
				     I386_LINUX_ORIG_EAX_REGNUM,
				     "orig_eax");
  if (!valid_p)
    return;

  /* Add the %orig_eax register used for syscall restarting.  */
  set_gdbarch_write_pc (gdbarch, i386_linux_write_pc);

  tdep->register_reggroup_p = i386_linux_register_reggroup_p;

  tdep->gregset_reg_offset = i386_linux_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386_linux_gregset_reg_offset);
  tdep->sizeof_gregset = 17 * 4;

  tdep->jb_pc_offset = 20;	/* From <bits/setjmp.h>.  */

  tdep->sigtramp_p = i386_linux_sigtramp_p;
  tdep->sigcontext_addr = i386_linux_sigcontext_addr;
  tdep->sc_reg_offset = i386_linux_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (i386_linux_sc_reg_offset);

  tdep->xsave_xcr0_offset = I386_LINUX_XSAVE_XCR0_OFFSET;
  set_gdbarch_core_read_x86_xsave_layout
    (gdbarch, i386_linux_core_read_x86_xsave_layout);

  set_gdbarch_process_record (gdbarch, i386_process_record);
  set_gdbarch_process_record_signal (gdbarch, i386_linux_record_signal);

  /* Initialize the i386_linux_record_tdep.  */
  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */
  i386_linux_record_tdep.size_pointer
    = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  i386_linux_record_tdep.size__old_kernel_stat = 32;
  i386_linux_record_tdep.size_tms = 16;
  i386_linux_record_tdep.size_loff_t = 8;
  i386_linux_record_tdep.size_flock = 16;
  i386_linux_record_tdep.size_oldold_utsname = 45;
  i386_linux_record_tdep.size_ustat = 20;
  i386_linux_record_tdep.size_old_sigaction = 16;
  i386_linux_record_tdep.size_old_sigset_t = 4;
  i386_linux_record_tdep.size_rlimit = 8;
  i386_linux_record_tdep.size_rusage = 72;
  i386_linux_record_tdep.size_timeval = 8;
  i386_linux_record_tdep.size_timezone = 8;
  i386_linux_record_tdep.size_old_gid_t = 2;
  i386_linux_record_tdep.size_old_uid_t = 2;
  i386_linux_record_tdep.size_fd_set = 128;
  i386_linux_record_tdep.size_old_dirent = 268;
  i386_linux_record_tdep.size_statfs = 64;
  i386_linux_record_tdep.size_statfs64 = 84;
  i386_linux_record_tdep.size_sockaddr = 16;
  i386_linux_record_tdep.size_int
    = gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT;
  i386_linux_record_tdep.size_long
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  i386_linux_record_tdep.size_ulong
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  i386_linux_record_tdep.size_msghdr = 28;
  i386_linux_record_tdep.size_itimerval = 16;
  i386_linux_record_tdep.size_stat = 88;
  i386_linux_record_tdep.size_old_utsname = 325;
  i386_linux_record_tdep.size_sysinfo = 64;
  i386_linux_record_tdep.size_msqid_ds = 88;
  i386_linux_record_tdep.size_shmid_ds = 84;
  i386_linux_record_tdep.size_new_utsname = 390;
  i386_linux_record_tdep.size_timex = 128;
  i386_linux_record_tdep.size_mem_dqinfo = 24;
  i386_linux_record_tdep.size_if_dqblk = 68;
  i386_linux_record_tdep.size_fs_quota_stat = 68;
  i386_linux_record_tdep.size_timespec = 8;
  i386_linux_record_tdep.size_pollfd = 8;
  i386_linux_record_tdep.size_NFS_FHSIZE = 32;
  i386_linux_record_tdep.size_knfsd_fh = 132;
  i386_linux_record_tdep.size_TASK_COMM_LEN = 16;
  i386_linux_record_tdep.size_sigaction = 20;
  i386_linux_record_tdep.size_sigset_t = 8;
  i386_linux_record_tdep.size_siginfo_t = 128;
  i386_linux_record_tdep.size_cap_user_data_t = 12;
  i386_linux_record_tdep.size_stack_t = 12;
  i386_linux_record_tdep.size_off_t = i386_linux_record_tdep.size_long;
  i386_linux_record_tdep.size_stat64 = 96;
  i386_linux_record_tdep.size_gid_t = 4;
  i386_linux_record_tdep.size_uid_t = 4;
  i386_linux_record_tdep.size_PAGE_SIZE = 4096;
  i386_linux_record_tdep.size_flock64 = 24;
  i386_linux_record_tdep.size_user_desc = 16;
  i386_linux_record_tdep.size_io_event = 32;
  i386_linux_record_tdep.size_iocb = 64;
  i386_linux_record_tdep.size_epoll_event = 12;
  i386_linux_record_tdep.size_itimerspec
    = i386_linux_record_tdep.size_timespec * 2;
  i386_linux_record_tdep.size_mq_attr = 32;
  i386_linux_record_tdep.size_termios = 36;
  i386_linux_record_tdep.size_termios2 = 44;
  i386_linux_record_tdep.size_pid_t = 4;
  i386_linux_record_tdep.size_winsize = 8;
  i386_linux_record_tdep.size_serial_struct = 60;
  i386_linux_record_tdep.size_serial_icounter_struct = 80;
  i386_linux_record_tdep.size_hayes_esp_config = 12;
  i386_linux_record_tdep.size_size_t = 4;
  i386_linux_record_tdep.size_iovec = 8;
  i386_linux_record_tdep.size_time_t = 4;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.  */
  i386_linux_record_tdep.ioctl_TCGETS = 0x5401;
  i386_linux_record_tdep.ioctl_TCSETS = 0x5402;
  i386_linux_record_tdep.ioctl_TCSETSW = 0x5403;
  i386_linux_record_tdep.ioctl_TCSETSF = 0x5404;
  i386_linux_record_tdep.ioctl_TCGETA = 0x5405;
  i386_linux_record_tdep.ioctl_TCSETA = 0x5406;
  i386_linux_record_tdep.ioctl_TCSETAW = 0x5407;
  i386_linux_record_tdep.ioctl_TCSETAF = 0x5408;
  i386_linux_record_tdep.ioctl_TCSBRK = 0x5409;
  i386_linux_record_tdep.ioctl_TCXONC = 0x540A;
  i386_linux_record_tdep.ioctl_TCFLSH = 0x540B;
  i386_linux_record_tdep.ioctl_TIOCEXCL = 0x540C;
  i386_linux_record_tdep.ioctl_TIOCNXCL = 0x540D;
  i386_linux_record_tdep.ioctl_TIOCSCTTY = 0x540E;
  i386_linux_record_tdep.ioctl_TIOCGPGRP = 0x540F;
  i386_linux_record_tdep.ioctl_TIOCSPGRP = 0x5410;
  i386_linux_record_tdep.ioctl_TIOCOUTQ = 0x5411;
  i386_linux_record_tdep.ioctl_TIOCSTI = 0x5412;
  i386_linux_record_tdep.ioctl_TIOCGWINSZ = 0x5413;
  i386_linux_record_tdep.ioctl_TIOCSWINSZ = 0x5414;
  i386_linux_record_tdep.ioctl_TIOCMGET = 0x5415;
  i386_linux_record_tdep.ioctl_TIOCMBIS = 0x5416;
  i386_linux_record_tdep.ioctl_TIOCMBIC = 0x5417;
  i386_linux_record_tdep.ioctl_TIOCMSET = 0x5418;
  i386_linux_record_tdep.ioctl_TIOCGSOFTCAR = 0x5419;
  i386_linux_record_tdep.ioctl_TIOCSSOFTCAR = 0x541A;
  i386_linux_record_tdep.ioctl_FIONREAD = 0x541B;
  i386_linux_record_tdep.ioctl_TIOCINQ = i386_linux_record_tdep.ioctl_FIONREAD;
  i386_linux_record_tdep.ioctl_TIOCLINUX = 0x541C;
  i386_linux_record_tdep.ioctl_TIOCCONS = 0x541D;
  i386_linux_record_tdep.ioctl_TIOCGSERIAL = 0x541E;
  i386_linux_record_tdep.ioctl_TIOCSSERIAL = 0x541F;
  i386_linux_record_tdep.ioctl_TIOCPKT = 0x5420;
  i386_linux_record_tdep.ioctl_FIONBIO = 0x5421;
  i386_linux_record_tdep.ioctl_TIOCNOTTY = 0x5422;
  i386_linux_record_tdep.ioctl_TIOCSETD = 0x5423;
  i386_linux_record_tdep.ioctl_TIOCGETD = 0x5424;
  i386_linux_record_tdep.ioctl_TCSBRKP = 0x5425;
  i386_linux_record_tdep.ioctl_TIOCTTYGSTRUCT = 0x5426;
  i386_linux_record_tdep.ioctl_TIOCSBRK = 0x5427;
  i386_linux_record_tdep.ioctl_TIOCCBRK = 0x5428;
  i386_linux_record_tdep.ioctl_TIOCGSID = 0x5429;
  i386_linux_record_tdep.ioctl_TCGETS2 = 0x802c542a;
  i386_linux_record_tdep.ioctl_TCSETS2 = 0x402c542b;
  i386_linux_record_tdep.ioctl_TCSETSW2 = 0x402c542c;
  i386_linux_record_tdep.ioctl_TCSETSF2 = 0x402c542d;
  i386_linux_record_tdep.ioctl_TIOCGPTN = 0x80045430;
  i386_linux_record_tdep.ioctl_TIOCSPTLCK = 0x40045431;
  i386_linux_record_tdep.ioctl_FIONCLEX = 0x5450;
  i386_linux_record_tdep.ioctl_FIOCLEX = 0x5451;
  i386_linux_record_tdep.ioctl_FIOASYNC = 0x5452;
  i386_linux_record_tdep.ioctl_TIOCSERCONFIG = 0x5453;
  i386_linux_record_tdep.ioctl_TIOCSERGWILD = 0x5454;
  i386_linux_record_tdep.ioctl_TIOCSERSWILD = 0x5455;
  i386_linux_record_tdep.ioctl_TIOCGLCKTRMIOS = 0x5456;
  i386_linux_record_tdep.ioctl_TIOCSLCKTRMIOS = 0x5457;
  i386_linux_record_tdep.ioctl_TIOCSERGSTRUCT = 0x5458;
  i386_linux_record_tdep.ioctl_TIOCSERGETLSR = 0x5459;
  i386_linux_record_tdep.ioctl_TIOCSERGETMULTI = 0x545A;
  i386_linux_record_tdep.ioctl_TIOCSERSETMULTI = 0x545B;
  i386_linux_record_tdep.ioctl_TIOCMIWAIT = 0x545C;
  i386_linux_record_tdep.ioctl_TIOCGICOUNT = 0x545D;
  i386_linux_record_tdep.ioctl_TIOCGHAYESESP = 0x545E;
  i386_linux_record_tdep.ioctl_TIOCSHAYESESP = 0x545F;
  i386_linux_record_tdep.ioctl_FIOQSIZE = 0x5460;

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  i386_linux_record_tdep.fcntl_F_GETLK = 5;
  i386_linux_record_tdep.fcntl_F_GETLK64 = 12;
  i386_linux_record_tdep.fcntl_F_SETLK64 = 13;
  i386_linux_record_tdep.fcntl_F_SETLKW64 = 14;

  i386_linux_record_tdep.arg1 = I386_EBX_REGNUM;
  i386_linux_record_tdep.arg2 = I386_ECX_REGNUM;
  i386_linux_record_tdep.arg3 = I386_EDX_REGNUM;
  i386_linux_record_tdep.arg4 = I386_ESI_REGNUM;
  i386_linux_record_tdep.arg5 = I386_EDI_REGNUM;
  i386_linux_record_tdep.arg6 = I386_EBP_REGNUM;

  tdep->i386_intx80_record = i386_linux_intx80_sysenter_syscall_record;
  tdep->i386_sysenter_record = i386_linux_intx80_sysenter_syscall_record;
  tdep->i386_syscall_record = i386_linux_intx80_sysenter_syscall_record;

  /* N_FUN symbols in shared libraries have 0 for their values and need
     to be relocated.  */
  set_gdbarch_sofun_address_maybe_missing (gdbarch, 1);

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_ilp32_fetch_link_map_offsets);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  dwarf2_frame_set_signal_frame_p (gdbarch, i386_linux_dwarf_signal_frame_p);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Core file support.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, i386_linux_iterate_over_regset_sections);
  set_gdbarch_core_read_description (gdbarch,
				     i386_linux_core_read_description);

  /* Displaced stepping.  */
  set_gdbarch_displaced_step_copy_insn (gdbarch,
					i386_linux_displaced_step_copy_insn);
  set_gdbarch_displaced_step_fixup (gdbarch, i386_displaced_step_fixup);

  /* Functions for 'catch syscall'.  */
  set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_I386);
  set_gdbarch_get_syscall_number (gdbarch,
				  i386_linux_get_syscall_number);
}

void _initialize_i386_linux_tdep ();
void
_initialize_i386_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_LINUX,
			  i386_linux_init_abi);
}
