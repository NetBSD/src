/* Native-dependent code for Linux running on i386's, for GDB.
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "inferior.h"
#include "gdbcore.h"

/* For i386_linux_skip_solib_resolver.  */
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"

#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/procfs.h>

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

/* On Linux, threads are implemented as pseudo-processes, in which
   case we may be tracing more than one process at a time.  In that
   case, inferior_pid will contain the main process ID and the
   individual thread (process) ID mashed together.  These macros are
   used to separate them out.  These definitions should be overridden
   if thread support is included.  */

#if !defined (PIDGET)	/* Default definition for PIDGET/TIDGET.  */
#define PIDGET(PID)	PID
#define TIDGET(PID)	0
#endif


/* The register sets used in Linux ELF core-dumps are identical to the
   register sets in `struct user' that is used for a.out core-dumps,
   and is also used by `ptrace'.  The corresponding types are
   `elf_gregset_t' for the general-purpose registers (with
   `elf_greg_t' the type of a single GP register) and `elf_fpregset_t'
   for the floating-point registers.

   Those types used to be available under the names `gregset_t' and
   `fpregset_t' too, and this file used those names in the past.  But
   those names are now used for the register sets used in the
   `mcontext_t' type, and have a different size and layout.  */

/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register array layout.  */
static int regmap[] = 
{
  EAX, ECX, EDX, EBX,
  UESP, EBP, ESI, EDI,
  EIP, EFL, CS, SS,
  DS, ES, FS, GS
};

/* Which ptrace request retrieves which registers?
   These apply to the corresponding SET requests as well.  */
#define GETREGS_SUPPLIES(regno) \
  (0 <= (regno) && (regno) <= 15)
#define GETFPREGS_SUPPLIES(regno) \
  (FP0_REGNUM <= (regno) && (regno) <= LAST_FPU_CTRL_REGNUM)
#define GETXFPREGS_SUPPLIES(regno) \
  (FP0_REGNUM <= (regno) && (regno) <= MXCSR_REGNUM)

/* Does the current host support the GETREGS request?  */
int have_ptrace_getregs =
#ifdef HAVE_PTRACE_GETREGS
  1
#else
  0
#endif
;

/* Does the current host support the GETXFPREGS request?  The header
   file may or may not define it, and even if it is defined, the
   kernel will return EIO if it's running on a pre-SSE processor.

   PTRACE_GETXFPREGS is a Cygnus invention, since we wrote our own
   Linux kernel patch for SSE support.  That patch may or may not
   actually make it into the official distribution.  If you find that
   years have gone by since this stuff was added, and Linux isn't
   using PTRACE_GETXFPREGS, that means that our patch didn't make it,
   and you can delete this, and the related code.

   My instinct is to attach this to some architecture- or
   target-specific data structure, but really, a particular GDB
   process can only run on top of one kernel at a time.  So it's okay
   for this to be a simple variable.  */
int have_ptrace_getxfpregs =
#ifdef HAVE_PTRACE_GETXFPREGS
  1
#else
  0
#endif
;


/* Fetching registers directly from the U area, one at a time.  */

/* FIXME: kettenis/2000-03-05: This duplicates code from `inptrace.c'.
   The problem is that we define FETCH_INFERIOR_REGISTERS since we
   want to use our own versions of {fetch,store}_inferior_registers
   that use the GETREGS request.  This means that the code in
   `infptrace.c' is #ifdef'd out.  But we need to fall back on that
   code when GDB is running on top of a kernel that doesn't support
   the GETREGS request.  I want to avoid changing `infptrace.c' right
   now.  */

#ifndef PT_READ_U
#define PT_READ_U PTRACE_PEEKUSR
#endif
#ifndef PT_WRITE_U
#define PT_WRITE_U PTRACE_POKEUSR
#endif

/* Default the type of the ptrace transfer to int.  */
#ifndef PTRACE_XFER_TYPE
#define PTRACE_XFER_TYPE int
#endif

/* Registers we shouldn't try to fetch.  */
#if !defined (CANNOT_FETCH_REGISTER)
#define CANNOT_FETCH_REGISTER(regno) 0
#endif

/* Fetch one register.  */

static void
fetch_register (regno)
     int regno;
{
  /* This isn't really an address.  But ptrace thinks of it as one.  */
  CORE_ADDR regaddr;
  char mess[128];		/* For messages */
  register int i;
  unsigned int offset;		/* Offset of registers within the u area.  */
  char buf[MAX_REGISTER_RAW_SIZE];
  int tid;

  if (CANNOT_FETCH_REGISTER (regno))
    {
      memset (buf, '\0', REGISTER_RAW_SIZE (regno));	/* Supply zeroes */
      supply_register (regno, buf);
      return;
    }

  /* Overload thread id onto process id */
  if ((tid = TIDGET (inferior_pid)) == 0)
    tid = inferior_pid;		/* no thread id, just use process id */

  offset = U_REGS_OFFSET;

  regaddr = register_addr (regno, offset);
  for (i = 0; i < REGISTER_RAW_SIZE (regno); i += sizeof (PTRACE_XFER_TYPE))
    {
      errno = 0;
      *(PTRACE_XFER_TYPE *) & buf[i] = ptrace (PT_READ_U, tid,
					       (PTRACE_ARG3_TYPE) regaddr, 0);
      regaddr += sizeof (PTRACE_XFER_TYPE);
      if (errno != 0)
	{
	  sprintf (mess, "reading register %s (#%d)", 
		   REGISTER_NAME (regno), regno);
	  perror_with_name (mess);
	}
    }
  supply_register (regno, buf);
}

/* Fetch register values from the inferior.
   If REGNO is negative, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time). */

void
old_fetch_inferior_registers (regno)
     int regno;
{
  if (regno >= 0)
    {
      fetch_register (regno);
    }
  else
    {
      for (regno = 0; regno < ARCH_NUM_REGS; regno++)
	{
	  fetch_register (regno);
	}
    }
}

/* Registers we shouldn't try to store.  */
#if !defined (CANNOT_STORE_REGISTER)
#define CANNOT_STORE_REGISTER(regno) 0
#endif

/* Store one register. */

static void
store_register (regno)
     int regno;
{
  /* This isn't really an address.  But ptrace thinks of it as one.  */
  CORE_ADDR regaddr;
  char mess[128];		/* For messages */
  register int i;
  unsigned int offset;		/* Offset of registers within the u area.  */
  int tid;

  if (CANNOT_STORE_REGISTER (regno))
    {
      return;
    }

  /* Overload thread id onto process id */
  if ((tid = TIDGET (inferior_pid)) == 0)
    tid = inferior_pid;		/* no thread id, just use process id */

  offset = U_REGS_OFFSET;

  regaddr = register_addr (regno, offset);
  for (i = 0; i < REGISTER_RAW_SIZE (regno); i += sizeof (PTRACE_XFER_TYPE))
    {
      errno = 0;
      ptrace (PT_WRITE_U, tid, (PTRACE_ARG3_TYPE) regaddr,
	      *(PTRACE_XFER_TYPE *) & registers[REGISTER_BYTE (regno) + i]);
      regaddr += sizeof (PTRACE_XFER_TYPE);
      if (errno != 0)
	{
	  sprintf (mess, "writing register %s (#%d)", 
		   REGISTER_NAME (regno), regno);
	  perror_with_name (mess);
	}
    }
}

/* Store our register values back into the inferior.
   If REGNO is negative, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

void
old_store_inferior_registers (regno)
     int regno;
{
  if (regno >= 0)
    {
      store_register (regno);
    }
  else
    {
      for (regno = 0; regno < ARCH_NUM_REGS; regno++)
	{
	  store_register (regno);
	}
    }
}


/* Transfering the general-purpose registers between GDB, inferiors
   and core files.  */

/* Fill GDB's register array with the genereal-purpose register values
   in *GREGSETP.  */

void
supply_gregset (elf_gregset_t *gregsetp)
{
  elf_greg_t *regp = (elf_greg_t *) gregsetp;
  int regi;

  for (regi = 0; regi < NUM_GREGS; regi++)
    supply_register (regi, (char *) (regp + regmap[regi]));
}

/* Convert the valid general-purpose register values in GDB's register
   array to `struct user' format and store them in *GREGSETP.  The
   array VALID indicates which register values are valid.  If VALID is
   NULL, all registers are assumed to be valid.  */

static void
convert_to_gregset (elf_gregset_t *gregsetp, signed char *valid)
{
  elf_greg_t *regp = (elf_greg_t *) gregsetp;
  int regi;

  for (regi = 0; regi < NUM_GREGS; regi++)
    if (! valid || valid[regi])
      *(regp + regmap[regi]) = * (int *) &registers[REGISTER_BYTE (regi)];
}

/* Fill register REGNO (if it is a general-purpose register) in
   *GREGSETPS with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */
void
fill_gregset (elf_gregset_t *gregsetp, int regno)
{
  if (regno == -1)
    {
      convert_to_gregset (gregsetp, NULL);
      return;
    }

  if (GETREGS_SUPPLIES (regno))
    {
      signed char valid[NUM_GREGS];

      memset (valid, 0, sizeof (valid));
      valid[regno] = 1;

      convert_to_gregset (gregsetp, valid);
    }
}

#ifdef HAVE_PTRACE_GETREGS

/* Fetch all general-purpose registers from process/thread TID and
   store their values in GDB's register array.  */

static void
fetch_regs (int tid)
{
  elf_gregset_t regs;
  int ret;

  ret = ptrace (PTRACE_GETREGS, tid, 0, (int) &regs);
  if (ret < 0)
    {
      if (errno == EIO)
	{
	  /* The kernel we're running on doesn't support the GETREGS
             request.  Reset `have_ptrace_getregs'.  */
	  have_ptrace_getregs = 0;
	  return;
	}

      warning ("Couldn't get registers.");
      return;
    }

  supply_gregset (&regs);
}

/* Store all valid general-purpose registers in GDB's register array
   into the process/thread specified by TID.  */

static void
store_regs (int tid)
{
  elf_gregset_t regs;
  int ret;

  ret = ptrace (PTRACE_GETREGS, tid, 0, (int) &regs);
  if (ret < 0)
    {
      warning ("Couldn't get registers.");
      return;
    }

  convert_to_gregset (&regs, register_valid);

  ret = ptrace (PTRACE_SETREGS, tid, 0, (int) &regs);
  if (ret < 0)
    {
      warning ("Couldn't write registers.");
      return;
    }
}

#else

static void fetch_regs (int tid) {}
static void store_regs (int tid) {}

#endif


/* Transfering floating-point registers between GDB, inferiors and cores.  */

/* What is the address of st(N) within the floating-point register set F?  */
#define FPREG_ADDR(f, n) ((char *) &(f)->st_space + (n) * 10)

/* Fill GDB's register array with the floating-point register values in
   *FPREGSETP.  */

void 
supply_fpregset (elf_fpregset_t *fpregsetp)
{
  int reg;
  long l;

  /* Supply the floating-point registers.  */
  for (reg = 0; reg < 8; reg++)
    supply_register (FP0_REGNUM + reg, FPREG_ADDR (fpregsetp, reg));

  /* We have to mask off the reserved bits in *FPREGSETP before
     storing the values in GDB's register file.  */
#define supply(REGNO, MEMBER)                                           \
  l = fpregsetp->MEMBER & 0xffff;                                       \
  supply_register (REGNO, (char *) &l)

  supply (FCTRL_REGNUM, cwd);
  supply (FSTAT_REGNUM, swd);
  supply (FTAG_REGNUM, twd);
  supply_register (FCOFF_REGNUM, (char *) &fpregsetp->fip);
  supply (FDS_REGNUM, fos);
  supply_register (FDOFF_REGNUM, (char *) &fpregsetp->foo);

#undef supply

  /* Extract the code segment and opcode from the  "fcs" member.  */
  l = fpregsetp->fcs & 0xffff;
  supply_register (FCS_REGNUM, (char *) &l);

  l = (fpregsetp->fcs >> 16) & ((1 << 11) - 1);
  supply_register (FOP_REGNUM, (char *) &l);
}

/* Convert the valid floating-point register values in GDB's register
   array to `struct user' format and store them in *FPREGSETP.  The
   array VALID indicates which register values are valid.  If VALID is
   NULL, all registers are assumed to be valid.  */

static void
convert_to_fpregset (elf_fpregset_t *fpregsetp, signed char *valid)
{
  int reg;

  /* Fill in the floating-point registers.  */
  for (reg = 0; reg < 8; reg++)
    if (!valid || valid[reg])
      memcpy (FPREG_ADDR (fpregsetp, reg),
	      &registers[REGISTER_BYTE (FP0_REGNUM + reg)],
	      REGISTER_RAW_SIZE(FP0_REGNUM + reg));

  /* We're not supposed to touch the reserved bits in *FPREGSETP.  */

#define fill(MEMBER, REGNO)						\
  if (! valid || valid[(REGNO)])					\
    fpregsetp->MEMBER                                                   \
      = ((fpregsetp->MEMBER & ~0xffff)                                  \
         | (* (int *) &registers[REGISTER_BYTE (REGNO)] & 0xffff))

#define fill_register(MEMBER, REGNO)                                    \
  if (! valid || valid[(REGNO)])                                        \
    memcpy (&fpregsetp->MEMBER, &registers[REGISTER_BYTE (REGNO)],      \
            sizeof (fpregsetp->MEMBER))

  fill (cwd, FCTRL_REGNUM);
  fill (swd, FSTAT_REGNUM);
  fill (twd, FTAG_REGNUM);
  fill_register (fip, FCOFF_REGNUM);
  fill (foo, FDOFF_REGNUM);
  fill_register (fos, FDS_REGNUM);

#undef fill
#undef fill_register

  if (! valid || valid[FCS_REGNUM])
    fpregsetp->fcs
      = ((fpregsetp->fcs & ~0xffff)
	 | (* (int *) &registers[REGISTER_BYTE (FCS_REGNUM)] & 0xffff));

  if (! valid || valid[FOP_REGNUM])
    fpregsetp->fcs
      = ((fpregsetp->fcs & 0xffff)
	 | ((*(int *) &registers[REGISTER_BYTE (FOP_REGNUM)] & ((1 << 11) - 1))
	    << 16));
}

/* Fill register REGNO (if it is a floating-point register) in
   *FPREGSETP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_fpregset (elf_fpregset_t *fpregsetp, int regno)
{
  if (regno == -1)
    {
      convert_to_fpregset (fpregsetp, NULL);
      return;
    }

  if (GETFPREGS_SUPPLIES(regno))
    {
      signed char valid[MAX_NUM_REGS];
      
      memset (valid, 0, sizeof (valid));
      valid[regno] = 1;
	      
      convert_to_fpregset (fpregsetp, valid);
    }
}

#ifdef HAVE_PTRACE_GETREGS

/* Fetch all floating-point registers from process/thread TID and store
   thier values in GDB's register array.  */

static void
fetch_fpregs (int tid)
{
  elf_fpregset_t fpregs;
  int ret;

  ret = ptrace (PTRACE_GETFPREGS, tid, 0, (int) &fpregs);
  if (ret < 0)
    {
      warning ("Couldn't get floating point status.");
      return;
    }

  supply_fpregset (&fpregs);
}

/* Store all valid floating-point registers in GDB's register array
   into the process/thread specified by TID.  */

static void
store_fpregs (int tid)
{
  elf_fpregset_t fpregs;
  int ret;

  ret = ptrace (PTRACE_GETFPREGS, tid, 0, (int) &fpregs);
  if (ret < 0)
    {
      warning ("Couldn't get floating point status.");
      return;
    }

  convert_to_fpregset (&fpregs, register_valid);

  ret = ptrace (PTRACE_SETFPREGS, tid, 0, (int) &fpregs);
  if (ret < 0)
    {
      warning ("Couldn't write floating point status.");
      return;
    }
}

#else

static void fetch_fpregs (int tid) {}
static void store_fpregs (int tid) {}

#endif


/* Transfering floating-point and SSE registers to and from GDB.  */

/* PTRACE_GETXFPREGS is a Cygnus invention, since we wrote our own
   Linux kernel patch for SSE support.  That patch may or may not
   actually make it into the official distribution.  If you find that
   years have gone by since this code was added, and Linux isn't using
   PTRACE_GETXFPREGS, that means that our patch didn't make it, and
   you can delete this code.  */

#ifdef HAVE_PTRACE_GETXFPREGS

/* Fill GDB's register array with the floating-point and SSE register
   values in *XFPREGS.  */

static void
supply_xfpregset (struct user_xfpregs_struct *xfpregs)
{
  int reg;

  /* Supply the floating-point registers.  */
  for (reg = 0; reg < 8; reg++)
    supply_register (FP0_REGNUM + reg, (char *) &xfpregs->st_space[reg]);

  {
    supply_register (FCTRL_REGNUM, (char *) &xfpregs->cwd);
    supply_register (FSTAT_REGNUM, (char *) &xfpregs->swd);
    supply_register (FTAG_REGNUM,  (char *) &xfpregs->twd);
    supply_register (FCOFF_REGNUM, (char *) &xfpregs->fip);
    supply_register (FDS_REGNUM,   (char *) &xfpregs->fos);
    supply_register (FDOFF_REGNUM, (char *) &xfpregs->foo);
  
    /* Extract the code segment and opcode from the  "fcs" member.  */
    {
      long l;
      
      l = xfpregs->fcs & 0xffff;
      supply_register (FCS_REGNUM, (char *) &l);

      l = (xfpregs->fcs >> 16) & ((1 << 11) - 1);
      supply_register (FOP_REGNUM, (char *) &l);
    }
  }

  /* Supply the SSE registers.  */
  for (reg = 0; reg < 8; reg++)
    supply_register (XMM0_REGNUM + reg, (char *) &xfpregs->xmm_space[reg]);
  supply_register (MXCSR_REGNUM, (char *) &xfpregs->mxcsr);
}

/* Convert the valid floating-point and SSE registers in GDB's
   register array to `struct user' format and store them in *XFPREGS.
   The array VALID indicates which registers are valid.  If VALID is
   NULL, all registers are assumed to be valid.  */

static void
convert_to_xfpregset (struct user_xfpregs_struct *xfpregs,
		      signed char *valid)
{
  int reg;

  /* Fill in the floating-point registers.  */
  for (reg = 0; reg < 8; reg++)
    if (!valid || valid[reg])
      memcpy (&xfpregs->st_space[reg],
	      &registers[REGISTER_BYTE (FP0_REGNUM + reg)],
	      REGISTER_RAW_SIZE(FP0_REGNUM + reg));

#define fill(MEMBER, REGNO)						\
  if (! valid || valid[(REGNO)])					\
    memcpy (&xfpregs->MEMBER, &registers[REGISTER_BYTE (REGNO)],	\
	    sizeof (xfpregs->MEMBER))

  fill (cwd, FCTRL_REGNUM);
  fill (swd, FSTAT_REGNUM);
  fill (twd, FTAG_REGNUM);
  fill (fip, FCOFF_REGNUM);
  fill (foo, FDOFF_REGNUM);
  fill (fos, FDS_REGNUM);

#undef fill

  if (! valid || valid[FCS_REGNUM])
    xfpregs->fcs
      = ((xfpregs->fcs & ~0xffff)
	 | (* (int *) &registers[REGISTER_BYTE (FCS_REGNUM)] & 0xffff));

  if (! valid || valid[FOP_REGNUM])
    xfpregs->fcs
      = ((xfpregs->fcs & 0xffff)
	 | ((*(int *) &registers[REGISTER_BYTE (FOP_REGNUM)] & ((1 << 11) - 1))
	    << 16));

  /* Fill in the XMM registers.  */
  for (reg = 0; reg < 8; reg++)
    if (! valid || valid[reg])
      memcpy (&xfpregs->xmm_space[reg],
	      &registers[REGISTER_BYTE (XMM0_REGNUM + reg)],
	      REGISTER_RAW_SIZE (XMM0_REGNUM + reg));
}

/* Fetch all registers covered by the PTRACE_SETXFPREGS request from
   process/thread TID and store their values in GDB's register array.
   Return non-zero if successful, zero otherwise.  */

static int
fetch_xfpregs (int tid)
{
  struct user_xfpregs_struct xfpregs;
  int ret;

  if (! have_ptrace_getxfpregs) 
    return 0;

  ret = ptrace (PTRACE_GETXFPREGS, tid, 0, &xfpregs);
  if (ret == -1)
    {
      if (errno == EIO)
	{
	  have_ptrace_getxfpregs = 0;
	  return 0;
	}

      warning ("Couldn't read floating-point and SSE registers.");
      return 0;
    }

  supply_xfpregset (&xfpregs);
  return 1;
}

/* Store all valid registers in GDB's register array covered by the
   PTRACE_SETXFPREGS request into the process/thread specified by TID.
   Return non-zero if successful, zero otherwise.  */

static int
store_xfpregs (int tid)
{
  struct user_xfpregs_struct xfpregs;
  int ret;

  if (! have_ptrace_getxfpregs)
    return 0;

  ret = ptrace (PTRACE_GETXFPREGS, tid, 0, &xfpregs);
  if (ret == -1)
    {
      if (errno == EIO)
	{
	  have_ptrace_getxfpregs = 0;
	  return 0;
	}

      warning ("Couldn't read floating-point and SSE registers.");
      return 0;
    }

  convert_to_xfpregset (&xfpregs, register_valid);

  if (ptrace (PTRACE_SETXFPREGS, tid, 0, &xfpregs) < 0)
    {
      warning ("Couldn't write floating-point and SSE registers.");
      return 0;
    }

  return 1;
}

/* Fill the XMM registers in the register array with dummy values.  For
   cases where we don't have access to the XMM registers.  I think
   this is cleaner than printing a warning.  For a cleaner solution,
   we should gdbarchify the i386 family.  */

static void
dummy_sse_values (void)
{
  /* C doesn't have a syntax for NaN's, so write it out as an array of
     longs.  */
  static long dummy[4] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
  static long mxcsr = 0x1f80;
  int reg;

  for (reg = 0; reg < 8; reg++)
    supply_register (XMM0_REGNUM + reg, (char *) dummy);
  supply_register (MXCSR_REGNUM, (char *) &mxcsr);
}

#else

/* Stub versions of the above routines, for systems that don't have
   PTRACE_GETXFPREGS.  */
static int store_xfpregs (int tid) { return 0; }
static int fetch_xfpregs (int tid) { return 0; }
static void dummy_sse_values (void) {}

#endif


/* Transferring arbitrary registers between GDB and inferior.  */

/* Fetch register REGNO from the child process.  If REGNO is -1, do
   this for all registers (including the floating point and SSE
   registers).  */

void
fetch_inferior_registers (int regno)
{
  int tid;

  /* Use the old method of peeking around in `struct user' if the
     GETREGS request isn't available.  */
  if (! have_ptrace_getregs)
    {
      old_fetch_inferior_registers (regno);
      return;
    }

  /* Linux LWP ID's are process ID's.  */
  if ((tid = TIDGET (inferior_pid)) == 0)
    tid = inferior_pid;		/* Not a threaded program.  */

  /* Use the PTRACE_GETXFPREGS request whenever possible, since it
     transfers more registers in one system call, and we'll cache the
     results.  But remember that fetch_xfpregs can fail, and return
     zero.  */
  if (regno == -1)
    {
      fetch_regs (tid);

      /* The call above might reset `have_ptrace_getregs'.  */
      if (! have_ptrace_getregs)
	{
	  old_fetch_inferior_registers (-1);
	  return;
	}

      if (fetch_xfpregs (tid))
	return;
      fetch_fpregs (tid);
      return;
    }

  if (GETREGS_SUPPLIES (regno))
    {
      fetch_regs (tid);
      return;
    }

  if (GETXFPREGS_SUPPLIES (regno))
    {
      if (fetch_xfpregs (tid))
	return;

      /* Either our processor or our kernel doesn't support the SSE
	 registers, so read the FP registers in the traditional way,
	 and fill the SSE registers with dummy values.  It would be
	 more graceful to handle differences in the register set using
	 gdbarch.  Until then, this will at least make things work
	 plausibly.  */
      fetch_fpregs (tid);
      dummy_sse_values ();
      return;
    }

  internal_error ("i386-linux-nat.c (fetch_inferior_registers): "
		  "got request for bad register number %d", regno);
}

/* Store register REGNO back into the child process.  If REGNO is -1,
   do this for all registers (including the floating point and SSE
   registers).  */
void
store_inferior_registers (int regno)
{
  int tid;

  /* Use the old method of poking around in `struct user' if the
     SETREGS request isn't available.  */
  if (! have_ptrace_getregs)
    {
      old_store_inferior_registers (regno);
      return;
    }

  /* Linux LWP ID's are process ID's.  */
  if ((tid = TIDGET (inferior_pid)) == 0)
    tid = inferior_pid;		/* Not a threaded program.  */

  /* Use the PTRACE_SETXFPREGS requests whenever possibl, since it
     transfers more registers in one system call.  But remember that
     store_xfpregs can fail, and return zero.  */
  if (regno == -1)
    {
      store_regs (tid);
      if (store_xfpregs (tid))
	return;
      store_fpregs (tid);
      return;
    }

  if (GETREGS_SUPPLIES (regno))
    {
      store_regs (tid);
      return;
    }

  if (GETXFPREGS_SUPPLIES (regno))
    {
      if (store_xfpregs (tid))
	return;

      /* Either our processor or our kernel doesn't support the SSE
	 registers, so just write the FP registers in the traditional
	 way.  */
      store_fpregs (tid);
      return;
    }

  internal_error ("Got request to store bad register number %d.", regno);
}


/* Interpreting register set info found in core files.  */

/* Provide registers to GDB from a core file.

   (We can't use the generic version of this function in
   core-regset.c, because Linux has *three* different kinds of
   register set notes.  core-regset.c would have to call
   supply_xfpregset, which most platforms don't have.)

   CORE_REG_SECT points to an array of bytes, which are the contents
   of a `note' from a core file which BFD thinks might contain
   register contents.  CORE_REG_SIZE is its size.

   WHICH says which register set corelow suspects this is:
     0 --- the general-purpose register set, in elf_gregset_t format
     2 --- the floating-point register set, in elf_fpregset_t format
     3 --- the extended floating-point register set, in struct
           user_xfpregs_struct format

   REG_ADDR isn't used on Linux.  */

static void
fetch_core_registers (char *core_reg_sect, unsigned core_reg_size,
		      int which, CORE_ADDR reg_addr)
{
  elf_gregset_t gregset;
  elf_fpregset_t fpregset;

  switch (which)
    {
    case 0:
      if (core_reg_size != sizeof (gregset))
	warning ("Wrong size gregset in core file.");
      else
	{
	  memcpy (&gregset, core_reg_sect, sizeof (gregset));
	  supply_gregset (&gregset);
	}
      break;

    case 2:
      if (core_reg_size != sizeof (fpregset))
	warning ("Wrong size fpregset in core file.");
      else
	{
	  memcpy (&fpregset, core_reg_sect, sizeof (fpregset));
	  supply_fpregset (&fpregset);
	}
      break;

#ifdef HAVE_PTRACE_GETXFPREGS
      {
	struct user_xfpregs_struct xfpregset;

      case 3:
	if (core_reg_size != sizeof (xfpregset))
	  warning ("Wrong size user_xfpregs_struct in core file.");
	else
	  {
	    memcpy (&xfpregset, core_reg_sect, sizeof (xfpregset));
	    supply_xfpregset (&xfpregset);
	  }
	break;
      }
#endif

    default:
      /* We've covered all the kinds of registers we know about here,
         so this must be something we wouldn't know what to do with
         anyway.  Just ignore it.  */
      break;
    }
}


/* Calling functions in shared libraries.  */
/* FIXME: kettenis/2000-03-05: Doesn't this belong in a
   target-dependent file?  The function
   `i386_linux_skip_solib_resolver' is mentioned in
   `config/i386/tm-linux.h'.  */

/* Find the minimal symbol named NAME, and return both the minsym
   struct and its objfile.  This probably ought to be in minsym.c, but
   everything there is trying to deal with things like C++ and
   SOFUN_ADDRESS_MAYBE_TURQUOISE, ...  Since this is so simple, it may
   be considered too special-purpose for general consumption.  */

static struct minimal_symbol *
find_minsym_and_objfile (char *name, struct objfile **objfile_p)
{
  struct objfile *objfile;

  ALL_OBJFILES (objfile)
    {
      struct minimal_symbol *msym;

      ALL_OBJFILE_MSYMBOLS (objfile, msym)
	{
	  if (SYMBOL_NAME (msym)
	      && STREQ (SYMBOL_NAME (msym), name))
	    {
	      *objfile_p = objfile;
	      return msym;
	    }
	}
    }

  return 0;
}


static CORE_ADDR
skip_hurd_resolver (CORE_ADDR pc)
{
  /* The HURD dynamic linker is part of the GNU C library, so many
     GNU/Linux distributions use it.  (All ELF versions, as far as I
     know.)  An unresolved PLT entry points to "_dl_runtime_resolve",
     which calls "fixup" to patch the PLT, and then passes control to
     the function.

     We look for the symbol `_dl_runtime_resolve', and find `fixup' in
     the same objfile.  If we are at the entry point of `fixup', then
     we set a breakpoint at the return address (at the top of the
     stack), and continue.
  
     It's kind of gross to do all these checks every time we're
     called, since they don't change once the executable has gotten
     started.  But this is only a temporary hack --- upcoming versions
     of Linux will provide a portable, efficient interface for
     debugging programs that use shared libraries.  */

  struct objfile *objfile;
  struct minimal_symbol *resolver 
    = find_minsym_and_objfile ("_dl_runtime_resolve", &objfile);

  if (resolver)
    {
      struct minimal_symbol *fixup
	= lookup_minimal_symbol ("fixup", 0, objfile);

      if (fixup && SYMBOL_VALUE_ADDRESS (fixup) == pc)
	return (SAVED_PC_AFTER_CALL (get_current_frame ()));
    }

  return 0;
}      

/* See the comments for SKIP_SOLIB_RESOLVER at the top of infrun.c.
   This function:
   1) decides whether a PLT has sent us into the linker to resolve
      a function reference, and 
   2) if so, tells us where to set a temporary breakpoint that will
      trigger when the dynamic linker is done.  */

CORE_ADDR
i386_linux_skip_solib_resolver (CORE_ADDR pc)
{
  CORE_ADDR result;

  /* Plug in functions for other kinds of resolvers here.  */
  result = skip_hurd_resolver (pc);
  if (result)
    return result;

  return 0;
}


/* Register that we are able to handle Linux ELF core file formats.  */

static struct core_fns linux_elf_core_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

void
_initialize_i386_linux_nat ()
{
  add_core_fns (&linux_elf_core_fns);
}
