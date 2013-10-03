/* IBM RS/6000 native-dependent code for GDB, the GNU debugger.

   Copyright (C) 1986-2013 Free Software Foundation, Inc.

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
#include "xcoffsolib.h"
#include "symfile.h"
#include "objfiles.h"
#include "libbfd.h"		/* For bfd_default_set_arch_mach (FIXME) */
#include "bfd.h"
#include "exceptions.h"
#include "gdb-stabs.h"
#include "regcache.h"
#include "arch-utils.h"
#include "inf-child.h"
#include "inf-ptrace.h"
#include "ppc-tdep.h"
#include "rs6000-tdep.h"
#include "exec.h"
#include "observer.h"
#include "xcoffread.h"

#include <sys/ptrace.h>
#include <sys/reg.h>

#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include <a.out.h>
#include <sys/file.h>
#include "gdb_stat.h"
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

#ifdef __ld_info32
# define ARCH3264
#endif

/* Return whether the current architecture is 64-bit.  */

#ifndef ARCH3264
# define ARCH64() 0
#else
# define ARCH64() (register_size (target_gdbarch (), 0) == 8)
#endif

/* Union of 32-bit and 64-bit versions of ld_info.  */

typedef union {
#ifndef ARCH3264
  struct ld_info l32;
  struct ld_info l64;
#else
  struct __ld_info32 l32;
  struct __ld_info64 l64;
#endif
} LdInfo;

/* If compiling with 32-bit and 64-bit debugging capability (e.g. AIX 4.x),
   declare and initialize a variable named VAR suitable for use as the arch64
   parameter to the various LDI_*() macros.  */

#ifndef ARCH3264
# define ARCH64_DECL(var)
#else
# define ARCH64_DECL(var) int var = ARCH64 ()
#endif

/* Return LDI's FIELD for a 64-bit process if ARCH64 and for a 32-bit process
   otherwise.  This technique only works for FIELDs with the same data type in
   32-bit and 64-bit versions of ld_info.  */

#ifndef ARCH3264
# define LDI_FIELD(ldi, arch64, field) (ldi)->l32.ldinfo_##field
#else
# define LDI_FIELD(ldi, arch64, field) \
  (arch64 ? (ldi)->l64.ldinfo_##field : (ldi)->l32.ldinfo_##field)
#endif

/* Return various LDI fields for a 64-bit process if ARCH64 and for a 32-bit
   process otherwise.  */

#define LDI_NEXT(ldi, arch64)		LDI_FIELD(ldi, arch64, next)
#define LDI_FD(ldi, arch64)		LDI_FIELD(ldi, arch64, fd)
#define LDI_FILENAME(ldi, arch64)	LDI_FIELD(ldi, arch64, filename)

extern struct vmap *map_vmap (bfd * bf, bfd * arch);

static void vmap_exec (void);

static void vmap_ldinfo (LdInfo *);

static struct vmap *add_vmap (LdInfo *);

static int objfile_symbol_add (void *);

static void vmap_symtab (struct vmap *);

static void exec_one_dummy_insn (struct regcache *);

extern void fixup_breakpoints (CORE_ADDR low, CORE_ADDR high, CORE_ADDR delta);

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
  int ret = ptrace (req, id, (int *)addr, data, buf);
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
  int ret = ptracex (req, id, addr, data, buf);
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
    rs6000_ptrace32 (PT_READ_FPR, PIDGET (inferior_ptid), addr, nr, 0);

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
	*addr = rs6000_ptrace32 (PT_READ_GPR, PIDGET (inferior_ptid),
				 (int *) nr, 0, 0);
      else
	{
	  /* PT_READ_GPR requires the buffer parameter to point to long long,
	     even if the register is really only 32 bits.  */
	  long long buf;
	  rs6000_ptrace64 (PT_READ_GPR, PIDGET (inferior_ptid), nr, 0, &buf);
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
    rs6000_ptrace32 (PT_WRITE_FPR, PIDGET (inferior_ptid), addr, nr, 0);

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
      if (regno == gdbarch_sp_regnum (gdbarch))
	/* Execute one dummy instruction (which is a breakpoint) in inferior
	   process to give kernel a chance to do internal housekeeping.
	   Otherwise the following ptrace(2) calls will mess up user stack
	   since kernel will get confused about the bottom of the stack
	   (%sp).  */
	exec_one_dummy_insn (regcache);

      /* The PT_WRITE_GPR operation is rather odd.  For 32-bit inferiors,
         the register's value is passed by value, but for 64-bit inferiors,
	 the address of a buffer containing the value is passed.  */
      if (!ARCH64 ())
	rs6000_ptrace32 (PT_WRITE_GPR, PIDGET (inferior_ptid),
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
	  rs6000_ptrace64 (PT_WRITE_GPR, PIDGET (inferior_ptid), nr, 0, &buf);
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


/* Attempt a transfer all LEN bytes starting at OFFSET between the
   inferior's OBJECT:ANNEX space and GDB's READBUF/WRITEBUF buffer.
   Return the number of bytes actually transferred.  */

static LONGEST
rs6000_xfer_partial (struct target_ops *ops, enum target_object object,
		     const char *annex, gdb_byte *readbuf,
		     const gdb_byte *writebuf,
		     ULONGEST offset, LONGEST len)
{
  pid_t pid = ptid_get_pid (inferior_ptid);
  int arch64 = ARCH64 ();

  switch (object)
    {
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
	      return 0;
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
	      return 0;

	    /* Copy appropriate bytes out of the buffer.  */
	    memcpy (readbuf, buffer.byte + (offset - rounded_offset),
		    partial_len);
	  }

	return partial_len;
      }

    default:
      return -1;
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

/* Execute one dummy breakpoint instruction.  This way we give the kernel
   a chance to do some housekeeping and update inferior's internal data,
   including u_area.  */

static void
exec_one_dummy_insn (struct regcache *regcache)
{
#define	DUMMY_INSN_ADDR	AIX_TEXT_SEGMENT_BASE+0x200

  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int ret, status, pid;
  CORE_ADDR prev_pc;
  void *bp;

  /* We plant one dummy breakpoint into DUMMY_INSN_ADDR address.  We
     assume that this address will never be executed again by the real
     code.  */

  bp = deprecated_insert_raw_breakpoint (gdbarch, NULL, DUMMY_INSN_ADDR);

  /* You might think this could be done with a single ptrace call, and
     you'd be correct for just about every platform I've ever worked
     on.  However, rs6000-ibm-aix4.1.3 seems to have screwed this up --
     the inferior never hits the breakpoint (it's also worth noting
     powerpc-ibm-aix4.1.3 works correctly).  */
  prev_pc = regcache_read_pc (regcache);
  regcache_write_pc (regcache, DUMMY_INSN_ADDR);
  if (ARCH64 ())
    ret = rs6000_ptrace64 (PT_CONTINUE, PIDGET (inferior_ptid), 1, 0, NULL);
  else
    ret = rs6000_ptrace32 (PT_CONTINUE, PIDGET (inferior_ptid),
			   (int *) 1, 0, NULL);

  if (ret != 0)
    perror (_("pt_continue"));

  do
    {
      pid = waitpid (PIDGET (inferior_ptid), &status, 0);
    }
  while (pid != PIDGET (inferior_ptid));

  regcache_write_pc (regcache, prev_pc);
  deprecated_remove_raw_breakpoint (gdbarch, bp);
}


/* Copy information about text and data sections from LDI to VP for a 64-bit
   process if ARCH64 and for a 32-bit process otherwise.  */

static void
vmap_secs (struct vmap *vp, LdInfo *ldi, int arch64)
{
  if (arch64)
    {
      vp->tstart = (CORE_ADDR) ldi->l64.ldinfo_textorg;
      vp->tend = vp->tstart + ldi->l64.ldinfo_textsize;
      vp->dstart = (CORE_ADDR) ldi->l64.ldinfo_dataorg;
      vp->dend = vp->dstart + ldi->l64.ldinfo_datasize;
    }
  else
    {
      vp->tstart = (unsigned long) ldi->l32.ldinfo_textorg;
      vp->tend = vp->tstart + ldi->l32.ldinfo_textsize;
      vp->dstart = (unsigned long) ldi->l32.ldinfo_dataorg;
      vp->dend = vp->dstart + ldi->l32.ldinfo_datasize;
    }

  /* The run time loader maps the file header in addition to the text
     section and returns a pointer to the header in ldinfo_textorg.
     Adjust the text start address to point to the real start address
     of the text section.  */
  vp->tstart += vp->toffs;
}

/* If the .bss section's VMA is set to an address located before
   the end of the .data section, causing the two sections to overlap,
   return the overlap in bytes.  Otherwise, return zero.

   Motivation:

   The GNU linker sometimes sets the start address of the .bss session
   before the end of the .data section, making the 2 sections overlap.
   The loader appears to handle this situation gracefully, by simply
   loading the bss section right after the end of the .data section.

   This means that the .data and the .bss sections are sometimes
   no longer relocated by the same amount.  The problem is that
   the ldinfo data does not contain any information regarding
   the relocation of the .bss section, assuming that it would be
   identical to the information provided for the .data section
   (this is what would normally happen if the program was linked
   correctly).

   GDB therefore needs to detect those cases, and make the corresponding
   adjustment to the .bss section offset computed from the ldinfo data
   when necessary.  This function returns the adjustment amount  (or
   zero when no adjustment is needed).  */

static CORE_ADDR
bss_data_overlap (struct objfile *objfile)
{
  struct obj_section *osect;
  struct bfd_section *data = NULL;
  struct bfd_section *bss = NULL;

  /* First, find the .data and .bss sections.  */
  ALL_OBJFILE_OSECTIONS (objfile, osect)
    {
      if (strcmp (bfd_section_name (objfile->obfd,
				    osect->the_bfd_section),
		  ".data") == 0)
	data = osect->the_bfd_section;
      else if (strcmp (bfd_section_name (objfile->obfd,
					 osect->the_bfd_section),
		       ".bss") == 0)
	bss = osect->the_bfd_section;
    }

  /* If either section is not defined, there can be no overlap.  */
  if (data == NULL || bss == NULL)
    return 0;

  /* Assume the problem only occurs with linkers that place the .bss
     section after the .data section (the problem has only been
     observed when using the GNU linker, and the default linker
     script always places the .data and .bss sections in that order).  */
  if (bfd_section_vma (objfile->obfd, bss)
      < bfd_section_vma (objfile->obfd, data))
    return 0;

  if (bfd_section_vma (objfile->obfd, bss)
      < bfd_section_vma (objfile->obfd, data) + bfd_get_section_size (data))
    return ((bfd_section_vma (objfile->obfd, data)
	     + bfd_get_section_size (data))
	    - bfd_section_vma (objfile->obfd, bss));

  return 0;
}

/* Handle symbol translation on vmapping.  */

static void
vmap_symtab (struct vmap *vp)
{
  struct objfile *objfile;
  struct section_offsets *new_offsets;
  int i;

  objfile = vp->objfile;
  if (objfile == NULL)
    {
      /* OK, it's not an objfile we opened ourselves.
         Currently, that can only happen with the exec file, so
         relocate the symbols for the symfile.  */
      if (symfile_objfile == NULL)
	return;
      objfile = symfile_objfile;
    }
  else if (!vp->loaded)
    /* If symbols are not yet loaded, offsets are not yet valid.  */
    return;

  new_offsets =
    (struct section_offsets *)
    alloca (SIZEOF_N_SECTION_OFFSETS (objfile->num_sections));

  for (i = 0; i < objfile->num_sections; ++i)
    new_offsets->offsets[i] = ANOFFSET (objfile->section_offsets, i);

  /* The symbols in the object file are linked to the VMA of the section,
     relocate them VMA relative.  */
  new_offsets->offsets[SECT_OFF_TEXT (objfile)] = vp->tstart - vp->tvma;
  new_offsets->offsets[SECT_OFF_DATA (objfile)] = vp->dstart - vp->dvma;
  new_offsets->offsets[SECT_OFF_BSS (objfile)] = vp->dstart - vp->dvma;

  /* Perform the same adjustment as the loader if the .data and
     .bss sections overlap.  */
  new_offsets->offsets[SECT_OFF_BSS (objfile)] += bss_data_overlap (objfile);

  objfile_relocate (objfile, new_offsets);
}

/* Add symbols for an objfile.  */

static int
objfile_symbol_add (void *arg)
{
  struct objfile *obj = (struct objfile *) arg;

  syms_from_objfile (obj, NULL, 0, 0, 0);
  new_symfile_objfile (obj, 0);
  return 1;
}

/* Add symbols for a vmap. Return zero upon error.  */

int
vmap_add_symbols (struct vmap *vp)
{
  if (catch_errors (objfile_symbol_add, vp->objfile,
		    "Error while reading shared library symbols:\n",
		    RETURN_MASK_ALL))
    {
      /* Note this is only done if symbol reading was successful.  */
      vp->loaded = 1;
      vmap_symtab (vp);
      return 1;
    }
  return 0;
}

/* Add a new vmap entry based on ldinfo() information.

   If ldi->ldinfo_fd is not valid (e.g. this struct ld_info is from a
   core file), the caller should set it to -1, and we will open the file.

   Return the vmap new entry.  */

static struct vmap *
add_vmap (LdInfo *ldi)
{
  bfd *abfd, *last;
  char *mem, *filename;
  struct objfile *obj;
  struct vmap *vp;
  int fd;
  ARCH64_DECL (arch64);

  /* This ldi structure was allocated using alloca() in 
     xcoff_relocate_symtab().  Now we need to have persistent object 
     and member names, so we should save them.  */

  filename = LDI_FILENAME (ldi, arch64);
  mem = filename + strlen (filename) + 1;
  mem = xstrdup (mem);

  fd = LDI_FD (ldi, arch64);
  abfd = gdb_bfd_open (filename, gnutarget, fd < 0 ? -1 : fd);
  if (!abfd)
    {
      warning (_("Could not open `%s' as an executable file: %s"),
	       filename, bfd_errmsg (bfd_get_error ()));
      return NULL;
    }

  /* Make sure we have an object file.  */

  if (bfd_check_format (abfd, bfd_object))
    vp = map_vmap (abfd, 0);

  else if (bfd_check_format (abfd, bfd_archive))
    {
      last = gdb_bfd_openr_next_archived_file (abfd, NULL);
      while (last != NULL)
	{
	  bfd *next;

	  if (strcmp (mem, last->filename) == 0)
	    break;

	  next = gdb_bfd_openr_next_archived_file (abfd, last);
	  gdb_bfd_unref (last);
	  last = next;
	}

      if (!last)
	{
	  warning (_("\"%s\": member \"%s\" missing."), filename, mem);
	  gdb_bfd_unref (abfd);
	  return NULL;
	}

      if (!bfd_check_format (last, bfd_object))
	{
	  warning (_("\"%s\": member \"%s\" not in executable format: %s."),
		   filename, mem, bfd_errmsg (bfd_get_error ()));
	  gdb_bfd_unref (last);
	  gdb_bfd_unref (abfd);
	  return NULL;
	}

      vp = map_vmap (last, abfd);
      /* map_vmap acquired a reference to LAST, so we can release
	 ours.  */
      gdb_bfd_unref (last);
    }
  else
    {
      warning (_("\"%s\": not in executable format: %s."),
	       filename, bfd_errmsg (bfd_get_error ()));
      gdb_bfd_unref (abfd);
      return NULL;
    }
  obj = allocate_objfile (vp->bfd, 0);
  vp->objfile = obj;

  /* Always add symbols for the main objfile.  */
  if (vp == vmap || auto_solib_add)
    vmap_add_symbols (vp);

  /* Anything needing a reference to ABFD has already acquired it, so
     release our local reference.  */
  gdb_bfd_unref (abfd);

  return vp;
}

/* update VMAP info with ldinfo() information
   Input is ptr to ldinfo() results.  */

static void
vmap_ldinfo (LdInfo *ldi)
{
  struct stat ii, vi;
  struct vmap *vp;
  int got_one, retried;
  int got_exec_file = 0;
  uint next;
  int arch64 = ARCH64 ();

  /* For each *ldi, see if we have a corresponding *vp.
     If so, update the mapping, and symbol table.
     If not, add an entry and symbol table.  */

  do
    {
      char *name = LDI_FILENAME (ldi, arch64);
      char *memb = name + strlen (name) + 1;
      int fd = LDI_FD (ldi, arch64);

      retried = 0;

      if (fstat (fd, &ii) < 0)
	{
	  /* The kernel sets ld_info to -1, if the process is still using the
	     object, and the object is removed.  Keep the symbol info for the
	     removed object and issue a warning.  */
	  warning (_("%s (fd=%d) has disappeared, keeping its symbols"),
		   name, fd);
	  continue;
	}
    retry:
      for (got_one = 0, vp = vmap; vp; vp = vp->nxt)
	{
	  struct objfile *objfile;

	  /* First try to find a `vp', which is the same as in ldinfo.
	     If not the same, just continue and grep the next `vp'.  If same,
	     relocate its tstart, tend, dstart, dend values.  If no such `vp'
	     found, get out of this for loop, add this ldi entry as a new vmap
	     (add_vmap) and come back, find its `vp' and so on...  */

	  /* The filenames are not always sufficient to match on.  */

	  if ((name[0] == '/' && strcmp (name, vp->name) != 0)
	      || (memb[0] && strcmp (memb, vp->member) != 0))
	    continue;

	  /* See if we are referring to the same file.
	     We have to check objfile->obfd, symfile.c:reread_symbols might
	     have updated the obfd after a change.  */
	  objfile = vp->objfile == NULL ? symfile_objfile : vp->objfile;
	  if (objfile == NULL
	      || objfile->obfd == NULL
	      || bfd_stat (objfile->obfd, &vi) < 0)
	    {
	      warning (_("Unable to stat %s, keeping its symbols"), name);
	      continue;
	    }

	  if (ii.st_dev != vi.st_dev || ii.st_ino != vi.st_ino)
	    continue;

	  if (!retried)
	    close (fd);

	  ++got_one;

	  /* Found a corresponding VMAP.  Remap!  */

	  vmap_secs (vp, ldi, arch64);

	  /* The objfile is only NULL for the exec file.  */
	  if (vp->objfile == NULL)
	    got_exec_file = 1;

	  /* relocate symbol table(s).  */
	  vmap_symtab (vp);

	  /* Announce new object files.  Doing this after symbol relocation
	     makes aix-thread.c's job easier.  */
	  if (vp->objfile)
	    observer_notify_new_objfile (vp->objfile);

	  /* There may be more, so we don't break out of the loop.  */
	}

      /* If there was no matching *vp, we must perforce create the
	 sucker(s). */
      if (!got_one && !retried)
	{
	  add_vmap (ldi);
	  ++retried;
	  goto retry;
	}
    }
  while ((next = LDI_NEXT (ldi, arch64))
	 && (ldi = (void *) (next + (char *) ldi)));

  /* If we don't find the symfile_objfile anywhere in the ldinfo, it
     is unlikely that the symbol file is relocated to the proper
     address.  And we might have attached to a process which is
     running a different copy of the same executable.  */
  if (symfile_objfile != NULL && !got_exec_file)
    {
      warning (_("Symbol file %s\nis not mapped; discarding it.\n\
If in fact that file has symbols which the mapped files listed by\n\
\"info files\" lack, you can load symbols with the \"symbol-file\" or\n\
\"add-symbol-file\" commands (note that you must take care of relocating\n\
symbols to the proper address)."),
	       symfile_objfile->name);
      free_objfile (symfile_objfile);
      gdb_assert (symfile_objfile == NULL);
    }
  breakpoint_re_set ();
}

/* As well as symbol tables, exec_sections need relocation.  After
   the inferior process' termination, there will be a relocated symbol
   table exist with no corresponding inferior process.  At that time, we
   need to use `exec' bfd, rather than the inferior process's memory space
   to look up symbols.

   `exec_sections' need to be relocated only once, as long as the exec
   file remains unchanged.  */

static void
vmap_exec (void)
{
  static bfd *execbfd;
  int i;
  struct target_section_table *table = target_get_section_table (&exec_ops);

  if (execbfd == exec_bfd)
    return;

  execbfd = exec_bfd;

  if (!vmap || !table->sections)
    error (_("vmap_exec: vmap or table->sections == 0."));

  for (i = 0; &table->sections[i] < table->sections_end; i++)
    {
      if (strcmp (".text", table->sections[i].the_bfd_section->name) == 0)
	{
	  table->sections[i].addr += vmap->tstart - vmap->tvma;
	  table->sections[i].endaddr += vmap->tstart - vmap->tvma;
	}
      else if (strcmp (".data", table->sections[i].the_bfd_section->name) == 0)
	{
	  table->sections[i].addr += vmap->dstart - vmap->dvma;
	  table->sections[i].endaddr += vmap->dstart - vmap->dvma;
	}
      else if (strcmp (".bss", table->sections[i].the_bfd_section->name) == 0)
	{
	  table->sections[i].addr += vmap->dstart - vmap->dvma;
	  table->sections[i].endaddr += vmap->dstart - vmap->dvma;
	}
    }
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


/* xcoff_relocate_symtab -      hook for symbol table relocation.
   
   This is only applicable to live processes, and is a no-op when
   debugging a core file.  */

void
xcoff_relocate_symtab (unsigned int pid)
{
  int load_segs = 64; /* number of load segments */
  int rc;
  LdInfo *ldi = NULL;
  int arch64 = ARCH64 ();
  int ldisize = arch64 ? sizeof (ldi->l64) : sizeof (ldi->l32);
  int size;

  /* Nothing to do if we are debugging a core file.  */
  if (!target_has_execution)
    return;

  do
    {
      size = load_segs * ldisize;
      ldi = (void *) xrealloc (ldi, size);

#if 0
      /* According to my humble theory, AIX has some timing problems and
         when the user stack grows, kernel doesn't update stack info in time
         and ptrace calls step on user stack.  That is why we sleep here a
         little, and give kernel to update its internals.  */
      usleep (36000);
#endif

      if (arch64)
	rc = rs6000_ptrace64 (PT_LDINFO, pid, (unsigned long) ldi, size, NULL);
      else
	rc = rs6000_ptrace32 (PT_LDINFO, pid, (int *) ldi, size, NULL);

      if (rc == -1)
        {
          if (errno == ENOMEM)
            load_segs *= 2;
          else
            perror_with_name (_("ptrace ldinfo"));
        }
      else
	{
          vmap_ldinfo (ldi);
          vmap_exec (); /* relocate the exec and core sections as well.  */
	}
    } while (rc == -1);
  if (ldi)
    xfree (ldi);
}

/* Core file stuff.  */

/* Relocate symtabs and read in shared library info, based on symbols
   from the core file.  */

void
xcoff_relocate_core (struct target_ops *target)
{
  struct bfd_section *ldinfo_sec;
  int offset = 0;
  LdInfo *ldi;
  struct vmap *vp;
  int arch64 = ARCH64 ();

  /* Size of a struct ld_info except for the variable-length filename.  */
  int nonfilesz = (int)LDI_FILENAME ((LdInfo *)0, arch64);

  /* Allocated size of buffer.  */
  int buffer_size = nonfilesz;
  char *buffer = xmalloc (buffer_size);
  struct cleanup *old = make_cleanup (free_current_contents, &buffer);

  ldinfo_sec = bfd_get_section_by_name (core_bfd, ".ldinfo");
  if (ldinfo_sec == NULL)
    {
    bfd_err:
      fprintf_filtered (gdb_stderr, "Couldn't get ldinfo from core file: %s\n",
			bfd_errmsg (bfd_get_error ()));
      do_cleanups (old);
      return;
    }
  do
    {
      int i;
      int names_found = 0;

      /* Read in everything but the name.  */
      if (bfd_get_section_contents (core_bfd, ldinfo_sec, buffer,
				    offset, nonfilesz) == 0)
	goto bfd_err;

      /* Now the name.  */
      i = nonfilesz;
      do
	{
	  if (i == buffer_size)
	    {
	      buffer_size *= 2;
	      buffer = xrealloc (buffer, buffer_size);
	    }
	  if (bfd_get_section_contents (core_bfd, ldinfo_sec, &buffer[i],
					offset + i, 1) == 0)
	    goto bfd_err;
	  if (buffer[i++] == '\0')
	    ++names_found;
	}
      while (names_found < 2);

      ldi = (LdInfo *) buffer;

      /* Can't use a file descriptor from the core file; need to open it.  */
      if (arch64)
	ldi->l64.ldinfo_fd = -1;
      else
	ldi->l32.ldinfo_fd = -1;

      /* The first ldinfo is for the exec file, allocated elsewhere.  */
      if (offset == 0 && vmap != NULL)
	vp = vmap;
      else
	vp = add_vmap (ldi);

      /* Process next shared library upon error.  */
      offset += LDI_NEXT (ldi, arch64);
      if (vp == NULL)
	continue;

      vmap_secs (vp, ldi, arch64);

      /* Unless this is the exec file,
         add our sections to the section table for the core target.  */
      if (vp != vmap)
	{
	  struct target_section *stp;

	  stp = deprecated_core_resize_section_table (2);

	  stp->bfd = vp->bfd;
	  stp->the_bfd_section = bfd_get_section_by_name (stp->bfd, ".text");
	  stp->addr = vp->tstart;
	  stp->endaddr = vp->tend;
	  stp++;

	  stp->bfd = vp->bfd;
	  stp->the_bfd_section = bfd_get_section_by_name (stp->bfd, ".data");
	  stp->addr = vp->dstart;
	  stp->endaddr = vp->dend;
	}

      vmap_symtab (vp);

      if (vp != vmap && vp->objfile)
	observer_notify_new_objfile (vp->objfile);
    }
  while (LDI_NEXT (ldi, arch64) != 0);
  vmap_exec ();
  breakpoint_re_set ();
  do_cleanups (old);
}

/* Under AIX, we have to pass the correct TOC pointer to a function
   when calling functions in the inferior.
   We try to find the relative toc offset of the objfile containing PC
   and add the current load address of the data segment from the vmap.  */

static CORE_ADDR
find_toc_address (CORE_ADDR pc)
{
  struct vmap *vp;

  for (vp = vmap; vp; vp = vp->nxt)
    {
      if (pc >= vp->tstart && pc < vp->tend)
	{
	  /* vp->objfile is only NULL for the exec file.  */
	  return vp->dstart + xcoff_get_toc_offset (vp->objfile == NULL
						    ? symfile_objfile
						    : vp->objfile);
	}
    }
  error (_("Unable to find TOC entry for pc %s."), hex_string (pc));
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

  /* Initialize hook in rs6000-tdep.c for determining the TOC address
     when calling functions in the inferior.  */
  rs6000_find_toc_address_hook = find_toc_address;
}
