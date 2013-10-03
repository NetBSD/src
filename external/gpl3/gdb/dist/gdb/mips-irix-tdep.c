/* Target-dependent code for the MIPS architecture running on IRIX,
   for GDB, the GNU Debugger.

   Copyright (C) 2002-2013 Free Software Foundation, Inc.

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
#include "osabi.h"
#include "gdb_string.h"
#include "solib.h"
#include "solib-irix.h"
#include "elf-bfd.h"
#include "mips-tdep.h"
#include "trad-frame.h"
#include "tramp-frame.h"

static void
mips_irix_elf_osabi_sniff_abi_tag_sections (bfd *abfd, asection *sect,
                                            void *obj)
{
  enum gdb_osabi *os_ident_ptr = obj;
  const char *name;
  unsigned int sectsize;

  name = bfd_get_section_name (abfd, sect);
  sectsize = bfd_section_size (abfd, sect);

  if (strncmp (name, ".MIPS.", 6) == 0 && sectsize > 0)
    {
      /* The presence of a section named with a ".MIPS." prefix is
         indicative of an IRIX binary.  */
      *os_ident_ptr = GDB_OSABI_IRIX;
    }
}

static enum gdb_osabi
mips_irix_elf_osabi_sniffer (bfd *abfd)
{
  unsigned int elfosabi;
  enum gdb_osabi osabi = GDB_OSABI_UNKNOWN;

  /* If the generic sniffer gets a hit, return and let other sniffers
     get a crack at it.  */
  bfd_map_over_sections (abfd,
			 generic_elf_osabi_sniff_abi_tag_sections,
			 &osabi);
  if (osabi != GDB_OSABI_UNKNOWN)
    return GDB_OSABI_UNKNOWN;

  elfosabi = elf_elfheader (abfd)->e_ident[EI_OSABI];

  if (elfosabi == ELFOSABI_NONE)
    {
      /* When elfosabi is ELFOSABI_NONE (0), then the ELF structures in the
	 file are conforming to the base specification for that machine 
	 (there are no OS-specific extensions).  In order to determine the 
	 real OS in use we must look for OS notes that have been added.
	 
	 For IRIX, we simply look for sections named with .MIPS. as
	 prefixes.  */
      bfd_map_over_sections (abfd,
			     mips_irix_elf_osabi_sniff_abi_tag_sections, 
			     &osabi);
    }
  return osabi;
}

/* Unwinding past the signal handler on mips-irix.

   Note: The following has only been tested with N32, but can probably
         be made to work with a small number of adjustments.

   On mips-irix, the sigcontext_t structure is stored at the base
   of the frame established by the _sigtramp function.  The definition
   of this structure can be found in <sys/signal.h> (comments have been
   C++'ified to avoid a collision with the C-style comment delimiters
   used by this comment):

      typedef struct sigcontext {
        __uint32_t      sc_regmask;     // regs to restore in sigcleanup
        __uint32_t      sc_status;      // cp0 status register
        __uint64_t      sc_pc;          // pc at time of signal
        // General purpose registers
        __uint64_t      sc_regs[32];    // processor regs 0 to 31
        // Floating point coprocessor state
        __uint64_t      sc_fpregs[32];  // fp regs 0 to 31
        __uint32_t      sc_ownedfp;     // fp has been used
        __uint32_t      sc_fpc_csr;     // fpu control and status reg
        __uint32_t      sc_fpc_eir;     // fpu exception instruction reg
                                        // implementation/revision
        __uint32_t      sc_ssflags;     // signal stack state to restore
        __uint64_t      sc_mdhi;        // Multiplier hi and low regs
        __uint64_t      sc_mdlo;
        // System coprocessor registers at time of signal
        __uint64_t      sc_cause;       // cp0 cause register
        __uint64_t      sc_badvaddr;    // cp0 bad virtual address
        __uint64_t      sc_triggersave; // state of graphics trigger (SGI)
        sigset_t        sc_sigset;      // signal mask to restore
        __uint64_t      sc_fp_rounded_result;   // for Ieee 754 support
        __uint64_t      sc_pad[31];
      } sigcontext_t;

   The following macros provide the offset of some of the fields
   used to retrieve the value of the registers before the signal
   was raised.  */

/* The size of the sigtramp frame.  The sigtramp frame base can then
   be computed by adding this size to the SP.  */
#define SIGTRAMP_FRAME_SIZE 48
/* The offset in sigcontext_t where the PC is saved.  */
#define SIGCONTEXT_PC_OFF 8
/* The offset in sigcontext_t where the GP registers are saved.  */
#define SIGCONTEXT_REGS_OFF (SIGCONTEXT_PC_OFF + 8)
/* The offset in sigcontext_t where the FP regsiters are saved.  */
#define SIGCONTEXT_FPREGS_OFF (SIGCONTEXT_REGS_OFF + 32 * 8)
/* The offset in sigcontext_t where the FP CSR register is saved.  */
#define SIGCONTEXT_FPCSR_OFF (SIGCONTEXT_FPREGS_OFF + 32 * 8 + 4)
/* The offset in sigcontext_t where the multiplier hi register is saved.  */
#define SIGCONTEXT_HI_OFF (SIGCONTEXT_FPCSR_OFF + 2 * 4)
/* The offset in sigcontext_t where the multiplier lo register is saved.  */
#define SIGCONTEXT_LO_OFF (SIGCONTEXT_HI_OFF + 4)

/* Implement the "init" routine in struct tramp_frame for the N32 ABI
   on mips-irix.  */
static void
mips_irix_n32_tramp_frame_init (const struct tramp_frame *self,
				struct frame_info *this_frame,
				struct trad_frame_cache *this_cache,
				CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  const int num_regs = gdbarch_num_regs (gdbarch);
  int sp_cooked_regno = num_regs + MIPS_SP_REGNUM;
  const CORE_ADDR sp = get_frame_register_signed (this_frame, sp_cooked_regno);
  const CORE_ADDR sigcontext_base = sp + 48;
  const struct mips_regnum *regs = mips_regnum (gdbarch);
  int ireg;

  trad_frame_set_reg_addr (this_cache, regs->pc + gdbarch_num_regs (gdbarch),
                           sigcontext_base + SIGCONTEXT_PC_OFF);

  for (ireg = 1; ireg < 32; ireg++)
    trad_frame_set_reg_addr (this_cache, ireg + MIPS_ZERO_REGNUM + num_regs,
                             sigcontext_base + SIGCONTEXT_REGS_OFF + ireg * 8);

  for (ireg = 0; ireg < 32; ireg++)
    trad_frame_set_reg_addr (this_cache, ireg + regs->fp0 + num_regs,
                             sigcontext_base + SIGCONTEXT_FPREGS_OFF
                               + ireg * 8);

  trad_frame_set_reg_addr (this_cache, regs->fp_control_status + num_regs,
                           sigcontext_base + SIGCONTEXT_FPCSR_OFF);

  trad_frame_set_reg_addr (this_cache, regs->hi + num_regs,
                           sigcontext_base + SIGCONTEXT_HI_OFF);

  trad_frame_set_reg_addr (this_cache, regs->lo + num_regs,
                           sigcontext_base + SIGCONTEXT_LO_OFF);

  trad_frame_set_id (this_cache, frame_id_build (sigcontext_base, func));
}

/* The tramp_frame structure describing sigtramp frames on mips-irix N32.

   Note that the list of instructions below is pretty much a pure dump
   of function _sigtramp on mips-irix.  A few instructions are actually
   not tested (mask set to 0), because a portion of these instructions
   contain an address which changes due to relocation.  We could use
   a smarter mask that checks the instrutction code alone, but given
   the number of instructions already being checked, this seemed
   unnecessary.  */

static const struct tramp_frame mips_irix_n32_tramp_frame =
{
  SIGTRAMP_FRAME,
  4,
  {
   { 0x3c0c8000, -1 }, /*    lui     t0,0x8000 */
   { 0x27bdffd0, -1 }, /*    addiu   sp,sp,-48 */
   { 0x008c6024, -1 }, /*    and     t0,a0,t0 */
   { 0xffa40018, -1 }, /*    sd      a0,24(sp) */
   { 0x00000000,  0 }, /*    beqz    t0,0xfaefcb8 <_sigtramp+40> */
   { 0xffa60028, -1 }, /*    sd      a2,40(sp) */
   { 0x01806027, -1 }, /*    nor     t0,t0,zero */
   { 0xffa00020, -1 }, /*    sd      zero,32(sp) */
   { 0x00000000,  0 }, /*    b       0xfaefcbc <_sigtramp+44> */
   { 0x008c2024, -1 }, /*    and     a0,a0,t0 */
   { 0xffa60020, -1 }, /*    sd      a2,32(sp) */
   { 0x03e0c025, -1 }, /*    move    t8,ra */
   { 0x00000000,  0 }, /*    bal     0xfaefcc8 <_sigtramp+56> */
   { 0x00000000, -1 }, /*    nop */
   { 0x3c0c0007, -1 }, /*    lui     t0,0x7 */
   { 0x00e0c825, -1 }, /*    move    t9,a3 */
   { 0x658c80fc, -1 }, /*    daddiu  t0,t0,-32516 */
   { 0x019f602d, -1 }, /*    daddu   t0,t0,ra */
   { 0x0300f825, -1 }, /*    move    ra,t8 */
   { 0x8d8c9880, -1 }, /*    lw      t0,-26496(t0) */
   { 0x8d8c0000, -1 }, /*    lw      t0,0(t0) */
   { 0x8d8d0000, -1 }, /*    lw      t1,0(t0) */
   { 0xffac0008, -1 }, /*    sd      t0,8(sp) */
   { 0x0320f809, -1 }, /*    jalr    t9 */
   { 0xffad0010, -1 }, /*    sd      t1,16(sp) */
   { 0xdfad0010, -1 }, /*    ld      t1,16(sp) */
   { 0xdfac0008, -1 }, /*    ld      t0,8(sp) */
   { 0xad8d0000, -1 }, /*    sw      t1,0(t0) */
   { 0xdfa40020, -1 }, /*    ld      a0,32(sp) */
   { 0xdfa50028, -1 }, /*    ld      a1,40(sp) */
   { 0xdfa60018, -1 }, /*    ld      a2,24(sp) */
   { 0x24020440, -1 }, /*    li      v0,1088 */
   { 0x0000000c, -1 }, /*    syscall */
   { TRAMP_SENTINEL_INSN, -1 }
  },
  mips_irix_n32_tramp_frame_init
};

/* Implement the "init" routine in struct tramp_frame for the stack-based
   trampolines used on mips-irix.  */

static void
mips_irix_n32_stack_tramp_frame_init (const struct tramp_frame *self,
				      struct frame_info *this_frame,
				      struct trad_frame_cache *this_cache,
				      CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  const int num_regs = gdbarch_num_regs (gdbarch);
  int sp_cooked_regno = num_regs + MIPS_SP_REGNUM;
  const CORE_ADDR sp = get_frame_register_signed (this_frame, sp_cooked_regno);

  /* The previous frame's PC is stored in RA.  */
  trad_frame_set_reg_realreg (this_cache, gdbarch_pc_regnum (gdbarch),
                              num_regs + MIPS_RA_REGNUM);

  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

/* A tramp_frame structure describing the stack-based trampoline
   used on mips-irix.  These trampolines are created on the stack
   before being called.  */

static const struct tramp_frame mips_irix_n32_stack_tramp_frame =
{
  SIGTRAMP_FRAME,
  4,
  {
   { 0x8f210000, 0xffff0000 },  /* lw     at, N(t9) */
   { 0x8f2f0000, 0xffff0000 },  /* lw     t3, M(t9) */
   { 0x00200008, 0xffffffff },  /* jr     at        */
   { 0x0020c82d, 0xffffffff },  /* move   t9, at    */
   { TRAMP_SENTINEL_INSN, -1 }
  },
  mips_irix_n32_stack_tramp_frame_init
};

static void
mips_irix_init_abi (struct gdbarch_info info,
                    struct gdbarch *gdbarch)
{
  set_solib_ops (gdbarch, &irix_so_ops);
  tramp_frame_prepend_unwinder (gdbarch, &mips_irix_n32_stack_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch, &mips_irix_n32_tramp_frame);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_mips_irix_tdep;

void
_initialize_mips_irix_tdep (void)
{
  /* Register an ELF OS ABI sniffer for IRIX binaries.  */
  gdbarch_register_osabi_sniffer (bfd_arch_mips,
				  bfd_target_elf_flavour,
				  mips_irix_elf_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_mips, 0, GDB_OSABI_IRIX,
			  mips_irix_init_abi);
}
