/* Common target-dependent code for ppc64 GDB, the GNU debugger.

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
#include "frame.h"
#include "gdbcore.h"
#include "ppc-tdep.h"
#include "ppc64-tdep.h"
#include "elf-bfd.h"

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

/* If DESC is the address of a 64-bit PowerPC FreeBSD function
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

static struct ppc_insn_pattern ppc64_standard_linkage1[] =
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

    /* ld r11, <any>(r12) <optional> */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 1 },

    /* bctr */
    { -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

#define PPC64_STANDARD_LINKAGE1_LEN ARRAY_SIZE (ppc64_standard_linkage1)

static struct ppc_insn_pattern ppc64_standard_linkage2[] =
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

    /* ld r11, <any>(r12) <optional> */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 1 },

    /* bctr */
    { -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

#define PPC64_STANDARD_LINKAGE2_LEN ARRAY_SIZE (ppc64_standard_linkage2)

static struct ppc_insn_pattern ppc64_standard_linkage3[] =
  {
    /* std r2, 40(r1) */
    { -1, insn_ds (62, 2, 1, 40, 0), 0 },

    /* ld r11, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 2, 0, 0), 0 },

    /* addi r2, r2, <any> <optional> */
    { insn_d (-1, -1, -1, 0), insn_d (14, 2, 2, 0), 1 },

    /* mtctr r11 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 11, 9, 467), 0 },

    /* ld r11, <any>(r2) <optional> */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 2, 0, 0), 1 },

    /* ld r2, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 2, 2, 0, 0), 0 },

    /* bctr */
    { -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

#define PPC64_STANDARD_LINKAGE3_LEN ARRAY_SIZE (ppc64_standard_linkage3)

/* When the dynamic linker is doing lazy symbol resolution, the first
   call to a function in another object will go like this:

   - The user's function calls the linkage function:

	 100007c4:	4b ff fc d5		bl	10000498
	 100007c8:	e8 41 00 28		ld	r2,40(r1)

   - The linkage function loads the entry point (and other stuff) from
	 the function descriptor in the PLT, and jumps to it:

	 10000498:	3d 82 00 00		addis	r12,r2,0
	 1000049c:	f8 41 00 28		std	r2,40(r1)
	 100004a0:	e9 6c 80 98		ld	r11,-32616(r12)
	 100004a4:	e8 4c 80 a0		ld	r2,-32608(r12)
	 100004a8:	7d 69 03 a6		mtctr	r11
	 100004ac:	e9 6c 80 a8		ld	r11,-32600(r12)
	 100004b0:	4e 80 04 20		bctr

   - But since this is the first time that PLT entry has been used, it
	 sends control to its glink entry.  That loads the number of the
	 PLT entry and jumps to the common glink0 code:

	 10000c98:	38 00 00 00		li	r0,0
	 10000c9c:	4b ff ff dc		b	10000c78

   - The common glink0 code then transfers control to the dynamic
	 linker's fixup code:

	 10000c78:	e8 41 00 28		ld	r2,40(r1)
	 10000c7c:	3d 82 00 00		addis	r12,r2,0
	 10000c80:	e9 6c 80 80		ld	r11,-32640(r12)
	 10000c84:	e8 4c 80 88		ld	r2,-32632(r12)
	 10000c88:	7d 69 03 a6		mtctr	r11
	 10000c8c:	e9 6c 80 90		ld	r11,-32624(r12)
	 10000c90:	4e 80 04 20		bctr

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
       + (ppc_insn_d_field (insn[0]) << 16)
       + ppc_insn_ds_field (insn[2]));

  /* The first word of the descriptor is the entry point.  Return that.  */
  return ppc64_desc_entry_point (gdbarch, desc);
}

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
       + (ppc_insn_d_field (insn[0]) << 16)
       + ppc_insn_ds_field (insn[2]));

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
       + ppc_insn_ds_field (insn[1]));

  /* The first word of the descriptor is the entry point.  Return that.  */
  return ppc64_desc_entry_point (gdbarch, desc);
}


/* Given that we've begun executing a call trampoline at PC, return
   the entry point of the function the trampoline will go to.  */

CORE_ADDR
ppc64_skip_trampoline_code (struct frame_info *frame, CORE_ADDR pc)
{
  unsigned int ppc64_standard_linkage1_insn[PPC64_STANDARD_LINKAGE1_LEN];
  unsigned int ppc64_standard_linkage2_insn[PPC64_STANDARD_LINKAGE2_LEN];
  unsigned int ppc64_standard_linkage3_insn[PPC64_STANDARD_LINKAGE3_LEN];
  CORE_ADDR target;

  if (ppc_insns_match_pattern (pc, ppc64_standard_linkage1,
			       ppc64_standard_linkage1_insn))
    pc = ppc64_standard_linkage1_target (frame, pc,
					 ppc64_standard_linkage1_insn);
  else if (ppc_insns_match_pattern (pc, ppc64_standard_linkage2,
				    ppc64_standard_linkage2_insn))
    pc = ppc64_standard_linkage2_target (frame, pc,
					 ppc64_standard_linkage2_insn);
  else if (ppc_insns_match_pattern (pc, ppc64_standard_linkage3,
				    ppc64_standard_linkage3_insn))
    pc = ppc64_standard_linkage3_target (frame, pc,
					 ppc64_standard_linkage3_insn);
  else
    return 0;

  /* The PLT descriptor will either point to the already resolved target
     address, or else to a glink stub.  As the latter carry synthetic @plt
     symbols, find_solib_trampoline_target should be able to resolve them.  */
  target = find_solib_trampoline_target (frame, pc);
  return target ? target : pc;
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

CORE_ADDR
ppc64_convert_from_func_ptr_addr (struct gdbarch *gdbarch,
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

/* A synthetic 'dot' symbols on ppc64 has the udata.p entry pointing
   back to the original ELF symbol it was derived from.  Get the size
   from that symbol.  */

void
ppc64_elf_make_msymbol_special (asymbol *sym, struct minimal_symbol *msym)
{
  if ((sym->flags & BSF_SYNTHETIC) != 0 && sym->udata.p != NULL)
    {
      elf_symbol_type *elf_sym = (elf_symbol_type *) sym->udata.p;
      SET_MSYMBOL_SIZE (msym, elf_sym->internal_elf_sym.st_size);
    }
}
