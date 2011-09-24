/* Target-dependent code for GDB, the GNU debugger.

   Copyright (C) 1986, 1987, 1989, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
   2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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
#include "ppc-linux-tdep.h"
#include "trad-frame.h"
#include "frame-unwind.h"
#include "tramp-frame.h"
#include "observer.h"
#include "auxv.h"
#include "elf/common.h"
#include "exceptions.h"
#include "arch-utils.h"
#include "spu-tdep.h"
#include "xml-syscall.h"
#include "linux-tdep.h"

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
  CORE_ADDR addr = bp_tgt->placed_address;
  const unsigned char *bp;
  int val;
  int bplen;
  gdb_byte old_contents[BREAKPOINT_MAX];
  struct cleanup *cleanup;

  /* Determine appropriate breakpoint contents and size for this address.  */
  bp = gdbarch_breakpoint_from_pc (gdbarch, &addr, &bplen);
  if (bp == NULL)
    error (_("Software breakpoints not implemented for this target."));

  /* Make sure we see the memory breakpoints.  */
  cleanup = make_show_memory_breakpoints_cleanup (1);
  val = target_read_memory (addr, old_contents, bplen);

  /* If our breakpoint is no longer at the address, this means that the
     program modified the code on us, so it is wrong to put back the
     old value.  */
  if (val == 0 && memcmp (bp, old_contents, bplen) == 0)
    val = target_write_memory (addr, bp_tgt->shadow_contents, bplen);

  do_cleanups (cleanup);
  return val;
}

/* For historic reasons, PPC 32 GNU/Linux follows PowerOpen rather
   than the 32 bit SYSV R4 ABI structure return convention - all
   structures, no matter their size, are put in memory.  Vectors,
   which were added later, do get returned in a register though.  */

static enum return_value_convention
ppc_linux_return_value (struct gdbarch *gdbarch, struct type *func_type,
			struct type *valtype, struct regcache *regcache,
			gdb_byte *readbuf, const gdb_byte *writebuf)
{  
  if ((TYPE_CODE (valtype) == TYPE_CODE_STRUCT
       || TYPE_CODE (valtype) == TYPE_CODE_UNION)
      && !((TYPE_LENGTH (valtype) == 16 || TYPE_LENGTH (valtype) == 8)
	   && TYPE_VECTOR (valtype)))
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    return ppc_sysv_abi_return_value (gdbarch, func_type, valtype, regcache,
				      readbuf, writebuf);
}

/* Macros for matching instructions.  Note that, since all the
   operands are masked off before they're or-ed into the instruction,
   you can use -1 to make masks.  */

#define insn_d(opcd, rts, ra, d)                \
  ((((opcd) & 0x3f) << 26)                      \
   | (((rts) & 0x1f) << 21)                     \
   | (((ra) & 0x1f) << 16)                      \
   | ((d) & 0xffff))

#define insn_ds(opcd, rts, ra, d, xo)           \
  ((((opcd) & 0x3f) << 26)                      \
   | (((rts) & 0x1f) << 21)                     \
   | (((ra) & 0x1f) << 16)                      \
   | ((d) & 0xfffc)                             \
   | ((xo) & 0x3))

#define insn_xfx(opcd, rts, spr, xo)            \
  ((((opcd) & 0x3f) << 26)                      \
   | (((rts) & 0x1f) << 21)                     \
   | (((spr) & 0x1f) << 16)                     \
   | (((spr) & 0x3e0) << 6)                     \
   | (((xo) & 0x3ff) << 1))

/* Read a PPC instruction from memory.  PPC instructions are always
   big-endian, no matter what endianness the program is running in, so
   we can't use read_memory_integer or one of its friends here.  */
static unsigned int
read_insn (CORE_ADDR pc)
{
  unsigned char buf[4];

  read_memory (pc, buf, 4);
  return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}


/* An instruction to match.  */
struct insn_pattern
{
  unsigned int mask;            /* mask the insn with this...  */
  unsigned int data;            /* ...and see if it matches this.  */
  int optional;                 /* If non-zero, this insn may be absent.  */
};

/* Return non-zero if the instructions at PC match the series
   described in PATTERN, or zero otherwise.  PATTERN is an array of
   'struct insn_pattern' objects, terminated by an entry whose mask is
   zero.

   When the match is successful, fill INSN[i] with what PATTERN[i]
   matched.  If PATTERN[i] is optional, and the instruction wasn't
   present, set INSN[i] to 0 (which is not a valid PPC instruction).
   INSN should have as many elements as PATTERN.  Note that, if
   PATTERN contains optional instructions which aren't present in
   memory, then INSN will have holes, so INSN[i] isn't necessarily the
   i'th instruction in memory.  */
static int
insns_match_pattern (CORE_ADDR pc,
                     struct insn_pattern *pattern,
                     unsigned int *insn)
{
  int i;

  for (i = 0; pattern[i].mask; i++)
    {
      insn[i] = read_insn (pc);
      if ((insn[i] & pattern[i].mask) == pattern[i].data)
        pc += 4;
      else if (pattern[i].optional)
        insn[i] = 0;
      else
        return 0;
    }

  return 1;
}


/* Return the 'd' field of the d-form instruction INSN, properly
   sign-extended.  */
static CORE_ADDR
insn_d_field (unsigned int insn)
{
  return ((((CORE_ADDR) insn & 0xffff) ^ 0x8000) - 0x8000);
}


/* Return the 'ds' field of the ds-form instruction INSN, with the two
   zero bits concatenated at the right, and properly
   sign-extended.  */
static CORE_ADDR
insn_ds_field (unsigned int insn)
{
  return ((((CORE_ADDR) insn & 0xfffc) ^ 0x8000) - 0x8000);
}


/* If DESC is the address of a 64-bit PowerPC GNU/Linux function
   descriptor, return the descriptor's entry point.  */
static CORE_ADDR
ppc64_desc_entry_point (struct gdbarch *gdbarch, CORE_ADDR desc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The first word of the descriptor is the entry point.  */
  return (CORE_ADDR) read_memory_unsigned_integer (desc, 8, byte_order);
}


/* Pattern for the standard linkage function.  These are built by
   build_plt_stub in elf64-ppc.c, whose GLINK argument is always
   zero.  */
static struct insn_pattern ppc64_standard_linkage1[] =
  {
    /* addis r12, r2, <any> */
    { insn_d (-1, -1, -1, 0), insn_d (15, 12, 2, 0), 0 },

    /* std r2, 40(r1) */
    { -1, insn_ds (62, 2, 1, 40, 0), 0 },

    /* ld r11, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 0 },

    /* addis r12, r12, 1 <optional> */
    { insn_d (-1, -1, -1, -1), insn_d (15, 12, 12, 1), 1 },

    /* ld r2, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 2, 12, 0, 0), 0 },

    /* addis r12, r12, 1 <optional> */
    { insn_d (-1, -1, -1, -1), insn_d (15, 12, 12, 1), 1 },

    /* mtctr r11 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 11, 9, 467), 0 },

    /* ld r11, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 0 },
      
    /* bctr */
    { -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };
#define PPC64_STANDARD_LINKAGE1_LEN \
  (sizeof (ppc64_standard_linkage1) / sizeof (ppc64_standard_linkage1[0]))

static struct insn_pattern ppc64_standard_linkage2[] =
  {
    /* addis r12, r2, <any> */
    { insn_d (-1, -1, -1, 0), insn_d (15, 12, 2, 0), 0 },

    /* std r2, 40(r1) */
    { -1, insn_ds (62, 2, 1, 40, 0), 0 },

    /* ld r11, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 0 },

    /* addi r12, r12, <any> <optional> */
    { insn_d (-1, -1, -1, 0), insn_d (14, 12, 12, 0), 1 },

    /* mtctr r11 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 11, 9, 467), 0 },

    /* ld r2, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 2, 12, 0, 0), 0 },

    /* ld r11, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 0 },
      
    /* bctr */
    { -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };
#define PPC64_STANDARD_LINKAGE2_LEN \
  (sizeof (ppc64_standard_linkage2) / sizeof (ppc64_standard_linkage2[0]))

static struct insn_pattern ppc64_standard_linkage3[] =
  {
    /* std r2, 40(r1) */
    { -1, insn_ds (62, 2, 1, 40, 0), 0 },

    /* ld r11, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 2, 0, 0), 0 },

    /* addi r2, r2, <any> <optional> */
    { insn_d (-1, -1, -1, 0), insn_d (14, 2, 2, 0), 1 },

    /* mtctr r11 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 11, 9, 467), 0 },

    /* ld r11, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 2, 0, 0), 0 },
      
    /* ld r2, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 2, 2, 0, 0), 0 },

    /* bctr */
    { -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };
#define PPC64_STANDARD_LINKAGE3_LEN \
  (sizeof (ppc64_standard_linkage3) / sizeof (ppc64_standard_linkage3[0]))


/* When the dynamic linker is doing lazy symbol resolution, the first
   call to a function in another object will go like this:

   - The user's function calls the linkage function:

     100007c4:	4b ff fc d5 	bl	10000498
     100007c8:	e8 41 00 28 	ld	r2,40(r1)

   - The linkage function loads the entry point (and other stuff) from
     the function descriptor in the PLT, and jumps to it:

     10000498:	3d 82 00 00 	addis	r12,r2,0
     1000049c:	f8 41 00 28 	std	r2,40(r1)
     100004a0:	e9 6c 80 98 	ld	r11,-32616(r12)
     100004a4:	e8 4c 80 a0 	ld	r2,-32608(r12)
     100004a8:	7d 69 03 a6 	mtctr	r11
     100004ac:	e9 6c 80 a8 	ld	r11,-32600(r12)
     100004b0:	4e 80 04 20 	bctr

   - But since this is the first time that PLT entry has been used, it
     sends control to its glink entry.  That loads the number of the
     PLT entry and jumps to the common glink0 code:

     10000c98:	38 00 00 00 	li	r0,0
     10000c9c:	4b ff ff dc 	b	10000c78

   - The common glink0 code then transfers control to the dynamic
     linker's fixup code:

     10000c78:	e8 41 00 28 	ld	r2,40(r1)
     10000c7c:	3d 82 00 00 	addis	r12,r2,0
     10000c80:	e9 6c 80 80 	ld	r11,-32640(r12)
     10000c84:	e8 4c 80 88 	ld	r2,-32632(r12)
     10000c88:	7d 69 03 a6 	mtctr	r11
     10000c8c:	e9 6c 80 90 	ld	r11,-32624(r12)
     10000c90:	4e 80 04 20 	bctr

   Eventually, this code will figure out how to skip all of this,
   including the dynamic linker.  At the moment, we just get through
   the linkage function.  */

/* If the current thread is about to execute a series of instructions
   at PC matching the ppc64_standard_linkage pattern, and INSN is the result
   from that pattern match, return the code address to which the
   standard linkage function will send them.  (This doesn't deal with
   dynamic linker lazy symbol resolution stubs.)  */
static CORE_ADDR
ppc64_standard_linkage1_target (struct frame_info *frame,
				CORE_ADDR pc, unsigned int *insn)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* The address of the function descriptor this linkage function
     references.  */
  CORE_ADDR desc
    = ((CORE_ADDR) get_frame_register_unsigned (frame,
						tdep->ppc_gp0_regnum + 2)
       + (insn_d_field (insn[0]) << 16)
       + insn_ds_field (insn[2]));

  /* The first word of the descriptor is the entry point.  Return that.  */
  return ppc64_desc_entry_point (gdbarch, desc);
}

static struct core_regset_section ppc_linux_vsx_regset_sections[] =
{
  { ".reg", 48 * 4, "general-purpose" },
  { ".reg2", 264, "floating-point" },
  { ".reg-ppc-vmx", 544, "ppc Altivec" },
  { ".reg-ppc-vsx", 256, "POWER7 VSX" },
  { NULL, 0}
};

static struct core_regset_section ppc_linux_vmx_regset_sections[] =
{
  { ".reg", 48 * 4, "general-purpose" },
  { ".reg2", 264, "floating-point" },
  { ".reg-ppc-vmx", 544, "ppc Altivec" },
  { NULL, 0}
};

static struct core_regset_section ppc_linux_fp_regset_sections[] =
{
  { ".reg", 48 * 4, "general-purpose" },
  { ".reg2", 264, "floating-point" },
  { NULL, 0}
};

static struct core_regset_section ppc64_linux_vsx_regset_sections[] =
{
  { ".reg", 48 * 8, "general-purpose" },
  { ".reg2", 264, "floating-point" },
  { ".reg-ppc-vmx", 544, "ppc Altivec" },
  { ".reg-ppc-vsx", 256, "POWER7 VSX" },
  { NULL, 0}
};

static struct core_regset_section ppc64_linux_vmx_regset_sections[] =
{
  { ".reg", 48 * 8, "general-purpose" },
  { ".reg2", 264, "floating-point" },
  { ".reg-ppc-vmx", 544, "ppc Altivec" },
  { NULL, 0}
};

static struct core_regset_section ppc64_linux_fp_regset_sections[] =
{
  { ".reg", 48 * 8, "general-purpose" },
  { ".reg2", 264, "floating-point" },
  { NULL, 0}
};

static CORE_ADDR
ppc64_standard_linkage2_target (struct frame_info *frame,
				CORE_ADDR pc, unsigned int *insn)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* The address of the function descriptor this linkage function
     references.  */
  CORE_ADDR desc
    = ((CORE_ADDR) get_frame_register_unsigned (frame,
						tdep->ppc_gp0_regnum + 2)
       + (insn_d_field (insn[0]) << 16)
       + insn_ds_field (insn[2]));

  /* The first word of the descriptor is the entry point.  Return that.  */
  return ppc64_desc_entry_point (gdbarch, desc);
}

static CORE_ADDR
ppc64_standard_linkage3_target (struct frame_info *frame,
				CORE_ADDR pc, unsigned int *insn)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* The address of the function descriptor this linkage function
     references.  */
  CORE_ADDR desc
    = ((CORE_ADDR) get_frame_register_unsigned (frame,
						tdep->ppc_gp0_regnum + 2)
       + insn_ds_field (insn[1]));

  /* The first word of the descriptor is the entry point.  Return that.  */
  return ppc64_desc_entry_point (gdbarch, desc);
}


/* Given that we've begun executing a call trampoline at PC, return
   the entry point of the function the trampoline will go to.  */
static CORE_ADDR
ppc64_skip_trampoline_code (struct frame_info *frame, CORE_ADDR pc)
{
  unsigned int ppc64_standard_linkage1_insn[PPC64_STANDARD_LINKAGE1_LEN];
  unsigned int ppc64_standard_linkage2_insn[PPC64_STANDARD_LINKAGE2_LEN];
  unsigned int ppc64_standard_linkage3_insn[PPC64_STANDARD_LINKAGE3_LEN];
  CORE_ADDR target;

  if (insns_match_pattern (pc, ppc64_standard_linkage1,
                           ppc64_standard_linkage1_insn))
    pc = ppc64_standard_linkage1_target (frame, pc,
					 ppc64_standard_linkage1_insn);
  else if (insns_match_pattern (pc, ppc64_standard_linkage2,
				ppc64_standard_linkage2_insn))
    pc = ppc64_standard_linkage2_target (frame, pc,
					 ppc64_standard_linkage2_insn);
  else if (insns_match_pattern (pc, ppc64_standard_linkage3,
				ppc64_standard_linkage3_insn))
    pc = ppc64_standard_linkage3_target (frame, pc,
					 ppc64_standard_linkage3_insn);
  else
    return 0;

  /* The PLT descriptor will either point to the already resolved target
     address, or else to a glink stub.  As the latter carry synthetic @plt
     symbols, find_solib_trampoline_target should be able to resolve them.  */
  target = find_solib_trampoline_target (frame, pc);
  return target? target : pc;
}


/* Support for convert_from_func_ptr_addr (ARCH, ADDR, TARG) on PPC64
   GNU/Linux.

   Usually a function pointer's representation is simply the address
   of the function.  On GNU/Linux on the PowerPC however, a function
   pointer may be a pointer to a function descriptor.

   For PPC64, a function descriptor is a TOC entry, in a data section,
   which contains three words: the first word is the address of the
   function, the second word is the TOC pointer (r2), and the third word
   is the static chain value.

   Throughout GDB it is currently assumed that a function pointer contains
   the address of the function, which is not easy to fix.  In addition, the
   conversion of a function address to a function pointer would
   require allocation of a TOC entry in the inferior's memory space,
   with all its drawbacks.  To be able to call C++ virtual methods in
   the inferior (which are called via function pointers),
   find_function_addr uses this function to get the function address
   from a function pointer.

   If ADDR points at what is clearly a function descriptor, transform
   it into the address of the corresponding function, if needed.  Be
   conservative, otherwise GDB will do the transformation on any
   random addresses such as occur when there is no symbol table.  */

static CORE_ADDR
ppc64_linux_convert_from_func_ptr_addr (struct gdbarch *gdbarch,
					CORE_ADDR addr,
					struct target_ops *targ)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct target_section *s = target_section_by_addr (targ, addr);

  /* Check if ADDR points to a function descriptor.  */
  if (s && strcmp (s->the_bfd_section->name, ".opd") == 0)
    {
      /* There may be relocations that need to be applied to the .opd 
	 section.  Unfortunately, this function may be called at a time
	 where these relocations have not yet been performed -- this can
	 happen for example shortly after a library has been loaded with
	 dlopen, but ld.so has not yet applied the relocations.

	 To cope with both the case where the relocation has been applied,
	 and the case where it has not yet been applied, we do *not* read
	 the (maybe) relocated value from target memory, but we instead
	 read the non-relocated value from the BFD, and apply the relocation
	 offset manually.

	 This makes the assumption that all .opd entries are always relocated
	 by the same offset the section itself was relocated.  This should
	 always be the case for GNU/Linux executables and shared libraries.
	 Note that other kind of object files (e.g. those added via
	 add-symbol-files) will currently never end up here anyway, as this
	 function accesses *target* sections only; only the main exec and
	 shared libraries are ever added to the target.  */

      gdb_byte buf[8];
      int res;

      res = bfd_get_section_contents (s->bfd, s->the_bfd_section,
				      &buf, addr - s->addr, 8);
      if (res != 0)
	return extract_unsigned_integer (buf, 8, byte_order)
		- bfd_section_vma (s->bfd, s->the_bfd_section) + s->addr;
   }

  return addr;
}

/* Wrappers to handle Linux-only registers.  */

static void
ppc_linux_supply_gregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *gregs, size_t len)
{
  const struct ppc_reg_offsets *offsets = regset->descr;

  ppc_supply_gregset (regset, regcache, regnum, gregs, len);

  if (ppc_linux_trap_reg_p (get_regcache_arch (regcache)))
    {
      /* "orig_r3" is stored 2 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_ORIG_R3_REGNUM)
	ppc_supply_reg (regcache, PPC_ORIG_R3_REGNUM, gregs,
			offsets->pc_offset + 2 * offsets->gpr_size,
			offsets->gpr_size);

      /* "trap" is stored 8 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_TRAP_REGNUM)
	ppc_supply_reg (regcache, PPC_TRAP_REGNUM, gregs,
			offsets->pc_offset + 8 * offsets->gpr_size,
			offsets->gpr_size);
    }
}

static void
ppc_linux_collect_gregset (const struct regset *regset,
			   const struct regcache *regcache,
			   int regnum, void *gregs, size_t len)
{
  const struct ppc_reg_offsets *offsets = regset->descr;

  /* Clear areas in the linux gregset not written elsewhere.  */
  if (regnum == -1)
    memset (gregs, 0, len);

  ppc_collect_gregset (regset, regcache, regnum, gregs, len);

  if (ppc_linux_trap_reg_p (get_regcache_arch (regcache)))
    {
      /* "orig_r3" is stored 2 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_ORIG_R3_REGNUM)
	ppc_collect_reg (regcache, PPC_ORIG_R3_REGNUM, gregs,
			 offsets->pc_offset + 2 * offsets->gpr_size,
			 offsets->gpr_size);

      /* "trap" is stored 8 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_TRAP_REGNUM)
	ppc_collect_reg (regcache, PPC_TRAP_REGNUM, gregs,
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
  ppc_linux_collect_gregset,
  NULL
};

static const struct regset ppc64_linux_gregset = {
  &ppc64_linux_reg_offsets,
  ppc_linux_supply_gregset,
  ppc_linux_collect_gregset,
  NULL
};

static const struct regset ppc32_linux_fpregset = {
  &ppc32_linux_reg_offsets,
  ppc_supply_fpregset,
  ppc_collect_fpregset,
  NULL
};

static const struct regset ppc32_linux_vrregset = {
  &ppc32_linux_reg_offsets,
  ppc_supply_vrregset,
  ppc_collect_vrregset,
  NULL
};

static const struct regset ppc32_linux_vsxregset = {
  &ppc32_linux_reg_offsets,
  ppc_supply_vsxregset,
  ppc_collect_vsxregset,
  NULL
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

static const struct regset *
ppc_linux_regset_from_core_section (struct gdbarch *core_arch,
				    const char *sect_name, size_t sect_size)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (core_arch);
  if (strcmp (sect_name, ".reg") == 0)
    {
      if (tdep->wordsize == 4)
	return &ppc32_linux_gregset;
      else
	return &ppc64_linux_gregset;
    }
  if (strcmp (sect_name, ".reg2") == 0)
    return &ppc32_linux_fpregset;
  if (strcmp (sect_name, ".reg-ppc-vmx") == 0)
    return &ppc32_linux_vrregset;
  if (strcmp (sect_name, ".reg-ppc-vsx") == 0)
    return &ppc32_linux_vsxregset;
  return NULL;
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


/* Address to use for displaced stepping.  When debugging a stand-alone
   SPU executable, entry_point_address () will point to an SPU local-store
   address and is thus not usable as displaced stepping location.  We use
   the auxiliary vector to determine the PowerPC-side entry point address
   instead.  */

static CORE_ADDR ppc_linux_entry_point_addr = 0;

static void
ppc_linux_inferior_created (struct target_ops *target, int from_tty)
{
  ppc_linux_entry_point_addr = 0;
}

static CORE_ADDR
ppc_linux_displaced_step_location (struct gdbarch *gdbarch)
{
  if (ppc_linux_entry_point_addr == 0)
    {
      CORE_ADDR addr;

      /* Determine entry point from target auxiliary vector.  */
      if (target_auxv_search (&current_target, AT_ENTRY, &addr) <= 0)
	error (_("Cannot find AT_ENTRY auxiliary vector entry."));

      /* Make certain that the address points at real code, and not a
	 function descriptor.  */
      addr = gdbarch_convert_from_func_ptr_addr (gdbarch, addr,
						 &current_target);

      /* Inferior calls also use the entry point as a breakpoint location.
	 We don't want displaced stepping to interfere with those
	 breakpoints, so leave space.  */
      ppc_linux_entry_point_addr = addr + 2 * PPC_INSN_SIZE;
    }

  return ppc_linux_entry_point_addr;
}


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
  return strncmp (bfd_section_name (abfd, asect), "SPU/", 4) == 0;
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
  struct minimal_symbol *sym;

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
  if (sym)
    {
      spe_context_objfile = objfile;
      spe_context_lm_addr = svr4_fetch_objfile_link_map (objfile);
      spe_context_offset = SYMBOL_VALUE_ADDRESS (sym);
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
      volatile struct gdb_exception ex;

      while (target && !target->to_get_thread_local_address)
	target = find_target_beneath (target);
      if (!target)
	return 0;

      TRY_CATCH (ex, RETURN_MASK_ERROR)
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

      if (ex.reason < 0)
	return 0;
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
  struct ppu2spu_cache *cache = *this_cache;
  return get_regcache_arch (cache->regcache);
}

static void
ppu2spu_this_id (struct frame_info *this_frame,
		 void **this_cache, struct frame_id *this_id)
{
  struct ppu2spu_cache *cache = *this_cache;
  *this_id = cache->frame_id;
}

static struct value *
ppu2spu_prev_register (struct frame_info *this_frame,
		       void **this_cache, int regnum)
{
  struct ppu2spu_cache *cache = *this_cache;
  struct gdbarch *gdbarch = get_regcache_arch (cache->regcache);
  gdb_byte *buf;

  buf = alloca (register_size (gdbarch, regnum));

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

static int
ppu2spu_unwind_register (void *src, int regnum, gdb_byte *buf)
{
  struct ppu2spu_data *data = src;
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
      info.tdep_info = (void *) &data.id;
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
  struct ppu2spu_cache *cache = this_cache;
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


static void
ppc_linux_init_abi (struct gdbarch_info info,
                    struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  struct tdesc_arch_data *tdesc_data = (void *) info.tdep_info;

  linux_init_abi (info, gdbarch);

  /* PPC GNU/Linux uses either 64-bit or 128-bit long doubles; where
     128-bit, they are IBM long double, not IEEE quad long double as
     in the System V ABI PowerPC Processor Supplement.  We can safely
     let them default to 128-bit, since the debug info will give the
     size of type actually used in each case.  */
  set_gdbarch_long_double_bit (gdbarch, 16 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_format (gdbarch, floatformats_ibm_long_double);

  /* Handle inferior calls during interrupted system calls.  */
  set_gdbarch_write_pc (gdbarch, ppc_linux_write_pc);

  /* Get the syscall number from the arch's register.  */
  set_gdbarch_get_syscall_number (gdbarch, ppc_linux_get_syscall_number);

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
      set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
      set_solib_svr4_fetch_link_map_offsets
        (gdbarch, svr4_ilp32_fetch_link_map_offsets);

      /* Setting the correct XML syscall filename.  */
      set_xml_syscall_file_name (XML_SYSCALL_FILENAME_PPC);

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

      /* Supported register sections.  */
      if (tdesc_find_feature (info.target_desc,
			      "org.gnu.gdb.power.vsx"))
	set_gdbarch_core_regset_sections (gdbarch,
					  ppc_linux_vsx_regset_sections);
      else if (tdesc_find_feature (info.target_desc,
			       "org.gnu.gdb.power.altivec"))
	set_gdbarch_core_regset_sections (gdbarch,
					  ppc_linux_vmx_regset_sections);
      else
	set_gdbarch_core_regset_sections (gdbarch,
					  ppc_linux_fp_regset_sections);
    }
  
  if (tdep->wordsize == 8)
    {
      /* Handle PPC GNU/Linux 64-bit function pointers (which are really
	 function descriptors).  */
      set_gdbarch_convert_from_func_ptr_addr
	(gdbarch, ppc64_linux_convert_from_func_ptr_addr);

      /* Shared library handling.  */
      set_gdbarch_skip_trampoline_code (gdbarch, ppc64_skip_trampoline_code);
      set_solib_svr4_fetch_link_map_offsets
        (gdbarch, svr4_lp64_fetch_link_map_offsets);

      /* Setting the correct XML syscall filename.  */
      set_xml_syscall_file_name (XML_SYSCALL_FILENAME_PPC64);

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

      /* Supported register sections.  */
      if (tdesc_find_feature (info.target_desc,
			      "org.gnu.gdb.power.vsx"))
	set_gdbarch_core_regset_sections (gdbarch,
					  ppc64_linux_vsx_regset_sections);
      else if (tdesc_find_feature (info.target_desc,
			       "org.gnu.gdb.power.altivec"))
	set_gdbarch_core_regset_sections (gdbarch,
					  ppc64_linux_vmx_regset_sections);
      else
	set_gdbarch_core_regset_sections (gdbarch,
					  ppc64_linux_fp_regset_sections);
    }
  set_gdbarch_regset_from_core_section (gdbarch,
					ppc_linux_regset_from_core_section);
  set_gdbarch_core_read_description (gdbarch, ppc_linux_core_read_description);

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

      /* The default displaced_step_at_entry_point doesn't work for
	 SPU stand-alone executables.  */
      set_gdbarch_displaced_step_location (gdbarch,
					   ppc_linux_displaced_step_location);
    }
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

  /* Attach to inferior_created observer.  */
  observer_attach_inferior_created (ppc_linux_inferior_created);

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
