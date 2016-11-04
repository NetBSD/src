/* IBM RS/6000 native-dependent code for GDB, the GNU debugger.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

#include "defs.h"
#include "inferior.h"
#include "target.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"
#include "bfd.h"
#include "gdb-stabs.h"
#include "regcache.h"
#include "arch-utils.h"
#include "inf-child.h"
#include "inf-ptrace.h"
#include "ppc-tdep.h"
#include "rs6000-tdep.h"
#include "rs6000-aix-tdep.h"
#include "exec.h"
#include "observer.h"
#include "xcoffread.h"

#include <sys/ptrace.h>
#include <sys/reg.h>

#include <sys/dir.h>
#include <sys/user.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <a.out.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "gdb_bfd.h"
#include <sys/core.h>
#define __LDINFO_PTRACE32__	/* for __ld_info32 */
#define __LDINFO_PTRACE64__	/* for __ld_info64 */
#include <sys/ldr.h>
#include <sys/systemcfg.h>

/* On AIX4.3+, sys/ldr.h provides different versions of struct ld_info for
   debugging 32-bit and 64-bit processes.  Define a typedef and macros for
   accessing fields in the appropriate structures.  */

/* In 32-bit compilation mode (which is the only mode from which ptrace()
   works on 4.3), __ld_info32 is #defined as equivalent to ld_info.  */

#if defined (__ld_info32) || defined (__ld_info64)
# define ARCH3264
#endif

/* Return whether the current architecture is 64-bit.  */

#ifndef ARCH3264
# define ARCH64() 0
#else
# define ARCH64() (register_size (target_gdbarch (), 0) == 8)
#endif

static target_xfer_partial_ftype rs6000_xfer_shared_libraries;

/* Given REGNO, a gdb register number, return the corresponding
   number suitable for use as a ptrace() parameter.  Return -1 if
   there's no suitable mapping.  Also, set the int pointed to by
   ISFLOAT to indicate whether REGNO is a floating point register.  */

static int
regmap (struct gdbarch *gdbarch, int regno, int *isfloat)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  *isfloat = 0;
  if (tdep->ppc_gp0_regnum <= regno
      && regno < tdep->ppc_gp0_regnum + ppc_num_gprs)
    return regno;
  else if (tdep->ppc_fp0_regnum >= 0
           && tdep->ppc_fp0_regnum <= regno
           && regno < tdep->ppc_fp0_regnum + ppc_num_fprs)
    {
      *isfloat = 1;
      return regno - tdep->ppc_fp0_regnum + FPR0;
    }
  else if (regno == gdbarch_pc_regnum (gdbarch))
    return IAR;
  else if (regno == tdep->ppc_ps_regnum)
    return MSR;
  else if (regno == tdep->ppc_cr_regnum)
    return CR;
  else if (regno == tdep->ppc_lr_regnum)
    return LR;
  else if (regno == tdep->ppc_ctr_regnum)
    return CTR;
  else if (regno == tdep->ppc_xer_regnum)
    return XER;
  else if (tdep->ppc_fpscr_regnum >= 0
           && regno == tdep->ppc_fpscr_regnum)
    return FPSCR;
  else if (tdep->ppc_mq_regnum >= 0 && regno == tdep->ppc_mq_regnum)
    return MQ;
  else
    return -1;
}

/* Call ptrace(REQ, ID, ADDR, DATA, BUF).  */

static int
rs6000_ptrace32 (int req, int id, int *addr, int data, int *buf)
{
#ifdef HAVE_PTRACE64
  int ret = ptrace64 (req, id, (uintptr_t) addr, data, buf);
#else
  int ret = ptrace (req, id, (int *)addr, data, buf);
#endif
#if 0
  printf ("rs6000_ptrace32 (%d, %d, 0x%x, %08x, 0x%x) = 0x%x\n",
	  req, id, (unsigned int)addr, data, (unsigned int)buf, ret);
#endif
  return ret;
}

/* Call ptracex(REQ, ID, ADDR, DATA, BUF).  */

static int
rs6000_ptrace64 (int req, int id, long long addr, int data, void *buf)
{
#ifdef ARCH3264
#  ifdef HAVE_PTRACE64
  int ret = ptrace64 (req, id, addr, data, (PTRACE_TYPE_ARG5) buf);
#  else
  int ret = ptracex (req, id, addr, data, (PTRACE_TYPE_ARG5) buf);
#  endif
#else
  int ret = 0;
#endif
#if 0
  printf ("rs6000_ptrace64 (%d, %d, %s, %08x, 0x%x) = 0x%x\n",
	  req, id, hex_string (addr), data, (unsigned int)buf, ret);
#endif
  return ret;
}

/* Fetch register REGNO from the inferior.  */

static void
fetch_register (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int addr[MAX_REGISTER_SIZE];
  int nr, isfloat;

  /* Retrieved values may be -1, so infer errors from errno.  */
  errno = 0;

  nr = regmap (gdbarch, regno, &isfloat);

  /* Floating-point registers.  */
  if (isfloat)
    rs6000_ptrace32 (PT_READ_FPR, ptid_get_pid (inferior_ptid), addr, nr, 0);

  /* Bogus register number.  */
  else if (nr < 0)
    {
      if (regno >= gdbarch_num_regs (gdbarch))
	fprintf_unfiltered (gdb_stderr,
			    "gdb error: register no %d not implemented.\n",
			    regno);
      return;
    }

  /* Fixed-point registers.  */
  else
    {
      if (!ARCH64 ())
	*addr = rs6000_ptrace32 (PT_READ_GPR, ptid_get_pid (inferior_ptid),
				 (int *) nr, 0, 0);
      else
	{
	  /* PT_READ_GPR requires the buffer parameter to point to long long,
	     even if the register is really only 32 bits.  */
	  long long buf;
	  rs6000_ptrace64 (PT_READ_GPR, ptid_get_pid (inferior_ptid),
			   nr, 0, &buf);
	  if (register_size (gdbarch, regno) == 8)
	    memcpy (addr, &buf, 8);
	  else
	    *addr = buf;
	}
    }

  if (!errno)
    regcache_raw_supply (regcache, regno, (char *) addr);
  else
    {
#if 0
      /* FIXME: this happens 3 times at the start of each 64-bit program.  */
      perror (_("ptrace read"));
#endif
      errno = 0;
    }
}

/* Store register REGNO back into the inferior.  */

static void
store_register (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int addr[MAX_REGISTER_SIZE];
  int nr, isfloat;

  /* Fetch the register's value from the register cache.  */
  regcache_raw_collect (regcache, regno, addr);

  /* -1 can be a successful return value, so infer errors from errno.  */
  errno = 0;

  nr = regmap (gdbarch, regno, &isfloat);

  /* Floating-point registers.  */
  if (isfloat)
    rs6000_ptrace32 (PT_WRITE_FPR, ptid_get_pid (inferior_ptid), addr, nr, 0);

  /* Bogus register number.  */
  else if (nr < 0)
    {
      if (regno >= gdbarch_num_regs (gdbarch))
	fprintf_unfiltered (gdb_stderr,
			    "gdb error: register no %d not implemented.\n",
			    regno);
    }

  /* Fixed-point registers.  */
  else
    {
      /* The PT_WRITE_GPR operation is rather odd.  For 32-bit inferiors,
         the register's value is passed by value, but for 64-bit inferiors,
	 the address of a buffer containing the value is passed.  */
      if (!ARCH64 ())
	rs6000_ptrace32 (PT_WRITE_GPR, ptid_get_pid (inferior_ptid),
			 (int *) nr, *addr, 0);
      else
	{
	  /* PT_WRITE_GPR requires the buffer parameter to point to an 8-byte
	     area, even if the register is really only 32 bits.  */
	  long long buf;
	  if (register_size (gdbarch, regno) == 8)
	    memcpy (&buf, addr, 8);
	  else
	    buf = *addr;
	  rs6000_ptrace64 (PT_WRITE_GPR, ptid_get_pid (inferior_ptid),
			   nr, 0, &buf);
	}
    }

  if (errno)
    {
      perror (_("ptrace write"));
      errno = 0;
    }
}

/* Read from the inferior all registers if REGNO == -1 and just register
   REGNO otherwise.  */

static void
rs6000_fetch_inferior_registers (struct target_ops *ops,
				 struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (regno != -1)
    fetch_register (regcache, regno);

  else
    {
      struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

      /* Read 32 general purpose registers.  */
      for (regno = tdep->ppc_gp0_regnum;
           regno < tdep->ppc_gp0_regnum + ppc_num_gprs;
	   regno++)
	{
	  fetch_register (regcache, regno);
	}

      /* Read general purpose floating point registers.  */
      if (tdep->ppc_fp0_regnum >= 0)
        for (regno = 0; regno < ppc_num_fprs; regno++)
          fetch_register (regcache, tdep->ppc_fp0_regnum + regno);

      /* Read special registers.  */
      fetch_register (regcache, gdbarch_pc_regnum (gdbarch));
      fetch_register (regcache, tdep->ppc_ps_regnum);
      fetch_register (regcache, tdep->ppc_cr_regnum);
      fetch_register (regcache, tdep->ppc_lr_regnum);
      fetch_register (regcache, tdep->ppc_ctr_regnum);
      fetch_register (regcache, tdep->ppc_xer_regnum);
      if (tdep->ppc_fpscr_regnum >= 0)
        fetch_register (regcache, tdep->ppc_fpscr_regnum);
      if (tdep->ppc_mq_regnum >= 0)
	fetch_register (regcache, tdep->ppc_mq_regnum);
    }
}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

static void
rs6000_store_inferior_registers (struct target_ops *ops,
				 struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (regno != -1)
    store_register (regcache, regno);

  else
    {
      struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

      /* Write general purpose registers first.  */
      for (regno = tdep->ppc_gp0_regnum;
           regno < tdep->ppc_gp0_regnum + ppc_num_gprs;
	   regno++)
	{
	  store_register (regcache, regno);
	}

      /* Write floating point registers.  */
      if (tdep->ppc_fp0_regnum >= 0)
        for (regno = 0; regno < ppc_num_fprs; regno++)
          store_register (regcache, tdep->ppc_fp0_regnum + regno);

      /* Write special registers.  */
      store_register (regcache, gdbarch_pc_regnum (gdbarch));
      store_register (regcache, tdep->ppc_ps_regnum);
      store_register (regcache, tdep->ppc_cr_regnum);
      store_register (regcache, tdep->ppc_lr_regnum);
      store_register (regcache, tdep->ppc_ctr_regnum);
      store_register (regcache, tdep->ppc_xer_regnum);
      if (tdep->ppc_fpscr_regnum >= 0)
        store_register (regcache, tdep->ppc_fpscr_regnum);
      if (tdep->ppc_mq_regnum >= 0)
	store_register (regcache, tdep->ppc_mq_regnum);
    }
}

/* Implement the to_xfer_partial target_ops method.  */

static enum target_xfer_status
rs6000_xfer_partial (struct target_ops *ops, enum target_object object,
		     const char *annex, gdb_byte *readbuf,
		     const gdb_byte *writebuf,
		     ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  pid_t pid = ptid_get_pid (inferior_ptid);
  int arch64 = ARCH64 ();

  switch (object)
    {
    case TARGET_OBJECT_LIBRARIES_AIX:
      return rs6000_xfer_shared_libraries (ops, object, annex,
					   readbuf, writebuf,
					   offset, len, xfered_len);
    case TARGET_OBJECT_MEMORY:
      {
	union
	{
	  PTRACE_TYPE_RET word;
	  gdb_byte byte[sizeof (PTRACE_TYPE_RET)];
	} buffer;
	ULONGEST rounded_offset;
	LONGEST partial_len;

	/* Round the start offset down to the next long word
	   boundary.  */
	rounded_offset = offset & -(ULONGEST) sizeof (PTRACE_TYPE_RET);

	/* Since ptrace will transfer a single word starting at that
	   rounded_offset the partial_len needs to be adjusted down to
	   that (remember this function only does a single transfer).
	   Should the required length be even less, adjust it down
	   again.  */
	partial_len = (rounded_offset + sizeof (PTRACE_TYPE_RET)) - offset;
	if (partial_len > len)
	  partial_len = len;

	if (writebuf)
	  {
	    /* If OFFSET:PARTIAL_LEN is smaller than
	       ROUNDED_OFFSET:WORDSIZE then a read/modify write will
	       be needed.  Read in the entire word.  */
	    if (rounded_offset < offset
		|| (offset + partial_len
		    < rounded_offset + sizeof (PTRACE_TYPE_RET)))
	      {
		/* Need part of initial word -- fetch it.  */
		if (arch64)
		  buffer.word = rs6000_ptrace64 (PT_READ_I, pid,
						 rounded_offset, 0, NULL);
		else
		  buffer.word = rs6000_ptrace32 (PT_READ_I, pid,
						 (int *) (uintptr_t)
						 rounded_offset,
						 0, NULL);
	      }

	    /* Copy data to be written over corresponding part of
	       buffer.  */
	    memcpy (buffer.byte + (offset - rounded_offset),
		    writebuf, partial_len);

	    errno = 0;
	    if (arch64)
	      rs6000_ptrace64 (PT_WRITE_D, pid,
			       rounded_offset, buffer.word, NULL);
	    else
	      rs6000_ptrace32 (PT_WRITE_D, pid,
			       (int *) (uintptr_t) rounded_offset,
			       buffer.word, NULL);
	    if (errno)
	      return TARGET_XFER_EOF;
	  }

	if (readbuf)
	  {
	    errno = 0;
	    if (arch64)
	      buffer.word = rs6000_ptrace64 (PT_READ_I, pid,
					     rounded_offset, 0, NULL);
	    else
	      buffer.word = rs6000_ptrace32 (PT_READ_I, pid,
					     (int *)(uintptr_t)rounded_offset,
					     0, NULL);
	    if (errno)
	      return TARGET_XFER_EOF;

	    /* Copy appropriate bytes out of the buffer.  */
	    memcpy (readbuf, buffer.byte + (offset - rounded_offset),
		    partial_len);
	  }

	*xfered_len = (ULONGEST) partial_len;
	return TARGET_XFER_OK;
      }

    default:
      return TARGET_XFER_E_IO;
    }
}

/* Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

static ptid_t
rs6000_wait (struct target_ops *ops,
	     ptid_t ptid, struct target_waitstatus *ourstatus, int options)
{
  pid_t pid;
  int status, save_errno;

  do
    {
      set_sigint_trap ();

      do
	{
	  pid = waitpid (ptid_get_pid (ptid), &status, 0);
	  save_errno = errno;
	}
      while (pid == -1 && errno == EINTR);

      clear_sigint_trap ();

      if (pid == -1)
	{
	  fprintf_unfiltered (gdb_stderr,
			      _("Child process unexpectedly missing: %s.\n"),
			      safe_strerror (save_errno));

	  /* Claim it exited with unknown signal.  */
	  ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
	  ourstatus->value.sig = GDB_SIGNAL_UNKNOWN;
	  return inferior_ptid;
	}

      /* Ignore terminated detached child processes.  */
      if (!WIFSTOPPED (status) && pid != ptid_get_pid (inferior_ptid))
	pid = -1;
    }
  while (pid == -1);

  /* AIX has a couple of strange returns from wait().  */

  /* stop after load" status.  */
  if (status == 0x57c)
    ourstatus->kind = TARGET_WAITKIND_LOADED;
  /* signal 0.  I have no idea why wait(2) returns with this status word.  */
  else if (status == 0x7f)
    ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
  /* A normal waitstatus.  Let the usual macros deal with it.  */
  else
    store_waitstatus (ourstatus, status);

  return pid_to_ptid (pid);
}


/* Set the current architecture from the host running GDB.  Called when
   starting a child process.  */

static void (*super_create_inferior) (struct target_ops *,char *exec_file, 
				      char *allargs, char **env, int from_tty);
static void
rs6000_create_inferior (struct target_ops * ops, char *exec_file,
			char *allargs, char **env, int from_tty)
{
  enum bfd_architecture arch;
  unsigned long mach;
  bfd abfd;
  struct gdbarch_info info;

  super_create_inferior (ops, exec_file, allargs, env, from_tty);

  if (__power_rs ())
    {
      arch = bfd_arch_rs6000;
      mach = bfd_mach_rs6k;
    }
  else
    {
      arch = bfd_arch_powerpc;
      mach = bfd_mach_ppc;
    }

  /* FIXME: schauer/2002-02-25:
     We don't know if we are executing a 32 or 64 bit executable,
     and have no way to pass the proper word size to rs6000_gdbarch_init.
     So we have to avoid switching to a new architecture, if the architecture
     matches already.
     Blindly calling rs6000_gdbarch_init used to work in older versions of
     GDB, as rs6000_gdbarch_init incorrectly used the previous tdep to
     determine the wordsize.  */
  if (exec_bfd)
    {
      const struct bfd_arch_info *exec_bfd_arch_info;

      exec_bfd_arch_info = bfd_get_arch_info (exec_bfd);
      if (arch == exec_bfd_arch_info->arch)
	return;
    }

  bfd_default_set_arch_mach (&abfd, arch, mach);

  gdbarch_info_init (&info);
  info.bfd_arch_info = bfd_get_arch_info (&abfd);
  info.abfd = exec_bfd;

  if (!gdbarch_update_p (info))
    internal_error (__FILE__, __LINE__,
		    _("rs6000_create_inferior: failed "
		      "to select architecture"));
}


/* Shared Object support.  */

/* Return the LdInfo data for the given process.  Raises an error
   if the data could not be obtained.

   The returned value must be deallocated after use.  */

static gdb_byte *
rs6000_ptrace_ldinfo (ptid_t ptid)
{
  const int pid = ptid_get_pid (ptid);
  int ldi_size = 1024;
  void *ldi = xmalloc (ldi_size);
  int rc = -1;

  while (1)
    {
      if (ARCH64 ())
	rc = rs6000_ptrace64 (PT_LDINFO, pid, (unsigned long) ldi, ldi_size,
			      NULL);
      else
	rc = rs6000_ptrace32 (PT_LDINFO, pid, (int *) ldi, ldi_size, NULL);

      if (rc != -1)
	break; /* Success, we got the entire ld_info data.  */

      if (errno != ENOMEM)
	perror_with_name (_("ptrace ldinfo"));

      /* ldi is not big enough.  Double it and try again.  */
      ldi_size *= 2;
      ldi = xrealloc (ldi, ldi_size);
    }

  return (gdb_byte *) ldi;
}

/* Implement the to_xfer_partial target_ops method for
   TARGET_OBJECT_LIBRARIES_AIX objects.  */

static enum target_xfer_status
rs6000_xfer_shared_libraries
  (struct target_ops *ops, enum target_object object,
   const char *annex, gdb_byte *readbuf, const gdb_byte *writebuf,
   ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  gdb_byte *ldi_buf;
  ULONGEST result;
  struct cleanup *cleanup;

  /* This function assumes that it is being run with a live process.
     Core files are handled via gdbarch.  */
  gdb_assert (target_has_execution);

  if (writebuf)
    return TARGET_XFER_E_IO;

  ldi_buf = rs6000_ptrace_ldinfo (inferior_ptid);
  gdb_assert (ldi_buf != NULL);
  cleanup = make_cleanup (xfree, ldi_buf);
  result = rs6000_aix_ld_info_to_xml (target_gdbarch (), ldi_buf,
				      readbuf, offset, len, 1);
  xfree (ldi_buf);

  do_cleanups (cleanup);

  if (result == 0)
    return TARGET_XFER_EOF;
  else
    {
      *xfered_len = result;
      return TARGET_XFER_OK;
    }
}

void _initialize_rs6000_nat (void);

void
_initialize_rs6000_nat (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  t->to_fetch_registers = rs6000_fetch_inferior_registers;
  t->to_store_registers = rs6000_store_inferior_registers;
  t->to_xfer_partial = rs6000_xfer_partial;

  super_create_inferior = t->to_create_inferior;
  t->to_create_inferior = rs6000_create_inferior;

  t->to_wait = rs6000_wait;

  add_target (t);
}
