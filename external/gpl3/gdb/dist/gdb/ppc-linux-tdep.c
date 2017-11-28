/* Target-dependent code for GDB, the GNU debugger.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "objfiles.h"
#include "regcache.h"
#include "value.h"
#include "osabi.h"
#include "regset.h"
#include "solib-svr4.h"
#include "solib-spu.h"
#include "solib.h"
#include "solist.h"
#include "ppc-tdep.h"
#include "ppc64-tdep.h"
#include "ppc-linux-tdep.h"
#include "glibc-tdep.h"
#include "trad-frame.h"
#include "frame-unwind.h"
#include "tramp-frame.h"
#include "observer.h"
#include "auxv.h"
#include "elf/common.h"
#include "elf/ppc64.h"
#include "arch-utils.h"
#include "spu-tdep.h"
#include "xml-syscall.h"
#include "linux-tdep.h"
#include "linux-record.h"
#include "record-full.h"
#include "infrun.h"

#include "stap-probe.h"
#include "ax.h"
#include "ax-gdb.h"
#include "cli/cli-utils.h"
#include "parser-defs.h"
#include "user-regs.h"
#include <ctype.h>
#include "elf-bfd.h"            /* for elfcore_write_* */

#include "features/rs6000/powerpc-32l.c"
#include "features/rs6000/powerpc-altivec32l.c"
#include "features/rs6000/powerpc-cell32l.c"
#include "features/rs6000/powerpc-vsx32l.c"
#include "features/rs6000/powerpc-isa205-32l.c"
#include "features/rs6000/powerpc-isa205-altivec32l.c"
#include "features/rs6000/powerpc-isa205-vsx32l.c"
#include "features/rs6000/powerpc-64l.c"
#include "features/rs6000/powerpc-altivec64l.c"
#include "features/rs6000/powerpc-cell64l.c"
#include "features/rs6000/powerpc-vsx64l.c"
#include "features/rs6000/powerpc-isa205-64l.c"
#include "features/rs6000/powerpc-isa205-altivec64l.c"
#include "features/rs6000/powerpc-isa205-vsx64l.c"
#include "features/rs6000/powerpc-e500l.c"

/* Shared library operations for PowerPC-Linux.  */
static struct target_so_ops powerpc_so_ops;

/* The syscall's XML filename for PPC and PPC64.  */
#define XML_SYSCALL_FILENAME_PPC "syscalls/ppc-linux.xml"
#define XML_SYSCALL_FILENAME_PPC64 "syscalls/ppc64-linux.xml"

/* ppc_linux_memory_remove_breakpoints attempts to remove a breakpoint
   in much the same fashion as memory_remove_breakpoint in mem-break.c,
   but is careful not to write back the previous contents if the code
   in question has changed in between inserting the breakpoint and
   removing it.

   Here is the problem that we're trying to solve...

   Once upon a time, before introducing this function to remove
   breakpoints from the inferior, setting a breakpoint on a shared
   library function prior to running the program would not work
   properly.  In order to understand the problem, it is first
   necessary to understand a little bit about dynamic linking on
   this platform.

   A call to a shared library function is accomplished via a bl
   (branch-and-link) instruction whose branch target is an entry
   in the procedure linkage table (PLT).  The PLT in the object
   file is uninitialized.  To gdb, prior to running the program, the
   entries in the PLT are all zeros.

   Once the program starts running, the shared libraries are loaded
   and the procedure linkage table is initialized, but the entries in
   the table are not (necessarily) resolved.  Once a function is
   actually called, the code in the PLT is hit and the function is
   resolved.  In order to better illustrate this, an example is in
   order; the following example is from the gdb testsuite.
	    
	We start the program shmain.

	    [kev@arroyo testsuite]$ ../gdb gdb.base/shmain
	    [...]

	We place two breakpoints, one on shr1 and the other on main.

	    (gdb) b shr1
	    Breakpoint 1 at 0x100409d4
	    (gdb) b main
	    Breakpoint 2 at 0x100006a0: file gdb.base/shmain.c, line 44.

	Examine the instruction (and the immediatly following instruction)
	upon which the breakpoint was placed.  Note that the PLT entry
	for shr1 contains zeros.

	    (gdb) x/2i 0x100409d4
	    0x100409d4 <shr1>:      .long 0x0
	    0x100409d8 <shr1+4>:    .long 0x0

	Now run 'til main.

	    (gdb) r
	    Starting program: gdb.base/shmain 
	    Breakpoint 1 at 0xffaf790: file gdb.base/shr1.c, line 19.

	    Breakpoint 2, main ()
		at gdb.base/shmain.c:44
	    44        g = 1;

	Examine the PLT again.  Note that the loading of the shared
	library has initialized the PLT to code which loads a constant
	(which I think is an index into the GOT) into r11 and then
	branchs a short distance to the code which actually does the
	resolving.

	    (gdb) x/2i 0x100409d4
	    0x100409d4 <shr1>:      li      r11,4
	    0x100409d8 <shr1+4>:    b       0x10040984 <sg+4>
	    (gdb) c
	    Continuing.

	    Breakpoint 1, shr1 (x=1)
		at gdb.base/shr1.c:19
	    19        l = 1;

	Now we've hit the breakpoint at shr1.  (The breakpoint was
	reset from the PLT entry to the actual shr1 function after the
	shared library was loaded.) Note that the PLT entry has been
	resolved to contain a branch that takes us directly to shr1.
	(The real one, not the PLT entry.)

	    (gdb) x/2i 0x100409d4
	    0x100409d4 <shr1>:      b       0xffaf76c <shr1>
	    0x100409d8 <shr1+4>:    b       0x10040984 <sg+4>

   The thing to note here is that the PLT entry for shr1 has been
   changed twice.

   Now the problem should be obvious.  GDB places a breakpoint (a
   trap instruction) on the zero value of the PLT entry for shr1.
   Later on, after the shared library had been loaded and the PLT
   initialized, GDB gets a signal indicating this fact and attempts
   (as it always does when it stops) to remove all the breakpoints.

   The breakpoint removal was causing the former contents (a zero
   word) to be written back to the now initialized PLT entry thus
   destroying a portion of the initialization that had occurred only a
   short time ago.  When execution continued, the zero word would be
   executed as an instruction an illegal instruction trap was
   generated instead.  (0 is not a legal instruction.)

   The fix for this problem was fairly straightforward.  The function
   memory_remove_breakpoint from mem-break.c was copied to this file,
   modified slightly, and renamed to ppc_linux_memory_remove_breakpoint.
   In tm-linux.h, MEMORY_REMOVE_BREAKPOINT is defined to call this new
   function.

   The differences between ppc_linux_memory_remove_breakpoint () and
   memory_remove_breakpoint () are minor.  All that the former does
   that the latter does not is check to make sure that the breakpoint
   location actually contains a breakpoint (trap instruction) prior
   to attempting to write back the old contents.  If it does contain
   a trap instruction, we allow the old contents to be written back.
   Otherwise, we silently do nothing.

   The big question is whether memory_remove_breakpoint () should be
   changed to have the same functionality.  The downside is that more
   traffic is generated for remote targets since we'll have an extra
   fetch of a memory word each time a breakpoint is removed.

   For the time being, we'll leave this self-modifying-code-friendly
   version in ppc-linux-tdep.c, but it ought to be migrated somewhere
   else in the event that some other platform has similar needs with
   regard to removing breakpoints in some potentially self modifying
   code.  */
static int
ppc_linux_memory_remove_breakpoint (struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr = bp_tgt->reqstd_address;
  const unsigned char *bp;
  int val;
  int bplen;
  gdb_byte old_contents[BREAKPOINT_MAX];
  struct cleanup *cleanup;

  /* Determine appropriate breakpoint contents and size for this address.  */
  bp = gdbarch_breakpoint_from_pc (gdbarch, &addr, &bplen);

  /* Make sure we see the memory breakpoints.  */
  cleanup = make_show_memory_breakpoints_cleanup (1);
  val = target_read_memory (addr, old_contents, bplen);

  /* If our breakpoint is no longer at the address, this means that the
     program modified the code on us, so it is wrong to put back the
     old value.  */
  if (val == 0 && memcmp (bp, old_contents, bplen) == 0)
    val = target_write_raw_memory (addr, bp_tgt->shadow_contents, bplen);

  do_cleanups (cleanup);
  return val;
}

/* For historic reasons, PPC 32 GNU/Linux follows PowerOpen rather
   than the 32 bit SYSV R4 ABI structure return convention - all
   structures, no matter their size, are put in memory.  Vectors,
   which were added later, do get returned in a register though.  */

static enum return_value_convention
ppc_linux_return_value (struct gdbarch *gdbarch, struct value *function,
			struct type *valtype, struct regcache *regcache,
			gdb_byte *readbuf, const gdb_byte *writebuf)
{  
  if ((TYPE_CODE (valtype) == TYPE_CODE_STRUCT
       || TYPE_CODE (valtype) == TYPE_CODE_UNION)
      && !((TYPE_LENGTH (valtype) == 16 || TYPE_LENGTH (valtype) == 8)
	   && TYPE_VECTOR (valtype)))
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    return ppc_sysv_abi_return_value (gdbarch, function, valtype, regcache,
				      readbuf, writebuf);
}

/* PLT stub in executable.  */
static struct ppc_insn_pattern powerpc32_plt_stub[] =
  {
    { 0xffff0000, 0x3d600000, 0 },	/* lis   r11, xxxx	 */
    { 0xffff0000, 0x816b0000, 0 },	/* lwz   r11, xxxx(r11)  */
    { 0xffffffff, 0x7d6903a6, 0 },	/* mtctr r11		 */
    { 0xffffffff, 0x4e800420, 0 },	/* bctr			 */
    {          0,          0, 0 }
  };

/* PLT stub in shared library.  */
static struct ppc_insn_pattern powerpc32_plt_stub_so[] =
  {
    { 0xffff0000, 0x817e0000, 0 },	/* lwz   r11, xxxx(r30)  */
    { 0xffffffff, 0x7d6903a6, 0 },	/* mtctr r11		 */
    { 0xffffffff, 0x4e800420, 0 },	/* bctr			 */
    { 0xffffffff, 0x60000000, 0 },	/* nop			 */
    {          0,          0, 0 }
  };
#define POWERPC32_PLT_STUB_LEN 	ARRAY_SIZE (powerpc32_plt_stub)

/* Check if PC is in PLT stub.  For non-secure PLT, stub is in .plt
   section.  For secure PLT, stub is in .text and we need to check
   instruction patterns.  */

static int
powerpc_linux_in_dynsym_resolve_code (CORE_ADDR pc)
{
  struct bound_minimal_symbol sym;

  /* Check whether PC is in the dynamic linker.  This also checks
     whether it is in the .plt section, used by non-PIC executables.  */
  if (svr4_in_dynsym_resolve_code (pc))
    return 1;

  /* Check if we are in the resolver.  */
  sym = lookup_minimal_symbol_by_pc (pc);
  if (sym.minsym != NULL
      && (strcmp (MSYMBOL_LINKAGE_NAME (sym.minsym), "__glink") == 0
	  || strcmp (MSYMBOL_LINKAGE_NAME (sym.minsym),
		     "__glink_PLTresolve") == 0))
    return 1;

  return 0;
}

/* Follow PLT stub to actual routine.

   When the execution direction is EXEC_REVERSE, scan backward to
   check whether we are in the middle of a PLT stub.  Currently,
   we only look-behind at most 4 instructions (the max length of PLT
   stub sequence.  */

static CORE_ADDR
ppc_skip_trampoline_code (struct frame_info *frame, CORE_ADDR pc)
{
  unsigned int insnbuf[POWERPC32_PLT_STUB_LEN];
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR target = 0;
  int scan_limit, i;

  scan_limit = 1;
  /* When reverse-debugging, scan backward to check whether we are
     in the middle of trampoline code.  */
  if (execution_direction == EXEC_REVERSE)
    scan_limit = 4;	/* At more 4 instructions.  */

  for (i = 0; i < scan_limit; i++)
    {
      if (ppc_insns_match_pattern (frame, pc, powerpc32_plt_stub, insnbuf))
	{
	  /* Insn pattern is
	     lis   r11, xxxx
	     lwz   r11, xxxx(r11)
	     Branch target is in r11.  */

	  target = (ppc_insn_d_field (insnbuf[0]) << 16)
		   | ppc_insn_d_field (insnbuf[1]);
	  target = read_memory_unsigned_integer (target, 4, byte_order);
	}
      else if (ppc_insns_match_pattern (frame, pc, powerpc32_plt_stub_so,
					insnbuf))
	{
	  /* Insn pattern is
	     lwz   r11, xxxx(r30)
	     Branch target is in r11.  */

	  target = get_frame_register_unsigned (frame,
						tdep->ppc_gp0_regnum + 30)
		   + ppc_insn_d_field (insnbuf[0]);
	  target = read_memory_unsigned_integer (target, 4, byte_order);
	}
      else
	{
	  /* Scan backward one more instructions if doesn't match.  */
	  pc -= 4;
	  continue;
	}

      return target;
    }

  return 0;
}

/* Wrappers to handle Linux-only registers.  */

static void
ppc_linux_supply_gregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *gregs, size_t len)
{
  const struct ppc_reg_offsets *offsets
    = (const struct ppc_reg_offsets *) regset->regmap;

  ppc_supply_gregset (regset, regcache, regnum, gregs, len);

  if (ppc_linux_trap_reg_p (get_regcache_arch (regcache)))
    {
      /* "orig_r3" is stored 2 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_ORIG_R3_REGNUM)
	ppc_supply_reg (regcache, PPC_ORIG_R3_REGNUM, (const gdb_byte *) gregs,
			offsets->pc_offset + 2 * offsets->gpr_size,
			offsets->gpr_size);

      /* "trap" is stored 8 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_TRAP_REGNUM)
	ppc_supply_reg (regcache, PPC_TRAP_REGNUM, (const gdb_byte *) gregs,
			offsets->pc_offset + 8 * offsets->gpr_size,
			offsets->gpr_size);
    }
}

static void
ppc_linux_collect_gregset (const struct regset *regset,
			   const struct regcache *regcache,
			   int regnum, void *gregs, size_t len)
{
  const struct ppc_reg_offsets *offsets
    = (const struct ppc_reg_offsets *) regset->regmap;

  /* Clear areas in the linux gregset not written elsewhere.  */
  if (regnum == -1)
    memset (gregs, 0, len);

  ppc_collect_gregset (regset, regcache, regnum, gregs, len);

  if (ppc_linux_trap_reg_p (get_regcache_arch (regcache)))
    {
      /* "orig_r3" is stored 2 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_ORIG_R3_REGNUM)
	ppc_collect_reg (regcache, PPC_ORIG_R3_REGNUM, (gdb_byte *) gregs,
			 offsets->pc_offset + 2 * offsets->gpr_size,
			 offsets->gpr_size);

      /* "trap" is stored 8 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_TRAP_REGNUM)
	ppc_collect_reg (regcache, PPC_TRAP_REGNUM, (gdb_byte *) gregs,
			 offsets->pc_offset + 8 * offsets->gpr_size,
			 offsets->gpr_size);
    }
}

/* Regset descriptions.  */
static const struct ppc_reg_offsets ppc32_linux_reg_offsets =
  {
    /* General-purpose registers.  */
    /* .r0_offset = */ 0,
    /* .gpr_size = */ 4,
    /* .xr_size = */ 4,
    /* .pc_offset = */ 128,
    /* .ps_offset = */ 132,
    /* .cr_offset = */ 152,
    /* .lr_offset = */ 144,
    /* .ctr_offset = */ 140,
    /* .xer_offset = */ 148,
    /* .mq_offset = */ 156,

    /* Floating-point registers.  */
    /* .f0_offset = */ 0,
    /* .fpscr_offset = */ 256,
    /* .fpscr_size = */ 8,

    /* AltiVec registers.  */
    /* .vr0_offset = */ 0,
    /* .vscr_offset = */ 512 + 12,
    /* .vrsave_offset = */ 528
  };

static const struct ppc_reg_offsets ppc64_linux_reg_offsets =
  {
    /* General-purpose registers.  */
    /* .r0_offset = */ 0,
    /* .gpr_size = */ 8,
    /* .xr_size = */ 8,
    /* .pc_offset = */ 256,
    /* .ps_offset = */ 264,
    /* .cr_offset = */ 304,
    /* .lr_offset = */ 288,
    /* .ctr_offset = */ 280,
    /* .xer_offset = */ 296,
    /* .mq_offset = */ 312,

    /* Floating-point registers.  */
    /* .f0_offset = */ 0,
    /* .fpscr_offset = */ 256,
    /* .fpscr_size = */ 8,

    /* AltiVec registers.  */
    /* .vr0_offset = */ 0,
    /* .vscr_offset = */ 512 + 12,
    /* .vrsave_offset = */ 528
  };

static const struct regset ppc32_linux_gregset = {
  &ppc32_linux_reg_offsets,
  ppc_linux_supply_gregset,
  ppc_linux_collect_gregset
};

static const struct regset ppc64_linux_gregset = {
  &ppc64_linux_reg_offsets,
  ppc_linux_supply_gregset,
  ppc_linux_collect_gregset
};

static const struct regset ppc32_linux_fpregset = {
  &ppc32_linux_reg_offsets,
  ppc_supply_fpregset,
  ppc_collect_fpregset
};

static const struct regset ppc32_linux_vrregset = {
  &ppc32_linux_reg_offsets,
  ppc_supply_vrregset,
  ppc_collect_vrregset
};

static const struct regset ppc32_linux_vsxregset = {
  &ppc32_linux_reg_offsets,
  ppc_supply_vsxregset,
  ppc_collect_vsxregset
};

const struct regset *
ppc_linux_gregset (int wordsize)
{
  return wordsize == 8 ? &ppc64_linux_gregset : &ppc32_linux_gregset;
}

const struct regset *
ppc_linux_fpregset (void)
{
  return &ppc32_linux_fpregset;
}

/* Iterate over supported core file register note sections. */

static void
ppc_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int have_altivec = tdep->ppc_vr0_regnum != -1;
  int have_vsx = tdep->ppc_vsr0_upper_regnum != -1;

  if (tdep->wordsize == 4)
    cb (".reg", 48 * 4, &ppc32_linux_gregset, NULL, cb_data);
  else
    cb (".reg", 48 * 8, &ppc64_linux_gregset, NULL, cb_data);

  cb (".reg2", 264, &ppc32_linux_fpregset, NULL, cb_data);

  if (have_altivec)
    cb (".reg-ppc-vmx", 544, &ppc32_linux_vrregset, "ppc Altivec", cb_data);

  if (have_vsx)
    cb (".reg-ppc-vsx", 256, &ppc32_linux_vsxregset, "POWER7 VSX", cb_data);
}

static void
ppc_linux_sigtramp_cache (struct frame_info *this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func, LONGEST offset,
			  int bias)
{
  CORE_ADDR base;
  CORE_ADDR regs;
  CORE_ADDR gpregs;
  CORE_ADDR fpregs;
  int i;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  base = get_frame_register_unsigned (this_frame,
				      gdbarch_sp_regnum (gdbarch));
  if (bias > 0 && get_frame_pc (this_frame) != func)
    /* See below, some signal trampolines increment the stack as their
       first instruction, need to compensate for that.  */
    base -= bias;

  /* Find the address of the register buffer pointer.  */
  regs = base + offset;
  /* Use that to find the address of the corresponding register
     buffers.  */
  gpregs = read_memory_unsigned_integer (regs, tdep->wordsize, byte_order);
  fpregs = gpregs + 48 * tdep->wordsize;

  /* General purpose.  */
  for (i = 0; i < 32; i++)
    {
      int regnum = i + tdep->ppc_gp0_regnum;
      trad_frame_set_reg_addr (this_cache,
			       regnum, gpregs + i * tdep->wordsize);
    }
  trad_frame_set_reg_addr (this_cache,
			   gdbarch_pc_regnum (gdbarch),
			   gpregs + 32 * tdep->wordsize);
  trad_frame_set_reg_addr (this_cache, tdep->ppc_ctr_regnum,
			   gpregs + 35 * tdep->wordsize);
  trad_frame_set_reg_addr (this_cache, tdep->ppc_lr_regnum,
			   gpregs + 36 * tdep->wordsize);
  trad_frame_set_reg_addr (this_cache, tdep->ppc_xer_regnum,
			   gpregs + 37 * tdep->wordsize);
  trad_frame_set_reg_addr (this_cache, tdep->ppc_cr_regnum,
			   gpregs + 38 * tdep->wordsize);

  if (ppc_linux_trap_reg_p (gdbarch))
    {
      trad_frame_set_reg_addr (this_cache, PPC_ORIG_R3_REGNUM,
			       gpregs + 34 * tdep->wordsize);
      trad_frame_set_reg_addr (this_cache, PPC_TRAP_REGNUM,
			       gpregs + 40 * tdep->wordsize);
    }

  if (ppc_floating_point_unit_p (gdbarch))
    {
      /* Floating point registers.  */
      for (i = 0; i < 32; i++)
	{
	  int regnum = i + gdbarch_fp0_regnum (gdbarch);
	  trad_frame_set_reg_addr (this_cache, regnum,
				   fpregs + i * tdep->wordsize);
	}
      trad_frame_set_reg_addr (this_cache, tdep->ppc_fpscr_regnum,
                         fpregs + 32 * tdep->wordsize);
    }
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

static void
ppc32_linux_sigaction_cache_init (const struct tramp_frame *self,
				  struct frame_info *this_frame,
				  struct trad_frame_cache *this_cache,
				  CORE_ADDR func)
{
  ppc_linux_sigtramp_cache (this_frame, this_cache, func,
			    0xd0 /* Offset to ucontext_t.  */
			    + 0x30 /* Offset to .reg.  */,
			    0);
}

static void
ppc64_linux_sigaction_cache_init (const struct tramp_frame *self,
				  struct frame_info *this_frame,
				  struct trad_frame_cache *this_cache,
				  CORE_ADDR func)
{
  ppc_linux_sigtramp_cache (this_frame, this_cache, func,
			    0x80 /* Offset to ucontext_t.  */
			    + 0xe0 /* Offset to .reg.  */,
			    128);
}

static void
ppc32_linux_sighandler_cache_init (const struct tramp_frame *self,
				   struct frame_info *this_frame,
				   struct trad_frame_cache *this_cache,
				   CORE_ADDR func)
{
  ppc_linux_sigtramp_cache (this_frame, this_cache, func,
			    0x40 /* Offset to ucontext_t.  */
			    + 0x1c /* Offset to .reg.  */,
			    0);
}

static void
ppc64_linux_sighandler_cache_init (const struct tramp_frame *self,
				   struct frame_info *this_frame,
				   struct trad_frame_cache *this_cache,
				   CORE_ADDR func)
{
  ppc_linux_sigtramp_cache (this_frame, this_cache, func,
			    0x80 /* Offset to struct sigcontext.  */
			    + 0x38 /* Offset to .reg.  */,
			    128);
}

static struct tramp_frame ppc32_linux_sigaction_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  { 
    { 0x380000ac, -1 }, /* li r0, 172 */
    { 0x44000002, -1 }, /* sc */
    { TRAMP_SENTINEL_INSN },
  },
  ppc32_linux_sigaction_cache_init
};
static struct tramp_frame ppc64_linux_sigaction_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { 0x38210080, -1 }, /* addi r1,r1,128 */
    { 0x380000ac, -1 }, /* li r0, 172 */
    { 0x44000002, -1 }, /* sc */
    { TRAMP_SENTINEL_INSN },
  },
  ppc64_linux_sigaction_cache_init
};
static struct tramp_frame ppc32_linux_sighandler_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  { 
    { 0x38000077, -1 }, /* li r0,119 */
    { 0x44000002, -1 }, /* sc */
    { TRAMP_SENTINEL_INSN },
  },
  ppc32_linux_sighandler_cache_init
};
static struct tramp_frame ppc64_linux_sighandler_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  { 
    { 0x38210080, -1 }, /* addi r1,r1,128 */
    { 0x38000077, -1 }, /* li r0,119 */
    { 0x44000002, -1 }, /* sc */
    { TRAMP_SENTINEL_INSN },
  },
  ppc64_linux_sighandler_cache_init
};

/* Return 1 if PPC_ORIG_R3_REGNUM and PPC_TRAP_REGNUM are usable.  */
int
ppc_linux_trap_reg_p (struct gdbarch *gdbarch)
{
  /* If we do not have a target description with registers, then
     the special registers will not be included in the register set.  */
  if (!tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return 0;

  /* If we do, then it is safe to check the size.  */
  return register_size (gdbarch, PPC_ORIG_R3_REGNUM) > 0
         && register_size (gdbarch, PPC_TRAP_REGNUM) > 0;
}

/* Return the current system call's number present in the
   r0 register.  When the function fails, it returns -1.  */
static LONGEST
ppc_linux_get_syscall_number (struct gdbarch *gdbarch,
                              ptid_t ptid)
{
  struct regcache *regcache = get_thread_regcache (ptid);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct cleanup *cleanbuf;
  /* The content of a register */
  gdb_byte *buf;
  /* The result */
  LONGEST ret;

  /* Make sure we're in a 32- or 64-bit machine */
  gdb_assert (tdep->wordsize == 4 || tdep->wordsize == 8);

  buf = (gdb_byte *) xmalloc (tdep->wordsize * sizeof (gdb_byte));

  cleanbuf = make_cleanup (xfree, buf);

  /* Getting the system call number from the register.
     When dealing with PowerPC architecture, this information
     is stored at 0th register.  */
  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum, buf);

  ret = extract_signed_integer (buf, tdep->wordsize, byte_order);
  do_cleanups (cleanbuf);

  return ret;
}

/* PPC process record-replay */

static struct linux_record_tdep ppc_linux_record_tdep;
static struct linux_record_tdep ppc64_linux_record_tdep;

/* ppc_canonicalize_syscall maps from the native PowerPC Linux set of
   syscall ids into a canonical set of syscall ids used by process
   record.  (See arch/powerpc/include/uapi/asm/unistd.h in kernel tree.)
   Return -1 if this system call is not supported by process record.
   Otherwise, return the syscall number for preocess reocrd of given
   SYSCALL.  */

static enum gdb_syscall
ppc_canonicalize_syscall (int syscall)
{
  int result = -1;

  if (syscall <= 165)
    result = syscall;
  else if (syscall >= 167 && syscall <= 190)	/* Skip query_module 166 */
    result = syscall + 1;
  else if (syscall >= 192 && syscall <= 197)	/* mmap2 */
    result = syscall;
  else if (syscall == 208)			/* tkill */
    result = gdb_sys_tkill;
  else if (syscall >= 207 && syscall <= 220)	/* gettid */
    result = syscall + 224 - 207;
  else if (syscall >= 234 && syscall <= 239)	/* exit_group */
    result = syscall + 252 - 234;
  else if (syscall >= 240 && syscall <= 248)	/* timer_create */
    result = syscall += 259 - 240;
  else if (syscall >= 250 && syscall <= 251)	/* tgkill */
    result = syscall + 270 - 250;
  else if (syscall == 336)
    result = gdb_sys_recv;
  else if (syscall == 337)
    result = gdb_sys_recvfrom;
  else if (syscall == 342)
    result = gdb_sys_recvmsg;

  return (enum gdb_syscall) result;
}

/* Record registers which might be clobbered during system call.
   Return 0 if successful.  */

static int
ppc_linux_syscall_record (struct regcache *regcache)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  ULONGEST scnum;
  enum gdb_syscall syscall_gdb;
  int ret;
  int i;

  regcache_raw_read_unsigned (regcache, tdep->ppc_gp0_regnum, &scnum);
  syscall_gdb = ppc_canonicalize_syscall (scnum);

  if (syscall_gdb < 0)
    {
      printf_unfiltered (_("Process record and replay target doesn't "
			   "support syscall number %d\n"), (int) scnum);
      return 0;
    }

  if (syscall_gdb == gdb_sys_sigreturn
      || syscall_gdb == gdb_sys_rt_sigreturn)
   {
     int i, j;
     int regsets[] = { tdep->ppc_gp0_regnum,
		       tdep->ppc_fp0_regnum,
		       tdep->ppc_vr0_regnum,
		       tdep->ppc_vsr0_upper_regnum };

     for (j = 0; j < 4; j++)
       {
	 if (regsets[j] == -1)
	   continue;
	 for (i = 0; i < 32; i++)
	   {
	     if (record_full_arch_list_add_reg (regcache, regsets[j] + i))
	       return -1;
	   }
       }

     if (record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum))
       return -1;
     if (record_full_arch_list_add_reg (regcache, tdep->ppc_ctr_regnum))
       return -1;
     if (record_full_arch_list_add_reg (regcache, tdep->ppc_lr_regnum))
       return -1;
     if (record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum))
       return -1;

     return 0;
   }

  if (tdep->wordsize == 8)
    ret = record_linux_system_call (syscall_gdb, regcache,
				    &ppc64_linux_record_tdep);
  else
    ret = record_linux_system_call (syscall_gdb, regcache,
				    &ppc_linux_record_tdep);

  if (ret != 0)
    return ret;

  /* Record registers clobbered during syscall.  */
  for (i = 3; i <= 12; i++)
    {
      if (record_full_arch_list_add_reg (regcache, tdep->ppc_gp0_regnum + i))
	return -1;
    }
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_gp0_regnum + 0))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_ctr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_lr_regnum))
    return -1;

  return 0;
}

/* Record registers which might be clobbered during signal handling.
   Return 0 if successful.  */

static int
ppc_linux_record_signal (struct gdbarch *gdbarch, struct regcache *regcache,
			 enum gdb_signal signal)
{
  /* See handle_rt_signal64 in arch/powerpc/kernel/signal_64.c
	 handle_rt_signal32 in arch/powerpc/kernel/signal_32.c
	 arch/powerpc/include/asm/ptrace.h
     for details.  */
  const int SIGNAL_FRAMESIZE = 128;
  const int sizeof_rt_sigframe = 1440 * 2 + 8 * 2 + 4 * 6 + 8 + 8 + 128 + 512;
  ULONGEST sp;
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int i;

  for (i = 3; i <= 12; i++)
    {
      if (record_full_arch_list_add_reg (regcache, tdep->ppc_gp0_regnum + i))
	return -1;
    }

  if (record_full_arch_list_add_reg (regcache, tdep->ppc_lr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_ctr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, gdbarch_pc_regnum (gdbarch)))
    return -1;
  if (record_full_arch_list_add_reg (regcache, gdbarch_sp_regnum (gdbarch)))
    return -1;

  /* Record the change in the stack.
     frame-size = sizeof (struct rt_sigframe) + SIGNAL_FRAMESIZE  */
  regcache_raw_read_unsigned (regcache, gdbarch_sp_regnum (gdbarch), &sp);
  sp -= SIGNAL_FRAMESIZE;
  sp -= sizeof_rt_sigframe;

  if (record_full_arch_list_add_mem (sp, SIGNAL_FRAMESIZE + sizeof_rt_sigframe))
    return -1;

  if (record_full_arch_list_add_end ())
    return -1;

  return 0;
}

static void
ppc_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  regcache_cooked_write_unsigned (regcache, gdbarch_pc_regnum (gdbarch), pc);

  /* Set special TRAP register to -1 to prevent the kernel from
     messing with the PC we just installed, if we happen to be
     within an interrupted system call that the kernel wants to
     restart.

     Note that after we return from the dummy call, the TRAP and
     ORIG_R3 registers will be automatically restored, and the
     kernel continues to restart the system call at this point.  */
  if (ppc_linux_trap_reg_p (gdbarch))
    regcache_cooked_write_unsigned (regcache, PPC_TRAP_REGNUM, -1);
}

static int
ppc_linux_spu_section (bfd *abfd, asection *asect, void *user_data)
{
  return startswith (bfd_section_name (abfd, asect), "SPU/");
}

static const struct target_desc *
ppc_linux_core_read_description (struct gdbarch *gdbarch,
				 struct target_ops *target,
				 bfd *abfd)
{
  asection *cell = bfd_sections_find_if (abfd, ppc_linux_spu_section, NULL);
  asection *altivec = bfd_get_section_by_name (abfd, ".reg-ppc-vmx");
  asection *vsx = bfd_get_section_by_name (abfd, ".reg-ppc-vsx");
  asection *section = bfd_get_section_by_name (abfd, ".reg");
  if (! section)
    return NULL;

  switch (bfd_section_size (abfd, section))
    {
    case 48 * 4:
      if (cell)
	return tdesc_powerpc_cell32l;
      else if (vsx)
	return tdesc_powerpc_vsx32l;
      else if (altivec)
	return tdesc_powerpc_altivec32l;
      else
	return tdesc_powerpc_32l;

    case 48 * 8:
      if (cell)
	return tdesc_powerpc_cell64l;
      else if (vsx)
	return tdesc_powerpc_vsx64l;
      else if (altivec)
	return tdesc_powerpc_altivec64l;
      else
	return tdesc_powerpc_64l;

    default:
      return NULL;
    }
}


/* Implementation of `gdbarch_elf_make_msymbol_special', as defined in
   gdbarch.h.  This implementation is used for the ELFv2 ABI only.  */

static void
ppc_elfv2_elf_make_msymbol_special (asymbol *sym, struct minimal_symbol *msym)
{
  elf_symbol_type *elf_sym = (elf_symbol_type *)sym;

  /* If the symbol is marked as having a local entry point, set a target
     flag in the msymbol.  We currently only support local entry point
     offsets of 8 bytes, which is the only entry point offset ever used
     by current compilers.  If/when other offsets are ever used, we will
     have to use additional target flag bits to store them.  */
  switch (PPC64_LOCAL_ENTRY_OFFSET (elf_sym->internal_elf_sym.st_other))
    {
    default:
      break;
    case 8:
      MSYMBOL_TARGET_FLAG_1 (msym) = 1;
      break;
    }
}

/* Implementation of `gdbarch_skip_entrypoint', as defined in
   gdbarch.h.  This implementation is used for the ELFv2 ABI only.  */

static CORE_ADDR
ppc_elfv2_skip_entrypoint (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct bound_minimal_symbol fun;
  int local_entry_offset = 0;

  fun = lookup_minimal_symbol_by_pc (pc);
  if (fun.minsym == NULL)
    return pc;

  /* See ppc_elfv2_elf_make_msymbol_special for how local entry point
     offset values are encoded.  */
  if (MSYMBOL_TARGET_FLAG_1 (fun.minsym))
    local_entry_offset = 8;

  if (BMSYMBOL_VALUE_ADDRESS (fun) <= pc
      && pc < BMSYMBOL_VALUE_ADDRESS (fun) + local_entry_offset)
    return BMSYMBOL_VALUE_ADDRESS (fun) + local_entry_offset;

  return pc;
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
ppc_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return (*s == 'i' /* Literal number.  */
	  || (isdigit (*s) && s[1] == '('
	      && isdigit (s[2])) /* Displacement.  */
	  || (*s == '(' && isdigit (s[1])) /* Register indirection.  */
	  || isdigit (*s)); /* Register value.  */
}

/* Implementation of `gdbarch_stap_parse_special_token', as defined in
   gdbarch.h.  */

static int
ppc_stap_parse_special_token (struct gdbarch *gdbarch,
			      struct stap_parse_info *p)
{
  if (isdigit (*p->arg))
    {
      /* This temporary pointer is needed because we have to do a lookahead.
	  We could be dealing with a register displacement, and in such case
	  we would not need to do anything.  */
      const char *s = p->arg;
      char *regname;
      int len;
      struct stoken str;

      while (isdigit (*s))
	++s;

      if (*s == '(')
	{
	  /* It is a register displacement indeed.  Returning 0 means we are
	     deferring the treatment of this case to the generic parser.  */
	  return 0;
	}

      len = s - p->arg;
      regname = (char *) alloca (len + 2);
      regname[0] = 'r';

      strncpy (regname + 1, p->arg, len);
      ++len;
      regname[len] = '\0';

      if (user_reg_map_name_to_regnum (gdbarch, regname, len) == -1)
	error (_("Invalid register name `%s' on expression `%s'."),
	       regname, p->saved_arg);

      write_exp_elt_opcode (&p->pstate, OP_REGISTER);
      str.ptr = regname;
      str.length = len;
      write_exp_string (&p->pstate, str);
      write_exp_elt_opcode (&p->pstate, OP_REGISTER);

      p->arg = s;
    }
  else
    {
      /* All the other tokens should be handled correctly by the generic
	 parser.  */
      return 0;
    }

  return 1;
}

/* Cell/B.E. active SPE context tracking support.  */

static struct objfile *spe_context_objfile = NULL;
static CORE_ADDR spe_context_lm_addr = 0;
static CORE_ADDR spe_context_offset = 0;

static ptid_t spe_context_cache_ptid;
static CORE_ADDR spe_context_cache_address;

/* Hook into inferior_created, solib_loaded, and solib_unloaded observers
   to track whether we've loaded a version of libspe2 (as static or dynamic
   library) that provides the __spe_current_active_context variable.  */
static void
ppc_linux_spe_context_lookup (struct objfile *objfile)
{
  struct bound_minimal_symbol sym;

  if (!objfile)
    {
      spe_context_objfile = NULL;
      spe_context_lm_addr = 0;
      spe_context_offset = 0;
      spe_context_cache_ptid = minus_one_ptid;
      spe_context_cache_address = 0;
      return;
    }

  sym = lookup_minimal_symbol ("__spe_current_active_context", NULL, objfile);
  if (sym.minsym)
    {
      spe_context_objfile = objfile;
      spe_context_lm_addr = svr4_fetch_objfile_link_map (objfile);
      spe_context_offset = MSYMBOL_VALUE_RAW_ADDRESS (sym.minsym);
      spe_context_cache_ptid = minus_one_ptid;
      spe_context_cache_address = 0;
      return;
    }
}

static void
ppc_linux_spe_context_inferior_created (struct target_ops *t, int from_tty)
{
  struct objfile *objfile;

  ppc_linux_spe_context_lookup (NULL);
  ALL_OBJFILES (objfile)
    ppc_linux_spe_context_lookup (objfile);
}

static void
ppc_linux_spe_context_solib_loaded (struct so_list *so)
{
  if (strstr (so->so_original_name, "/libspe") != NULL)
    {
      solib_read_symbols (so, 0);
      ppc_linux_spe_context_lookup (so->objfile);
    }
}

static void
ppc_linux_spe_context_solib_unloaded (struct so_list *so)
{
  if (so->objfile == spe_context_objfile)
    ppc_linux_spe_context_lookup (NULL);
}

/* Retrieve contents of the N'th element in the current thread's
   linked SPE context list into ID and NPC.  Return the address of
   said context element, or 0 if not found.  */
static CORE_ADDR
ppc_linux_spe_context (int wordsize, enum bfd_endian byte_order,
		       int n, int *id, unsigned int *npc)
{
  CORE_ADDR spe_context = 0;
  gdb_byte buf[16];
  int i;

  /* Quick exit if we have not found __spe_current_active_context.  */
  if (!spe_context_objfile)
    return 0;

  /* Look up cached address of thread-local variable.  */
  if (!ptid_equal (spe_context_cache_ptid, inferior_ptid))
    {
      struct target_ops *target = &current_target;

      TRY
	{
	  /* We do not call target_translate_tls_address here, because
	     svr4_fetch_objfile_link_map may invalidate the frame chain,
	     which must not do while inside a frame sniffer.

	     Instead, we have cached the lm_addr value, and use that to
	     directly call the target's to_get_thread_local_address.  */
	  spe_context_cache_address
	    = target->to_get_thread_local_address (target, inferior_ptid,
						   spe_context_lm_addr,
						   spe_context_offset);
	  spe_context_cache_ptid = inferior_ptid;
	}

      CATCH (ex, RETURN_MASK_ERROR)
	{
	  return 0;
	}
      END_CATCH
    }

  /* Read variable value.  */
  if (target_read_memory (spe_context_cache_address, buf, wordsize) == 0)
    spe_context = extract_unsigned_integer (buf, wordsize, byte_order);

  /* Cyle through to N'th linked list element.  */
  for (i = 0; i < n && spe_context; i++)
    if (target_read_memory (spe_context + align_up (12, wordsize),
			    buf, wordsize) == 0)
      spe_context = extract_unsigned_integer (buf, wordsize, byte_order);
    else
      spe_context = 0;

  /* Read current context.  */
  if (spe_context
      && target_read_memory (spe_context, buf, 12) != 0)
    spe_context = 0;

  /* Extract data elements.  */
  if (spe_context)
    {
      if (id)
	*id = extract_signed_integer (buf, 4, byte_order);
      if (npc)
	*npc = extract_unsigned_integer (buf + 4, 4, byte_order);
    }

  return spe_context;
}


/* Cell/B.E. cross-architecture unwinder support.  */

struct ppu2spu_cache
{
  struct frame_id frame_id;
  struct regcache *regcache;
};

static struct gdbarch *
ppu2spu_prev_arch (struct frame_info *this_frame, void **this_cache)
{
  struct ppu2spu_cache *cache = (struct ppu2spu_cache *) *this_cache;
  return get_regcache_arch (cache->regcache);
}

static void
ppu2spu_this_id (struct frame_info *this_frame,
		 void **this_cache, struct frame_id *this_id)
{
  struct ppu2spu_cache *cache = (struct ppu2spu_cache *) *this_cache;
  *this_id = cache->frame_id;
}

static struct value *
ppu2spu_prev_register (struct frame_info *this_frame,
		       void **this_cache, int regnum)
{
  struct ppu2spu_cache *cache = (struct ppu2spu_cache *) *this_cache;
  struct gdbarch *gdbarch = get_regcache_arch (cache->regcache);
  gdb_byte *buf;

  buf = (gdb_byte *) alloca (register_size (gdbarch, regnum));

  if (regnum < gdbarch_num_regs (gdbarch))
    regcache_raw_read (cache->regcache, regnum, buf);
  else
    gdbarch_pseudo_register_read (gdbarch, cache->regcache, regnum, buf);

  return frame_unwind_got_bytes (this_frame, regnum, buf);
}

struct ppu2spu_data
{
  struct gdbarch *gdbarch;
  int id;
  unsigned int npc;
  gdb_byte gprs[128*16];
};

static enum register_status
ppu2spu_unwind_register (void *src, int regnum, gdb_byte *buf)
{
  struct ppu2spu_data *data = (struct ppu2spu_data *) src;
  enum bfd_endian byte_order = gdbarch_byte_order (data->gdbarch);

  if (regnum >= 0 && regnum < SPU_NUM_GPRS)
    memcpy (buf, data->gprs + 16*regnum, 16);
  else if (regnum == SPU_ID_REGNUM)
    store_unsigned_integer (buf, 4, byte_order, data->id);
  else if (regnum == SPU_PC_REGNUM)
    store_unsigned_integer (buf, 4, byte_order, data->npc);
  else
    return REG_UNAVAILABLE;

  return REG_VALID;
}

static int
ppu2spu_sniffer (const struct frame_unwind *self,
		 struct frame_info *this_frame, void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct ppu2spu_data data;
  struct frame_info *fi;
  CORE_ADDR base, func, backchain, spe_context;
  gdb_byte buf[8];
  int n = 0;

  /* Count the number of SPU contexts already in the frame chain.  */
  for (fi = get_next_frame (this_frame); fi; fi = get_next_frame (fi))
    if (get_frame_type (fi) == ARCH_FRAME
	&& gdbarch_bfd_arch_info (get_frame_arch (fi))->arch == bfd_arch_spu)
      n++;

  base = get_frame_sp (this_frame);
  func = get_frame_pc (this_frame);
  if (target_read_memory (base, buf, tdep->wordsize))
    return 0;
  backchain = extract_unsigned_integer (buf, tdep->wordsize, byte_order);

  spe_context = ppc_linux_spe_context (tdep->wordsize, byte_order,
				       n, &data.id, &data.npc);
  if (spe_context && base <= spe_context && spe_context < backchain)
    {
      char annex[32];

      /* Find gdbarch for SPU.  */
      struct gdbarch_info info;
      gdbarch_info_init (&info);
      info.bfd_arch_info = bfd_lookup_arch (bfd_arch_spu, bfd_mach_spu);
      info.byte_order = BFD_ENDIAN_BIG;
      info.osabi = GDB_OSABI_LINUX;
      info.tdep_info = &data.id;
      data.gdbarch = gdbarch_find_by_info (info);
      if (!data.gdbarch)
	return 0;

      xsnprintf (annex, sizeof annex, "%d/regs", data.id);
      if (target_read (&current_target, TARGET_OBJECT_SPU, annex,
		       data.gprs, 0, sizeof data.gprs)
	  == sizeof data.gprs)
	{
	  struct ppu2spu_cache *cache
	    = FRAME_OBSTACK_CALLOC (1, struct ppu2spu_cache);

	  struct address_space *aspace = get_frame_address_space (this_frame);
	  struct regcache *regcache = regcache_xmalloc (data.gdbarch, aspace);
	  struct cleanup *cleanups = make_cleanup_regcache_xfree (regcache);
	  regcache_save (regcache, ppu2spu_unwind_register, &data);
	  discard_cleanups (cleanups);

	  cache->frame_id = frame_id_build (base, func);
	  cache->regcache = regcache;
	  *this_prologue_cache = cache;
	  return 1;
	}
    }

  return 0;
}

static void
ppu2spu_dealloc_cache (struct frame_info *self, void *this_cache)
{
  struct ppu2spu_cache *cache = (struct ppu2spu_cache *) this_cache;
  regcache_xfree (cache->regcache);
}

static const struct frame_unwind ppu2spu_unwind = {
  ARCH_FRAME,
  default_frame_unwind_stop_reason,
  ppu2spu_this_id,
  ppu2spu_prev_register,
  NULL,
  ppu2spu_sniffer,
  ppu2spu_dealloc_cache,
  ppu2spu_prev_arch,
};

/* Initialize linux_record_tdep if not initialized yet.
   WORDSIZE is 4 or 8 for 32- or 64-bit PowerPC Linux respectively.
   Sizes of data structures are initialized accordingly.  */

static void
ppc_init_linux_record_tdep (struct linux_record_tdep *record_tdep,
			    int wordsize)
{
  /* Simply return if it had been initialized.  */
  if (record_tdep->size_pointer != 0)
    return;

  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */

  if (wordsize == 8)
    {
      record_tdep->size_pointer = 8;
      record_tdep->size__old_kernel_stat = 32;
      record_tdep->size_tms = 32;
      record_tdep->size_loff_t = 8;
      record_tdep->size_flock = 32;
      record_tdep->size_oldold_utsname = 45;
      record_tdep->size_ustat = 32;
      record_tdep->size_old_sigaction = 32;
      record_tdep->size_old_sigset_t = 8;
      record_tdep->size_rlimit = 16;
      record_tdep->size_rusage = 144;
      record_tdep->size_timeval = 16;
      record_tdep->size_timezone = 8;
      record_tdep->size_old_gid_t = 4;
      record_tdep->size_old_uid_t = 4;
      record_tdep->size_fd_set = 128;
      record_tdep->size_old_dirent = 280;
      record_tdep->size_statfs = 120;
      record_tdep->size_statfs64 = 120;
      record_tdep->size_sockaddr = 16;
      record_tdep->size_int = 4;
      record_tdep->size_long = 8;
      record_tdep->size_ulong = 8;
      record_tdep->size_msghdr = 56;
      record_tdep->size_itimerval = 32;
      record_tdep->size_stat = 144;
      record_tdep->size_old_utsname = 325;
      record_tdep->size_sysinfo = 112;
      record_tdep->size_msqid_ds = 120;
      record_tdep->size_shmid_ds = 112;
      record_tdep->size_new_utsname = 390;
      record_tdep->size_timex = 208;
      record_tdep->size_mem_dqinfo = 24;
      record_tdep->size_if_dqblk = 72;
      record_tdep->size_fs_quota_stat = 80;
      record_tdep->size_timespec = 16;
      record_tdep->size_pollfd = 8;
      record_tdep->size_NFS_FHSIZE = 32;
      record_tdep->size_knfsd_fh = 132;
      record_tdep->size_TASK_COMM_LEN = 16;
      record_tdep->size_sigaction = 32;
      record_tdep->size_sigset_t = 8;
      record_tdep->size_siginfo_t = 128;
      record_tdep->size_cap_user_data_t = 8;
      record_tdep->size_stack_t = 24;
      record_tdep->size_off_t = 8;
      record_tdep->size_stat64 = 104;
      record_tdep->size_gid_t = 4;
      record_tdep->size_uid_t = 4;
      record_tdep->size_PAGE_SIZE = 0x10000;	/* 64KB */
      record_tdep->size_flock64 = 32;
      record_tdep->size_io_event = 32;
      record_tdep->size_iocb = 64;
      record_tdep->size_epoll_event = 16;
      record_tdep->size_itimerspec = 32;
      record_tdep->size_mq_attr = 64;
      record_tdep->size_termios = 44;
      record_tdep->size_pid_t = 4;
      record_tdep->size_winsize = 8;
      record_tdep->size_serial_struct = 72;
      record_tdep->size_serial_icounter_struct = 80;
      record_tdep->size_size_t = 8;
      record_tdep->size_iovec = 16;
      record_tdep->size_time_t = 8;
    }
  else if (wordsize == 4)
    {
      record_tdep->size_pointer = 4;
      record_tdep->size__old_kernel_stat = 32;
      record_tdep->size_tms = 16;
      record_tdep->size_loff_t = 8;
      record_tdep->size_flock = 16;
      record_tdep->size_oldold_utsname = 45;
      record_tdep->size_ustat = 20;
      record_tdep->size_old_sigaction = 16;
      record_tdep->size_old_sigset_t = 4;
      record_tdep->size_rlimit = 8;
      record_tdep->size_rusage = 72;
      record_tdep->size_timeval = 8;
      record_tdep->size_timezone = 8;
      record_tdep->size_old_gid_t = 4;
      record_tdep->size_old_uid_t = 4;
      record_tdep->size_fd_set = 128;
      record_tdep->size_old_dirent = 268;
      record_tdep->size_statfs = 64;
      record_tdep->size_statfs64 = 88;
      record_tdep->size_sockaddr = 16;
      record_tdep->size_int = 4;
      record_tdep->size_long = 4;
      record_tdep->size_ulong = 4;
      record_tdep->size_msghdr = 28;
      record_tdep->size_itimerval = 16;
      record_tdep->size_stat = 88;
      record_tdep->size_old_utsname = 325;
      record_tdep->size_sysinfo = 64;
      record_tdep->size_msqid_ds = 68;
      record_tdep->size_shmid_ds = 60;
      record_tdep->size_new_utsname = 390;
      record_tdep->size_timex = 128;
      record_tdep->size_mem_dqinfo = 24;
      record_tdep->size_if_dqblk = 72;
      record_tdep->size_fs_quota_stat = 80;
      record_tdep->size_timespec = 8;
      record_tdep->size_pollfd = 8;
      record_tdep->size_NFS_FHSIZE = 32;
      record_tdep->size_knfsd_fh = 132;
      record_tdep->size_TASK_COMM_LEN = 16;
      record_tdep->size_sigaction = 20;
      record_tdep->size_sigset_t = 8;
      record_tdep->size_siginfo_t = 128;
      record_tdep->size_cap_user_data_t = 4;
      record_tdep->size_stack_t = 12;
      record_tdep->size_off_t = 4;
      record_tdep->size_stat64 = 104;
      record_tdep->size_gid_t = 4;
      record_tdep->size_uid_t = 4;
      record_tdep->size_PAGE_SIZE = 0x10000;	/* 64KB */
      record_tdep->size_flock64 = 32;
      record_tdep->size_io_event = 32;
      record_tdep->size_iocb = 64;
      record_tdep->size_epoll_event = 16;
      record_tdep->size_itimerspec = 16;
      record_tdep->size_mq_attr = 32;
      record_tdep->size_termios = 44;
      record_tdep->size_pid_t = 4;
      record_tdep->size_winsize = 8;
      record_tdep->size_serial_struct = 60;
      record_tdep->size_serial_icounter_struct = 80;
      record_tdep->size_size_t = 4;
      record_tdep->size_iovec = 8;
      record_tdep->size_time_t = 4;
    }
  else
    internal_error (__FILE__, __LINE__, _("unexpected wordsize"));

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  record_tdep->fcntl_F_GETLK = 5;
  record_tdep->fcntl_F_GETLK64 = 12;
  record_tdep->fcntl_F_SETLK64 = 13;
  record_tdep->fcntl_F_SETLKW64 = 14;

  record_tdep->arg1 = PPC_R0_REGNUM + 3;
  record_tdep->arg2 = PPC_R0_REGNUM + 4;
  record_tdep->arg3 = PPC_R0_REGNUM + 5;
  record_tdep->arg4 = PPC_R0_REGNUM + 6;
  record_tdep->arg5 = PPC_R0_REGNUM + 7;
  record_tdep->arg6 = PPC_R0_REGNUM + 8;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.
     See arch/powerpc/include/uapi/asm/ioctls.h.  */
  record_tdep->ioctl_TCGETS = 0x403c7413;
  record_tdep->ioctl_TCSETS = 0x803c7414;
  record_tdep->ioctl_TCSETSW = 0x803c7415;
  record_tdep->ioctl_TCSETSF = 0x803c7416;
  record_tdep->ioctl_TCGETA = 0x40147417;
  record_tdep->ioctl_TCSETA = 0x80147418;
  record_tdep->ioctl_TCSETAW = 0x80147419;
  record_tdep->ioctl_TCSETAF = 0x8014741c;
  record_tdep->ioctl_TCSBRK = 0x2000741d;
  record_tdep->ioctl_TCXONC = 0x2000741e;
  record_tdep->ioctl_TCFLSH = 0x2000741f;
  record_tdep->ioctl_TIOCEXCL = 0x540c;
  record_tdep->ioctl_TIOCNXCL = 0x540d;
  record_tdep->ioctl_TIOCSCTTY = 0x540e;
  record_tdep->ioctl_TIOCGPGRP = 0x40047477;
  record_tdep->ioctl_TIOCSPGRP = 0x80047476;
  record_tdep->ioctl_TIOCOUTQ = 0x40047473;
  record_tdep->ioctl_TIOCSTI = 0x5412;
  record_tdep->ioctl_TIOCGWINSZ = 0x40087468;
  record_tdep->ioctl_TIOCSWINSZ = 0x80087467;
  record_tdep->ioctl_TIOCMGET = 0x5415;
  record_tdep->ioctl_TIOCMBIS = 0x5416;
  record_tdep->ioctl_TIOCMBIC = 0x5417;
  record_tdep->ioctl_TIOCMSET = 0x5418;
  record_tdep->ioctl_TIOCGSOFTCAR = 0x5419;
  record_tdep->ioctl_TIOCSSOFTCAR = 0x541a;
  record_tdep->ioctl_FIONREAD = 0x4004667f;
  record_tdep->ioctl_TIOCINQ = 0x4004667f;
  record_tdep->ioctl_TIOCLINUX = 0x541c;
  record_tdep->ioctl_TIOCCONS = 0x541d;
  record_tdep->ioctl_TIOCGSERIAL = 0x541e;
  record_tdep->ioctl_TIOCSSERIAL = 0x541f;
  record_tdep->ioctl_TIOCPKT = 0x5420;
  record_tdep->ioctl_FIONBIO = 0x8004667e;
  record_tdep->ioctl_TIOCNOTTY = 0x5422;
  record_tdep->ioctl_TIOCSETD = 0x5423;
  record_tdep->ioctl_TIOCGETD = 0x5424;
  record_tdep->ioctl_TCSBRKP = 0x5425;
  record_tdep->ioctl_TIOCSBRK = 0x5427;
  record_tdep->ioctl_TIOCCBRK = 0x5428;
  record_tdep->ioctl_TIOCGSID = 0x5429;
  record_tdep->ioctl_TIOCGPTN = 0x40045430;
  record_tdep->ioctl_TIOCSPTLCK = 0x80045431;
  record_tdep->ioctl_FIONCLEX = 0x20006602;
  record_tdep->ioctl_FIOCLEX = 0x20006601;
  record_tdep->ioctl_FIOASYNC = 0x8004667d;
  record_tdep->ioctl_TIOCSERCONFIG = 0x5453;
  record_tdep->ioctl_TIOCSERGWILD = 0x5454;
  record_tdep->ioctl_TIOCSERSWILD = 0x5455;
  record_tdep->ioctl_TIOCGLCKTRMIOS = 0x5456;
  record_tdep->ioctl_TIOCSLCKTRMIOS = 0x5457;
  record_tdep->ioctl_TIOCSERGSTRUCT = 0x5458;
  record_tdep->ioctl_TIOCSERGETLSR = 0x5459;
  record_tdep->ioctl_TIOCSERGETMULTI = 0x545a;
  record_tdep->ioctl_TIOCSERSETMULTI = 0x545b;
  record_tdep->ioctl_TIOCMIWAIT = 0x545c;
  record_tdep->ioctl_TIOCGICOUNT = 0x545d;
  record_tdep->ioctl_FIOQSIZE = 0x40086680;
}

/* Return a floating-point format for a floating-point variable of
   length LEN in bits.  If non-NULL, NAME is the name of its type.
   If no suitable type is found, return NULL.  */

const struct floatformat **
ppc_floatformat_for_type (struct gdbarch *gdbarch,
                          const char *name, int len)
{
  if (len == 128 && name)
    if (strcmp (name, "__float128") == 0
        || strcmp (name, "_Float128") == 0
        || strcmp (name, "_Float64x") == 0
        || strcmp (name, "complex _Float128") == 0
        || strcmp (name, "complex _Float64x") == 0)
      return floatformats_ia64_quad;

  return default_floatformat_for_type (gdbarch, name, len);
}

static void
ppc_linux_init_abi (struct gdbarch_info info,
                    struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  struct tdesc_arch_data *tdesc_data
    = (struct tdesc_arch_data *) info.tdep_info;
  static const char *const stap_integer_prefixes[] = { "i", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "(",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { ")",
								    NULL };

  linux_init_abi (info, gdbarch);

  /* PPC GNU/Linux uses either 64-bit or 128-bit long doubles; where
     128-bit, they are IBM long double, not IEEE quad long double as
     in the System V ABI PowerPC Processor Supplement.  We can safely
     let them default to 128-bit, since the debug info will give the
     size of type actually used in each case.  */
  set_gdbarch_long_double_bit (gdbarch, 16 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_format (gdbarch, floatformats_ibm_long_double);

  /* Support for floating-point data type variants.  */
  set_gdbarch_floatformat_for_type (gdbarch, ppc_floatformat_for_type);

  /* Handle inferior calls during interrupted system calls.  */
  set_gdbarch_write_pc (gdbarch, ppc_linux_write_pc);

  /* Get the syscall number from the arch's register.  */
  set_gdbarch_get_syscall_number (gdbarch, ppc_linux_get_syscall_number);

  /* SystemTap functions.  */
  set_gdbarch_stap_integer_prefixes (gdbarch, stap_integer_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					  stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					  stap_register_indirection_suffixes);
  set_gdbarch_stap_gdb_register_prefix (gdbarch, "r");
  set_gdbarch_stap_is_single_operand (gdbarch, ppc_stap_is_single_operand);
  set_gdbarch_stap_parse_special_token (gdbarch,
					ppc_stap_parse_special_token);

  if (tdep->wordsize == 4)
    {
      /* Until November 2001, gcc did not comply with the 32 bit SysV
	 R4 ABI requirement that structures less than or equal to 8
	 bytes should be returned in registers.  Instead GCC was using
	 the AIX/PowerOpen ABI - everything returned in memory
	 (well ignoring vectors that is).  When this was corrected, it
	 wasn't fixed for GNU/Linux native platform.  Use the
	 PowerOpen struct convention.  */
      set_gdbarch_return_value (gdbarch, ppc_linux_return_value);

      set_gdbarch_memory_remove_breakpoint (gdbarch,
                                            ppc_linux_memory_remove_breakpoint);

      /* Shared library handling.  */
      set_gdbarch_skip_trampoline_code (gdbarch, ppc_skip_trampoline_code);
      set_solib_svr4_fetch_link_map_offsets
        (gdbarch, svr4_ilp32_fetch_link_map_offsets);

      /* Setting the correct XML syscall filename.  */
      set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_PPC);

      /* Trampolines.  */
      tramp_frame_prepend_unwinder (gdbarch,
				    &ppc32_linux_sigaction_tramp_frame);
      tramp_frame_prepend_unwinder (gdbarch,
				    &ppc32_linux_sighandler_tramp_frame);

      /* BFD target for core files.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
	set_gdbarch_gcore_bfd_target (gdbarch, "elf32-powerpcle");
      else
	set_gdbarch_gcore_bfd_target (gdbarch, "elf32-powerpc");

      if (powerpc_so_ops.in_dynsym_resolve_code == NULL)
	{
	  powerpc_so_ops = svr4_so_ops;
	  /* Override dynamic resolve function.  */
	  powerpc_so_ops.in_dynsym_resolve_code =
	    powerpc_linux_in_dynsym_resolve_code;
	}
      set_solib_ops (gdbarch, &powerpc_so_ops);

      set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);
    }
  
  if (tdep->wordsize == 8)
    {
      if (tdep->elf_abi == POWERPC_ELF_V1)
	{
	  /* Handle PPC GNU/Linux 64-bit function pointers (which are really
	     function descriptors).  */
	  set_gdbarch_convert_from_func_ptr_addr
	    (gdbarch, ppc64_convert_from_func_ptr_addr);

	  set_gdbarch_elf_make_msymbol_special
	    (gdbarch, ppc64_elf_make_msymbol_special);
	}
      else
	{
	  set_gdbarch_elf_make_msymbol_special
	    (gdbarch, ppc_elfv2_elf_make_msymbol_special);

	  set_gdbarch_skip_entrypoint (gdbarch, ppc_elfv2_skip_entrypoint);
	}

      /* Shared library handling.  */
      set_gdbarch_skip_trampoline_code (gdbarch, ppc64_skip_trampoline_code);
      set_solib_svr4_fetch_link_map_offsets
        (gdbarch, svr4_lp64_fetch_link_map_offsets);

      /* Setting the correct XML syscall filename.  */
      set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_PPC64);

      /* Trampolines.  */
      tramp_frame_prepend_unwinder (gdbarch,
				    &ppc64_linux_sigaction_tramp_frame);
      tramp_frame_prepend_unwinder (gdbarch,
				    &ppc64_linux_sighandler_tramp_frame);

      /* BFD target for core files.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
	set_gdbarch_gcore_bfd_target (gdbarch, "elf64-powerpcle");
      else
	set_gdbarch_gcore_bfd_target (gdbarch, "elf64-powerpc");
    }

  /* PPC32 uses a different prpsinfo32 compared to most other Linux
     archs.  */
  if (tdep->wordsize == 4)
    set_gdbarch_elfcore_write_linux_prpsinfo (gdbarch,
					      elfcore_write_ppc_linux_prpsinfo32);

  set_gdbarch_core_read_description (gdbarch, ppc_linux_core_read_description);
  set_gdbarch_iterate_over_regset_sections (gdbarch,
					    ppc_linux_iterate_over_regset_sections);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
                                             svr4_fetch_objfile_link_map);

  if (tdesc_data)
    {
      const struct tdesc_feature *feature;

      /* If we have target-described registers, then we can safely
         reserve a number for PPC_ORIG_R3_REGNUM and PPC_TRAP_REGNUM
	 (whether they are described or not).  */
      gdb_assert (gdbarch_num_regs (gdbarch) <= PPC_ORIG_R3_REGNUM);
      set_gdbarch_num_regs (gdbarch, PPC_TRAP_REGNUM + 1);

      /* If they are present, then assign them to the reserved number.  */
      feature = tdesc_find_feature (info.target_desc,
                                    "org.gnu.gdb.power.linux");
      if (feature != NULL)
	{
	  tdesc_numbered_register (feature, tdesc_data,
				   PPC_ORIG_R3_REGNUM, "orig_r3");
	  tdesc_numbered_register (feature, tdesc_data,
				   PPC_TRAP_REGNUM, "trap");
	}
    }

  /* Enable Cell/B.E. if supported by the target.  */
  if (tdesc_compatible_p (info.target_desc,
			  bfd_lookup_arch (bfd_arch_spu, bfd_mach_spu)))
    {
      /* Cell/B.E. multi-architecture support.  */
      set_spu_solib_ops (gdbarch);

      /* Cell/B.E. cross-architecture unwinder support.  */
      frame_unwind_prepend_unwinder (gdbarch, &ppu2spu_unwind);
    }

  set_gdbarch_displaced_step_location (gdbarch,
				       linux_displaced_step_location);

  /* Support reverse debugging.  */
  set_gdbarch_process_record (gdbarch, ppc_process_record);
  set_gdbarch_process_record_signal (gdbarch, ppc_linux_record_signal);
  tdep->ppc_syscall_record = ppc_linux_syscall_record;

  ppc_init_linux_record_tdep (&ppc_linux_record_tdep, 4);
  ppc_init_linux_record_tdep (&ppc64_linux_record_tdep, 8);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_ppc_linux_tdep;

void
_initialize_ppc_linux_tdep (void)
{
  /* Register for all sub-familes of the POWER/PowerPC: 32-bit and
     64-bit PowerPC, and the older rs6k.  */
  gdbarch_register_osabi (bfd_arch_powerpc, bfd_mach_ppc, GDB_OSABI_LINUX,
                         ppc_linux_init_abi);
  gdbarch_register_osabi (bfd_arch_powerpc, bfd_mach_ppc64, GDB_OSABI_LINUX,
                         ppc_linux_init_abi);
  gdbarch_register_osabi (bfd_arch_rs6000, bfd_mach_rs6k, GDB_OSABI_LINUX,
                         ppc_linux_init_abi);

  /* Attach to observers to track __spe_current_active_context.  */
  observer_attach_inferior_created (ppc_linux_spe_context_inferior_created);
  observer_attach_solib_loaded (ppc_linux_spe_context_solib_loaded);
  observer_attach_solib_unloaded (ppc_linux_spe_context_solib_unloaded);

  /* Initialize the Linux target descriptions.  */
  initialize_tdesc_powerpc_32l ();
  initialize_tdesc_powerpc_altivec32l ();
  initialize_tdesc_powerpc_cell32l ();
  initialize_tdesc_powerpc_vsx32l ();
  initialize_tdesc_powerpc_isa205_32l ();
  initialize_tdesc_powerpc_isa205_altivec32l ();
  initialize_tdesc_powerpc_isa205_vsx32l ();
  initialize_tdesc_powerpc_64l ();
  initialize_tdesc_powerpc_altivec64l ();
  initialize_tdesc_powerpc_cell64l ();
  initialize_tdesc_powerpc_vsx64l ();
  initialize_tdesc_powerpc_isa205_64l ();
  initialize_tdesc_powerpc_isa205_altivec64l ();
  initialize_tdesc_powerpc_isa205_vsx64l ();
  initialize_tdesc_powerpc_e500l ();
}
