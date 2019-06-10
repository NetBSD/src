/* Low level interface to SPUs, for the remote server for GDB.
   Copyright (C) 2006-2017 Free Software Foundation, Inc.

   Contributed by Ulrich Weigand <uweigand@de.ibm.com>.

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

#include "server.h"

#include "gdb_wait.h"
#include <sys/ptrace.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "filestuff.h"
#include "hostio.h"

/* Some older glibc versions do not define this.  */
#ifndef __WNOTHREAD
#define __WNOTHREAD     0x20000000      /* Don't wait on children of other
					   threads in this group */
#endif

#define PTRACE_TYPE_RET long
#define PTRACE_TYPE_ARG3 long

/* Number of registers.  */
#define SPU_NUM_REGS         130
#define SPU_NUM_CORE_REGS    128

/* Special registers.  */
#define SPU_ID_REGNUM        128
#define SPU_PC_REGNUM        129

/* PPU side system calls.  */
#define INSTR_SC	0x44000002
#define NR_spu_run	0x0116

/* These are used in remote-utils.c.  */
int using_threads = 0;

/* Defined in auto-generated file reg-spu.c.  */
void init_registers_spu (void);
extern const struct target_desc *tdesc_spu;

/* Software breakpoint instruction.  */
static const gdb_byte breakpoint[] = { 0x00, 0x00, 0x3f, 0xff };

/* Fetch PPU register REGNO.  */
static CORE_ADDR
fetch_ppc_register (int regno)
{
  PTRACE_TYPE_RET res;

  int tid = ptid_get_lwp (current_ptid);

#ifndef __powerpc64__
  /* If running as a 32-bit process on a 64-bit system, we attempt
     to get the full 64-bit register content of the target process.
     If the PPC special ptrace call fails, we're on a 32-bit system;
     just fall through to the regular ptrace call in that case.  */
  {
    char buf[8];

    errno = 0;
    ptrace ((PTRACE_TYPE_ARG1) PPC_PTRACE_PEEKUSR_3264, tid,
	    (PTRACE_TYPE_ARG3) (regno * 8), buf);
    if (errno == 0)
      ptrace ((PTRACE_TYPE_ARG1) PPC_PTRACE_PEEKUSR_3264, tid,
	      (PTRACE_TYPE_ARG3) (regno * 8 + 4), buf + 4);
    if (errno == 0)
      return (CORE_ADDR) *(unsigned long long *)buf;
  }
#endif

  errno = 0;
  res = ptrace (PT_READ_U, tid,
		(PTRACE_TYPE_ARG3) (regno * sizeof (PTRACE_TYPE_RET)), 0);
  if (errno != 0)
    {
      char mess[128];
      sprintf (mess, "reading PPC register #%d", regno);
      perror_with_name (mess);
    }

  return (CORE_ADDR) (unsigned long) res;
}

/* Fetch WORD from PPU memory at (aligned) MEMADDR in thread TID.  */
static int
fetch_ppc_memory_1 (int tid, CORE_ADDR memaddr, PTRACE_TYPE_RET *word)
{
  errno = 0;

#ifndef __powerpc64__
  if (memaddr >> 32)
    {
      unsigned long long addr_8 = (unsigned long long) memaddr;
      ptrace ((PTRACE_TYPE_ARG1) PPC_PTRACE_PEEKTEXT_3264, tid,
	      (PTRACE_TYPE_ARG3) &addr_8, word);
    }
  else
#endif
    *word = ptrace (PT_READ_I, tid, (PTRACE_TYPE_ARG3) (size_t) memaddr, 0);

  return errno;
}

/* Store WORD into PPU memory at (aligned) MEMADDR in thread TID.  */
static int
store_ppc_memory_1 (int tid, CORE_ADDR memaddr, PTRACE_TYPE_RET word)
{
  errno = 0;

#ifndef __powerpc64__
  if (memaddr >> 32)
    {
      unsigned long long addr_8 = (unsigned long long) memaddr;
      ptrace ((PTRACE_TYPE_ARG1) PPC_PTRACE_POKEDATA_3264, tid,
	      (PTRACE_TYPE_ARG3) &addr_8, word);
    }
  else
#endif
    ptrace (PT_WRITE_D, tid, (PTRACE_TYPE_ARG3) (size_t) memaddr, word);

  return errno;
}

/* Fetch LEN bytes of PPU memory at MEMADDR to MYADDR.  */
static int
fetch_ppc_memory (CORE_ADDR memaddr, char *myaddr, int len)
{
  int i, ret;

  CORE_ADDR addr = memaddr & -(CORE_ADDR) sizeof (PTRACE_TYPE_RET);
  int count = ((((memaddr + len) - addr) + sizeof (PTRACE_TYPE_RET) - 1)
	       / sizeof (PTRACE_TYPE_RET));
  PTRACE_TYPE_RET *buffer;

  int tid = ptid_get_lwp (current_ptid);

  buffer = XALLOCAVEC (PTRACE_TYPE_RET, count);
  for (i = 0; i < count; i++, addr += sizeof (PTRACE_TYPE_RET))
    if ((ret = fetch_ppc_memory_1 (tid, addr, &buffer[i])) != 0)
      return ret;

  memcpy (myaddr,
	  (char *) buffer + (memaddr & (sizeof (PTRACE_TYPE_RET) - 1)),
	  len);

  return 0;
}

/* Store LEN bytes from MYADDR to PPU memory at MEMADDR.  */
static int
store_ppc_memory (CORE_ADDR memaddr, char *myaddr, int len)
{
  int i, ret;

  CORE_ADDR addr = memaddr & -(CORE_ADDR) sizeof (PTRACE_TYPE_RET);
  int count = ((((memaddr + len) - addr) + sizeof (PTRACE_TYPE_RET) - 1)
	       / sizeof (PTRACE_TYPE_RET));
  PTRACE_TYPE_RET *buffer;

  int tid = ptid_get_lwp (current_ptid);

  buffer = XALLOCAVEC (PTRACE_TYPE_RET, count);

  if (addr != memaddr || len < (int) sizeof (PTRACE_TYPE_RET))
    if ((ret = fetch_ppc_memory_1 (tid, addr, &buffer[0])) != 0)
      return ret;

  if (count > 1)
    if ((ret = fetch_ppc_memory_1 (tid, addr + (count - 1)
					       * sizeof (PTRACE_TYPE_RET),
				   &buffer[count - 1])) != 0)
      return ret;

  memcpy ((char *) buffer + (memaddr & (sizeof (PTRACE_TYPE_RET) - 1)),
	  myaddr, len);

  for (i = 0; i < count; i++, addr += sizeof (PTRACE_TYPE_RET))
    if ((ret = store_ppc_memory_1 (tid, addr, buffer[i])) != 0)
      return ret;

  return 0;
}


/* If the PPU thread is currently stopped on a spu_run system call,
   return to FD and ADDR the file handle and NPC parameter address
   used with the system call.  Return non-zero if successful.  */
static int
parse_spufs_run (int *fd, CORE_ADDR *addr)
{
  unsigned int insn;
  CORE_ADDR pc = fetch_ppc_register (32);  /* nip */

  /* Fetch instruction preceding current NIP.  */
  if (fetch_ppc_memory (pc-4, (char *) &insn, 4) != 0)
    return 0;
  /* It should be a "sc" instruction.  */
  if (insn != INSTR_SC)
    return 0;
  /* System call number should be NR_spu_run.  */
  if (fetch_ppc_register (0) != NR_spu_run)
    return 0;

  /* Register 3 contains fd, register 4 the NPC param pointer.  */
  *fd = fetch_ppc_register (34);  /* orig_gpr3 */
  *addr = fetch_ppc_register (4);
  return 1;
}


/* Copy LEN bytes at OFFSET in spufs file ANNEX into/from READBUF or WRITEBUF,
   using the /proc file system.  */
static int
spu_proc_xfer_spu (const char *annex, unsigned char *readbuf,
		   const unsigned char *writebuf,
		   CORE_ADDR offset, int len)
{
  char buf[128];
  int fd = 0;
  int ret = -1;

  if (!annex)
    return 0;

  sprintf (buf, "/proc/%ld/fd/%s", ptid_get_lwp (current_ptid), annex);
  fd = open (buf, writebuf? O_WRONLY : O_RDONLY);
  if (fd <= 0)
    return -1;

  if (offset != 0
      && lseek (fd, (off_t) offset, SEEK_SET) != (off_t) offset)
    {
      close (fd);
      return 0;
    }

  if (writebuf)
    ret = write (fd, writebuf, (size_t) len);
  else if (readbuf)
    ret = read (fd, readbuf, (size_t) len);

  close (fd);
  return ret;
}


/* Start an inferior process and returns its pid.
   ALLARGS is a vector of program-name and args. */
static int
spu_create_inferior (char *program, char **allargs)
{
  int pid;
  ptid_t ptid;
  struct process_info *proc;

  pid = fork ();
  if (pid < 0)
    perror_with_name ("fork");

  if (pid == 0)
    {
      close_most_fds ();
      ptrace (PTRACE_TRACEME, 0, 0, 0);

      setpgid (0, 0);

      execv (program, allargs);
      if (errno == ENOENT)
	execvp (program, allargs);

      fprintf (stderr, "Cannot exec %s: %s.\n", program,
	       strerror (errno));
      fflush (stderr);
      _exit (0177);
    }

  proc = add_process (pid, 0);
  proc->tdesc = tdesc_spu;

  ptid = ptid_build (pid, pid, 0);
  add_thread (ptid, NULL);
  return pid;
}

/* Attach to an inferior process.  */
int
spu_attach (unsigned long  pid)
{
  ptid_t ptid;
  struct process_info *proc;

  if (ptrace (PTRACE_ATTACH, pid, 0, 0) != 0)
    {
      fprintf (stderr, "Cannot attach to process %ld: %s (%d)\n", pid,
	       strerror (errno), errno);
      fflush (stderr);
      _exit (0177);
    }

  proc = add_process (pid, 1);
  proc->tdesc = tdesc_spu;
  ptid = ptid_build (pid, pid, 0);
  add_thread (ptid, NULL);
  return 0;
}

/* Kill the inferior process.  */
static int
spu_kill (int pid)
{
  int status, ret;
  struct process_info *process = find_process_pid (pid);
  if (process == NULL)
    return -1;

  ptrace (PTRACE_KILL, pid, 0, 0);

  do {
    ret = waitpid (pid, &status, 0);
    if (WIFEXITED (status) || WIFSIGNALED (status))
      break;
  } while (ret != -1 || errno != ECHILD);

  clear_inferiors ();
  remove_process (process);
  return 0;
}

/* Detach from inferior process.  */
static int
spu_detach (int pid)
{
  struct process_info *process = find_process_pid (pid);
  if (process == NULL)
    return -1;

  ptrace (PTRACE_DETACH, pid, 0, 0);

  clear_inferiors ();
  remove_process (process);
  return 0;
}

static void
spu_mourn (struct process_info *process)
{
  remove_process (process);
}

static void
spu_join (int pid)
{
  int status, ret;

  do {
    ret = waitpid (pid, &status, 0);
    if (WIFEXITED (status) || WIFSIGNALED (status))
      break;
  } while (ret != -1 || errno != ECHILD);
}

/* Return nonzero if the given thread is still alive.  */
static int
spu_thread_alive (ptid_t ptid)
{
  return ptid_equal (ptid, current_ptid);
}

/* Resume process.  */
static void
spu_resume (struct thread_resume *resume_info, size_t n)
{
  struct thread_info *thr = get_first_thread ();
  size_t i;

  for (i = 0; i < n; i++)
    if (ptid_equal (resume_info[i].thread, minus_one_ptid)
	|| ptid_equal (resume_info[i].thread, ptid_of (thr)))
      break;

  if (i == n)
    return;

  /* We don't support hardware single-stepping right now, assume
     GDB knows to use software single-stepping.  */
  if (resume_info[i].kind == resume_step)
    fprintf (stderr, "Hardware single-step not supported.\n");

  regcache_invalidate ();

  errno = 0;
  ptrace (PTRACE_CONT, ptid_get_lwp (ptid_of (thr)), 0, resume_info[i].sig);
  if (errno)
    perror_with_name ("ptrace");
}

/* Wait for process, returns status.  */
static ptid_t
spu_wait (ptid_t ptid, struct target_waitstatus *ourstatus, int options)
{
  int pid = ptid_get_pid (ptid);
  int w;
  int ret;

  while (1)
    {
      ret = waitpid (pid, &w, WNOHANG | __WALL | __WNOTHREAD);

      if (ret == -1)
	{
	  if (errno != ECHILD)
	    perror_with_name ("waitpid");
	}
      else if (ret > 0)
	break;

      usleep (1000);
    }

  /* On the first wait, continue running the inferior until we are
     blocked inside an spu_run system call.  */
  if (!server_waiting)
    {
      int fd;
      CORE_ADDR addr;

      while (!parse_spufs_run (&fd, &addr))
	{
	  ptrace (PT_SYSCALL, pid, (PTRACE_TYPE_ARG3) 0, 0);
	  waitpid (pid, NULL, __WALL | __WNOTHREAD);
	}
    }

  if (WIFEXITED (w))
    {
      fprintf (stderr, "\nChild exited with retcode = %x \n", WEXITSTATUS (w));
      ourstatus->kind =  TARGET_WAITKIND_EXITED;
      ourstatus->value.integer = WEXITSTATUS (w);
      clear_inferiors ();
      return pid_to_ptid (ret);
    }
  else if (!WIFSTOPPED (w))
    {
      fprintf (stderr, "\nChild terminated with signal = %x \n", WTERMSIG (w));
      ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
      ourstatus->value.sig = gdb_signal_from_host (WTERMSIG (w));
      clear_inferiors ();
      return pid_to_ptid (ret);
    }

  /* After attach, we may have received a SIGSTOP.  Do not return this
     as signal to GDB, or else it will try to continue with SIGSTOP ...  */
  if (!server_waiting)
    {
      ourstatus->kind = TARGET_WAITKIND_STOPPED;
      ourstatus->value.sig = GDB_SIGNAL_0;
      return ptid_build (ret, ret, 0);
    }

  ourstatus->kind = TARGET_WAITKIND_STOPPED;
  ourstatus->value.sig = gdb_signal_from_host (WSTOPSIG (w));
  return ptid_build (ret, ret, 0);
}

/* Fetch inferior registers.  */
static void
spu_fetch_registers (struct regcache *regcache, int regno)
{
  int fd;
  CORE_ADDR addr;

  /* We must be stopped on a spu_run system call.  */
  if (!parse_spufs_run (&fd, &addr))
    return;

  /* The ID register holds the spufs file handle.  */
  if (regno == -1 || regno == SPU_ID_REGNUM)
    supply_register (regcache, SPU_ID_REGNUM, (char *)&fd);

  /* The NPC register is found at ADDR.  */
  if (regno == -1 || regno == SPU_PC_REGNUM)
    {
      char buf[4];
      if (fetch_ppc_memory (addr, buf, 4) == 0)
	supply_register (regcache, SPU_PC_REGNUM, buf);
    }

  /* The GPRs are found in the "regs" spufs file.  */
  if (regno == -1 || (regno >= 0 && regno < SPU_NUM_CORE_REGS))
    {
      unsigned char buf[16*SPU_NUM_CORE_REGS];
      char annex[32];
      int i;

      sprintf (annex, "%d/regs", fd);
      if (spu_proc_xfer_spu (annex, buf, NULL, 0, sizeof buf) == sizeof buf)
	for (i = 0; i < SPU_NUM_CORE_REGS; i++)
	  supply_register (regcache, i, buf + i*16);
    }
}

/* Store inferior registers.  */
static void
spu_store_registers (struct regcache *regcache, int regno)
{
  int fd;
  CORE_ADDR addr;

  /* ??? Some callers use 0 to mean all registers.  */
  if (regno == 0)
    regno = -1;

  /* We must be stopped on a spu_run system call.  */
  if (!parse_spufs_run (&fd, &addr))
    return;

  /* The NPC register is found at ADDR.  */
  if (regno == -1 || regno == SPU_PC_REGNUM)
    {
      char buf[4];
      collect_register (regcache, SPU_PC_REGNUM, buf);
      store_ppc_memory (addr, buf, 4);
    }

  /* The GPRs are found in the "regs" spufs file.  */
  if (regno == -1 || (regno >= 0 && regno < SPU_NUM_CORE_REGS))
    {
      unsigned char buf[16*SPU_NUM_CORE_REGS];
      char annex[32];
      int i;

      for (i = 0; i < SPU_NUM_CORE_REGS; i++)
	collect_register (regcache, i, buf + i*16);

      sprintf (annex, "%d/regs", fd);
      spu_proc_xfer_spu (annex, NULL, buf, 0, sizeof buf);
    }
}

/* Copy LEN bytes from inferior's memory starting at MEMADDR
   to debugger memory starting at MYADDR.  */
static int
spu_read_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  int fd, ret;
  CORE_ADDR addr;
  char annex[32], lslr_annex[32], buf[32];
  CORE_ADDR lslr;

  /* We must be stopped on a spu_run system call.  */
  if (!parse_spufs_run (&fd, &addr))
    return 0;

  /* Use the "mem" spufs file to access SPU local store.  */
  sprintf (annex, "%d/mem", fd);
  ret = spu_proc_xfer_spu (annex, myaddr, NULL, memaddr, len);
  if (ret > 0)
    return ret == len ? 0 : EIO;

  /* SPU local store access wraps the address around at the
     local store limit.  We emulate this here.  To avoid needing
     an extra access to retrieve the LSLR, we only do that after
     trying the original address first, and getting end-of-file.  */
  sprintf (lslr_annex, "%d/lslr", fd);
  memset (buf, 0, sizeof buf);
  if (spu_proc_xfer_spu (lslr_annex, (unsigned char *)buf, NULL,
			 0, sizeof buf) <= 0)
    return ret;

  lslr = strtoul (buf, NULL, 16);
  ret = spu_proc_xfer_spu (annex, myaddr, NULL, memaddr & lslr, len);

  return ret == len ? 0 : EIO;
}

/* Copy LEN bytes of data from debugger memory at MYADDR
   to inferior's memory at MEMADDR.
   On failure (cannot write the inferior)
   returns the value of errno.  */
static int
spu_write_memory (CORE_ADDR memaddr, const unsigned char *myaddr, int len)
{
  int fd, ret;
  CORE_ADDR addr;
  char annex[32], lslr_annex[32], buf[32];
  CORE_ADDR lslr;

  /* We must be stopped on a spu_run system call.  */
  if (!parse_spufs_run (&fd, &addr))
    return 0;

  /* Use the "mem" spufs file to access SPU local store.  */
  sprintf (annex, "%d/mem", fd);
  ret = spu_proc_xfer_spu (annex, NULL, myaddr, memaddr, len);
  if (ret > 0)
    return ret == len ? 0 : EIO;

  /* SPU local store access wraps the address around at the
     local store limit.  We emulate this here.  To avoid needing
     an extra access to retrieve the LSLR, we only do that after
     trying the original address first, and getting end-of-file.  */
  sprintf (lslr_annex, "%d/lslr", fd);
  memset (buf, 0, sizeof buf);
  if (spu_proc_xfer_spu (lslr_annex, (unsigned char *)buf, NULL,
			 0, sizeof buf) <= 0)
    return ret;

  lslr = strtoul (buf, NULL, 16);
  ret = spu_proc_xfer_spu (annex, NULL, myaddr, memaddr & lslr, len);

  return ret == len ? 0 : EIO;
}

/* Look up special symbols -- unneded here.  */
static void
spu_look_up_symbols (void)
{
}

/* Send signal to inferior.  */
static void
spu_request_interrupt (void)
{
  struct thread_info *thr = get_first_thread ();

  syscall (SYS_tkill, lwpid_of (thr), SIGINT);
}

/* Implementation of the target_ops method "sw_breakpoint_from_kind".  */

static const gdb_byte *
spu_sw_breakpoint_from_kind (int kind, int *size)
{
  *size = sizeof breakpoint;
  return breakpoint;
}

static struct target_ops spu_target_ops = {
  spu_create_inferior,
  NULL,  /* post_create_inferior */
  spu_attach,
  spu_kill,
  spu_detach,
  spu_mourn,
  spu_join,
  spu_thread_alive,
  spu_resume,
  spu_wait,
  spu_fetch_registers,
  spu_store_registers,
  NULL, /* prepare_to_access_memory */
  NULL, /* done_accessing_memory */
  spu_read_memory,
  spu_write_memory,
  spu_look_up_symbols,
  spu_request_interrupt,
  NULL,
  NULL,  /* supports_z_point_type */
  NULL,
  NULL,
  NULL, /* stopped_by_sw_breakpoint */
  NULL, /* supports_stopped_by_sw_breakpoint */
  NULL, /* stopped_by_hw_breakpoint */
  NULL, /* supports_stopped_by_hw_breakpoint */
  NULL, /* supports_hardware_single_step */
  NULL,
  NULL,
  NULL,
  NULL,
  spu_proc_xfer_spu,
  hostio_last_error_from_errno,
  NULL, /* qxfer_osdata */
  NULL, /* qxfer_siginfo */
  NULL, /* supports_non_stop */
  NULL, /* async */
  NULL, /* start_non_stop */
  NULL, /* supports_multi_process */
  NULL, /* supports_fork_events */
  NULL, /* supports_vfork_events */
  NULL, /* supports_exec_events */
  NULL, /* handle_new_gdb_connection */
  NULL, /* handle_monitor_command */
  NULL, /* core_of_thread */
  NULL, /* read_loadmap */
  NULL, /* process_qsupported */
  NULL, /* supports_tracepoints */
  NULL, /* read_pc */
  NULL, /* write_pc */
  NULL, /* thread_stopped */
  NULL, /* get_tib_address */
  NULL, /* pause_all */
  NULL, /* unpause_all */
  NULL, /* stabilize_threads */
  NULL, /* install_fast_tracepoint_jump_pad */
  NULL, /* emit_ops */
  NULL, /* supports_disable_randomization */
  NULL, /* get_min_fast_tracepoint_insn_len */
  NULL, /* qxfer_libraries_svr4 */
  NULL, /* support_agent */
  NULL, /* support_btrace */
  NULL, /* enable_btrace */
  NULL, /* disable_btrace */
  NULL, /* read_btrace */
  NULL, /* read_btrace_conf */
  NULL, /* supports_range_stepping */
  NULL, /* pid_to_exec_file */
  NULL, /* multifs_open */
  NULL, /* multifs_unlink */
  NULL, /* multifs_readlink */
  NULL, /* breakpoint_kind_from_pc */
  spu_sw_breakpoint_from_kind,
};

void
initialize_low (void)
{
  set_target_ops (&spu_target_ops);
  init_registers_spu ();
}
