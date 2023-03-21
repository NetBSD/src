/* Target-dependent code for the RISC-V architecture, for GDB.

   Copyright (C) 2018-2020 Free Software Foundation, Inc.

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
#include "value.h"
#include "gdbcmd.h"
#include "language.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "target.h"
#include "arch-utils.h"
#include "regcache.h"
#include "osabi.h"
#include "riscv-tdep.h"
#include "block.h"
#include "reggroups.h"
#include "opcode/riscv.h"
#include "elf/riscv.h"
#include "elf-bfd.h"
#include "symcat.h"
#include "dis-asm.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "infcall.h"
#include "floatformat.h"
#include "remote.h"
#include "target-descriptions.h"
#include "dwarf2/frame.h"
#include "user-regs.h"
#include "valprint.h"
#include "gdbsupport/common-defs.h"
#include "opcode/riscv-opc.h"
#include "cli/cli-decode.h"
#include "observable.h"
#include "prologue-value.h"
#include "arch/riscv.h"
#include "riscv-ravenscar-thread.h"

/* The stack must be 16-byte aligned.  */
#define SP_ALIGNMENT 16

/* The biggest alignment that the target supports.  */
#define BIGGEST_ALIGNMENT 16

/* Define a series of is_XXX_insn functions to check if the value INSN
   is an instance of instruction XXX.  */
#define DECLARE_INSN(INSN_NAME, INSN_MATCH, INSN_MASK) \
static inline bool is_ ## INSN_NAME ## _insn (long insn) \
{ \
  return (insn & INSN_MASK) == INSN_MATCH; \
}
#include "opcode/riscv-opc.h"
#undef DECLARE_INSN

/* Cached information about a frame.  */

struct riscv_unwind_cache
{
  /* The register from which we can calculate the frame base.  This is
     usually $sp or $fp.  */
  int frame_base_reg;

  /* The offset from the current value in register FRAME_BASE_REG to the
     actual frame base address.  */
  int frame_base_offset;

  /* Information about previous register values.  */
  struct trad_frame_saved_reg *regs;

  /* The id for this frame.  */
  struct frame_id this_id;

  /* The base (stack) address for this frame.  This is the stack pointer
     value on entry to this frame before any adjustments are made.  */
  CORE_ADDR frame_base;
};

/* RISC-V specific register group for CSRs.  */

static reggroup *csr_reggroup = NULL;

/* Callback function for user_reg_add.  */

static struct value *
value_of_riscv_user_reg (struct frame_info *frame, const void *baton)
{
  const int *reg_p = (const int *) baton;
  return value_of_register (*reg_p, frame);
}

/* Information about a register alias that needs to be set up for this
   target.  These are collected when the target's XML description is
   analysed, and then processed later, once the gdbarch has been created.  */

class riscv_pending_register_alias
{
public:
  /* Constructor.  */

  riscv_pending_register_alias (const char *name, const void *baton)
    : m_name (name),
      m_baton (baton)
  { /* Nothing.  */ }

  /* Convert this into a user register for GDBARCH.  */

  void create (struct gdbarch *gdbarch) const
  {
    user_reg_add (gdbarch, m_name, value_of_riscv_user_reg, m_baton);
  }

private:
  /* The name for this alias.  */
  const char *m_name;

  /* The baton value for passing to user_reg_add.  This must point to some
     data that will live for at least as long as the gdbarch object to
     which the user register is attached.  */
  const void *m_baton;
};

/* Registers in the RISCV_REGISTER_FEATURE lists below are either optional,
   or required.  For example the $pc register is always going to be a
   required register, you can't do much debugging without that.  In
   contrast, most of the CSRs are optional, GDB doesn't require them in
   order to have a useful debug session.  This enum models the difference
   between these register types.  */

enum riscv_register_required_status
{
  /* This register is optional within this feature.  */
  RISCV_REG_OPTIONAL,

  /* This register is required within this feature.  */
  RISCV_REG_REQUIRED,

  /* This register is required, the register must either be in this
     feature, or it could appear within the CSR feature.  */
  RISCV_REG_REQUIRED_MAYBE_CSR
};

/* A set of registers that we expect to find in a tdesc_feature.  These
   are use in RISCV_GDBARCH_INIT when processing the target description.  */

struct riscv_register_feature
{
  /* Information for a single register.  */
  struct register_info
  {
    /* The GDB register number for this register.  */
    int regnum;

    /* List of names for this register.  The first name in this list is the
       preferred name, the name GDB should use when describing this
       register.  */
    std::vector<const char *> names;

    /* Is this register required within this feature?  In some cases the
       register could be required, but might also be in the CSR feature.  */
    riscv_register_required_status required;

    /* Look in FEATURE for a register with a name from this classes names
       list.  If the register is found then register its number with
       TDESC_DATA and add all its aliases to the ALIASES list.  REG_SET is
       used to help create the aliases.  */
    bool check (struct tdesc_arch_data *tdesc_data,
		const struct tdesc_feature *feature,
		const struct riscv_register_feature *reg_set,
		std::vector<riscv_pending_register_alias> *aliases) const;
  };

  /* The name for this feature.  This is the name used to find this feature
     within the target description.  */
  const char *name;

  /* For x-regs and f-regs we always force GDB to use the first name from
     the REGISTERS.NAMES vector, it is therefore important that we create
     user-register aliases for all of the remaining names at indexes 1+ in
     the names vector.

     For CSRs we take a different approach, we prefer whatever name the
     target description uses, in this case we want to create user-register
     aliases for any other names that aren't the target description
     provided name.

     When this flag is true we are dealing with the first case, and when
     this is false we are dealing with the latter.  */
  bool prefer_first_name;

  /* List of all the registers that we expect that we might find in this
     register set.  */
  std::vector<struct register_info> registers;
};

/* See description in the class declaration above.  */

bool
riscv_register_feature::register_info::check
	(struct tdesc_arch_data *tdesc_data,
	 const struct tdesc_feature *feature,
	 const struct riscv_register_feature *reg_set,
	 std::vector<riscv_pending_register_alias> *aliases) const
{
  for (const char *name : this->names)
    {
      bool found = tdesc_numbered_register (feature, tdesc_data,
					    this->regnum, name);
      if (found)
	{
	  /* We know that the target description mentions this
	     register.  In RISCV_REGISTER_NAME we ensure that GDB
	     always uses the first name for each register, so here we
	     add aliases for all of the remaining names.  */
	  bool prefer_first_name = reg_set->prefer_first_name;
	  int start_index = prefer_first_name ? 1 : 0;
	  for (int i = start_index; i < this->names.size (); ++i)
	    {
	      const char *alias = this->names[i];
	      if (alias == name && !prefer_first_name)
		continue;
	      aliases->emplace_back (alias, (void *) &this->regnum);
	    }
	  return true;
	}
    }
  return false;
}

/* The general x-registers feature set.  */

static const struct riscv_register_feature riscv_xreg_feature =
{
 "org.gnu.gdb.riscv.cpu", true,
 {
   { RISCV_ZERO_REGNUM + 0, { "zero", "x0" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 1, { "ra", "x1" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 2, { "sp", "x2" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 3, { "gp", "x3" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 4, { "tp", "x4" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 5, { "t0", "x5" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 6, { "t1", "x6" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 7, { "t2", "x7" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 8, { "fp", "x8", "s0" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 9, { "s1", "x9" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 10, { "a0", "x10" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 11, { "a1", "x11" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 12, { "a2", "x12" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 13, { "a3", "x13" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 14, { "a4", "x14" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 15, { "a5", "x15" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 16, { "a6", "x16" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 17, { "a7", "x17" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 18, { "s2", "x18" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 19, { "s3", "x19" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 20, { "s4", "x20" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 21, { "s5", "x21" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 22, { "s6", "x22" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 23, { "s7", "x23" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 24, { "s8", "x24" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 25, { "s9", "x25" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 26, { "s10", "x26" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 27, { "s11", "x27" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 28, { "t3", "x28" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 29, { "t4", "x29" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 30, { "t5", "x30" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 31, { "t6", "x31" }, RISCV_REG_REQUIRED },
   { RISCV_ZERO_REGNUM + 32, { "pc" }, RISCV_REG_REQUIRED }
 }
};

/* The f-registers feature set.  */

static const struct riscv_register_feature riscv_freg_feature =
{
 "org.gnu.gdb.riscv.fpu", true,
 {
   { RISCV_FIRST_FP_REGNUM + 0, { "ft0", "f0" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 1, { "ft1", "f1" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 2, { "ft2", "f2" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 3, { "ft3", "f3" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 4, { "ft4", "f4" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 5, { "ft5", "f5" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 6, { "ft6", "f6" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 7, { "ft7", "f7" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 8, { "fs0", "f8" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 9, { "fs1", "f9" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 10, { "fa0", "f10" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 11, { "fa1", "f11" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 12, { "fa2", "f12" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 13, { "fa3", "f13" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 14, { "fa4", "f14" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 15, { "fa5", "f15" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 16, { "fa6", "f16" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 17, { "fa7", "f17" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 18, { "fs2", "f18" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 19, { "fs3", "f19" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 20, { "fs4", "f20" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 21, { "fs5", "f21" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 22, { "fs6", "f22" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 23, { "fs7", "f23" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 24, { "fs8", "f24" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 25, { "fs9", "f25" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 26, { "fs10", "f26" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 27, { "fs11", "f27" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 28, { "ft8", "f28" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 29, { "ft9", "f29" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 30, { "ft10", "f30" }, RISCV_REG_REQUIRED },
   { RISCV_FIRST_FP_REGNUM + 31, { "ft11", "f31" }, RISCV_REG_REQUIRED },

   { RISCV_CSR_FFLAGS_REGNUM, { "fflags", "csr1" }, RISCV_REG_REQUIRED_MAYBE_CSR },
   { RISCV_CSR_FRM_REGNUM, { "frm", "csr2" }, RISCV_REG_REQUIRED_MAYBE_CSR },
   { RISCV_CSR_FCSR_REGNUM, { "fcsr", "csr3" }, RISCV_REG_REQUIRED_MAYBE_CSR },

 }
};

/* Set of virtual registers.  These are not physical registers on the
   hardware, but might be available from the target.  These are not pseudo
   registers, reading these really does result in a register read from the
   target, it is just that there might not be a physical register backing
   the result.  */

static const struct riscv_register_feature riscv_virtual_feature =
{
 "org.gnu.gdb.riscv.virtual", false,
 {
   { RISCV_PRIV_REGNUM, { "priv" }, RISCV_REG_OPTIONAL }
 }
};

/* Feature set for CSRs.  This set is NOT constant as the register names
   list for each register is not complete.  The aliases are computed
   during RISCV_CREATE_CSR_ALIASES.  */

static struct riscv_register_feature riscv_csr_feature =
{
 "org.gnu.gdb.riscv.csr", false,
 {
#define DECLARE_CSR(NAME,VALUE,CLASS,DEFINE_VER,ABORT_VER) \
  { RISCV_ ## VALUE ## _REGNUM, { # NAME }, RISCV_REG_OPTIONAL },
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR
 }
};

/* Complete RISCV_CSR_FEATURE, building the CSR alias names and adding them
   to the name list for each register.  */

static void
riscv_create_csr_aliases ()
{
  for (auto &reg : riscv_csr_feature.registers)
    {
      int csr_num = reg.regnum - RISCV_FIRST_CSR_REGNUM;
      const char *alias = xstrprintf ("csr%d", csr_num);
      reg.names.push_back (alias);

      /* Setup the other csr aliases.  We don't use a switch table here in
         case there are multiple aliases with the same value.  Also filter
         based on ABRT_VER in order to avoid a very old alias for misa that
         duplicates the name "misa" but at a different CSR address.  */
#define DECLARE_CSR_ALIAS(NAME,VALUE,CLASS,DEF_VER,ABRT_VER)	 \
      if (csr_num == VALUE && ABRT_VER >= PRIV_SPEC_CLASS_1P11)  \
        reg.names.push_back ( # NAME );
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR_ALIAS
    }
}

/* Controls whether we place compressed breakpoints or not.  When in auto
   mode GDB tries to determine if the target supports compressed
   breakpoints, and uses them if it does.  */

static enum auto_boolean use_compressed_breakpoints;

/* The show callback for 'show riscv use-compressed-breakpoints'.  */

static void
show_use_compressed_breakpoints (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c,
				 const char *value)
{
  fprintf_filtered (file,
		    _("Debugger's use of compressed breakpoints is set "
		      "to %s.\n"), value);
}

/* The set and show lists for 'set riscv' and 'show riscv' prefixes.  */

static struct cmd_list_element *setriscvcmdlist = NULL;
static struct cmd_list_element *showriscvcmdlist = NULL;

/* The set and show lists for 'set riscv' and 'show riscv' prefixes.  */

static struct cmd_list_element *setdebugriscvcmdlist = NULL;
static struct cmd_list_element *showdebugriscvcmdlist = NULL;

/* The show callback for all 'show debug riscv VARNAME' variables.  */

static void
show_riscv_debug_variable (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c,
			   const char *value)
{
  fprintf_filtered (file,
		    _("RiscV debug variable `%s' is set to: %s\n"),
		    c->name, value);
}

/* When this is set to non-zero debugging information about breakpoint
   kinds will be printed.  */

static unsigned int riscv_debug_breakpoints = 0;

/* When this is set to non-zero debugging information about inferior calls
   will be printed.  */

static unsigned int riscv_debug_infcall = 0;

/* When this is set to non-zero debugging information about stack unwinding
   will be printed.  */

static unsigned int riscv_debug_unwinder = 0;

/* When this is set to non-zero debugging information about gdbarch
   initialisation will be printed.  */

static unsigned int riscv_debug_gdbarch = 0;

/* See riscv-tdep.h.  */

int
riscv_isa_xlen (struct gdbarch *gdbarch)
{
  return gdbarch_tdep (gdbarch)->isa_features.xlen;
}

/* See riscv-tdep.h.  */

int
riscv_abi_xlen (struct gdbarch *gdbarch)
{
  return gdbarch_tdep (gdbarch)->abi_features.xlen;
}

/* See riscv-tdep.h.  */

int
riscv_isa_flen (struct gdbarch *gdbarch)
{
  return gdbarch_tdep (gdbarch)->isa_features.flen;
}

/* See riscv-tdep.h.  */

int
riscv_abi_flen (struct gdbarch *gdbarch)
{
  return gdbarch_tdep (gdbarch)->abi_features.flen;
}

/* Return true if the target for GDBARCH has floating point hardware.  */

static bool
riscv_has_fp_regs (struct gdbarch *gdbarch)
{
  return (riscv_isa_flen (gdbarch) > 0);
}

/* Return true if GDBARCH is using any of the floating point hardware ABIs.  */

static bool
riscv_has_fp_abi (struct gdbarch *gdbarch)
{
  return gdbarch_tdep (gdbarch)->abi_features.flen > 0;
}

/* Return true if REGNO is a floating pointer register.  */

static bool
riscv_is_fp_regno_p (int regno)
{
  return (regno >= RISCV_FIRST_FP_REGNUM
	  && regno <= RISCV_LAST_FP_REGNUM);
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
riscv_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  if (use_compressed_breakpoints == AUTO_BOOLEAN_AUTO)
    {
      bool unaligned_p = false;
      gdb_byte buf[1];

      /* Some targets don't support unaligned reads.  The address can only
	 be unaligned if the C extension is supported.  So it is safe to
	 use a compressed breakpoint in this case.  */
      if (*pcptr & 0x2)
	unaligned_p = true;
      else
	{
	  /* Read the opcode byte to determine the instruction length.  If
	     the read fails this may be because we tried to set the
	     breakpoint at an invalid address, in this case we provide a
	     fake result which will give a breakpoint length of 4.
	     Hopefully when we try to actually insert the breakpoint we
	     will see a failure then too which will be reported to the
	     user.  */
	  if (target_read_code (*pcptr, buf, 1) == -1)
	    buf[0] = 0;
	  read_code (*pcptr, buf, 1);
	}

      if (riscv_debug_breakpoints)
	{
	  const char *bp = (unaligned_p || riscv_insn_length (buf[0]) == 2
			    ? "C.EBREAK" : "EBREAK");

	  fprintf_unfiltered (gdb_stdlog, "Using %s for breakpoint at %s ",
			      bp, paddress (gdbarch, *pcptr));
	  if (unaligned_p)
	    fprintf_unfiltered (gdb_stdlog, "(unaligned address)\n");
	  else
	    fprintf_unfiltered (gdb_stdlog, "(instruction length %d)\n",
				riscv_insn_length (buf[0]));
	}
      if (unaligned_p || riscv_insn_length (buf[0]) == 2)
	return 2;
      else
	return 4;
    }
  else if (use_compressed_breakpoints == AUTO_BOOLEAN_TRUE)
    return 2;
  else
    return 4;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
riscv_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  static const gdb_byte ebreak[] = { 0x73, 0x00, 0x10, 0x00, };
  static const gdb_byte c_ebreak[] = { 0x02, 0x90 };

  *size = kind;
  switch (kind)
    {
    case 2:
      return c_ebreak;
    case 4:
      return ebreak;
    default:
      gdb_assert_not_reached (_("unhandled breakpoint kind"));
    }
}

/* Implement the register_name gdbarch method.  This is used instead of
   the function supplied by calling TDESC_USE_REGISTERS so that we can
   ensure the preferred names are offered for x-regs and f-regs.  */

static const char *
riscv_register_name (struct gdbarch *gdbarch, int regnum)
{
  /* Lookup the name through the target description.  If we get back NULL
     then this is an unknown register.  If we do get a name back then we
     look up the registers preferred name below.  */
  const char *name = tdesc_register_name (gdbarch, regnum);
  if (name == NULL || name[0] == '\0')
    return NULL;

  /* We want GDB to use the ABI names for registers even if the target
     gives us a target description with the architectural name.  For
     example we want to see 'ra' instead of 'x1' whatever the target
     description called it.  */
  if (regnum >= RISCV_ZERO_REGNUM && regnum < RISCV_FIRST_FP_REGNUM)
    {
      gdb_assert (regnum < riscv_xreg_feature.registers.size ());
      return riscv_xreg_feature.registers[regnum].names[0];
    }

  /* Like with the x-regs we prefer the abi names for the floating point
     registers.  */
  if (regnum >= RISCV_FIRST_FP_REGNUM && regnum <= RISCV_LAST_FP_REGNUM)
    {
      if (riscv_has_fp_regs (gdbarch))
        {
          regnum -= RISCV_FIRST_FP_REGNUM;
          gdb_assert (regnum < riscv_freg_feature.registers.size ());
          return riscv_freg_feature.registers[regnum].names[0];
        }
      else
        return NULL;
    }

  /* Some targets (QEMU) are reporting these three registers twice, once
     in the FPU feature, and once in the CSR feature.  Both of these read
     the same underlying state inside the target, but naming the register
     twice in the target description results in GDB having two registers
     with the same name, only one of which can ever be accessed, but both
     will show up in 'info register all'.  Unless, we identify the
     duplicate copies of these registers (in riscv_tdesc_unknown_reg) and
     then hide the registers here by giving them no name.  */
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  if (tdep->duplicate_fflags_regnum == regnum)
    return NULL;
  if (tdep->duplicate_frm_regnum == regnum)
    return NULL;
  if (tdep->duplicate_fcsr_regnum == regnum)
    return NULL;

  /* The remaining registers are different.  For all other registers on the
     machine we prefer to see the names that the target description
     provides.  This is particularly important for CSRs which might be
     renamed over time.  If GDB keeps track of the "latest" name, but a
     particular target provides an older name then we don't want to force
     users to see the newer name in register output.

     The other case that reaches here are any registers that the target
     provided that GDB is completely unaware of.  For these we have no
     choice but to accept the target description name.

     Just accept whatever name TDESC_REGISTER_NAME returned.  */
  return name;
}

/* Construct a type for 64-bit FP registers.  */

static struct type *
riscv_fpreg_d_type (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (tdep->riscv_fpreg_d_type == nullptr)
    {
      const struct builtin_type *bt = builtin_type (gdbarch);

      /* The type we're building is this: */
#if 0
      union __gdb_builtin_type_fpreg_d
      {
	float f;
	double d;
      };
#endif

      struct type *t;

      t = arch_composite_type (gdbarch,
			       "__gdb_builtin_type_fpreg_d", TYPE_CODE_UNION);
      append_composite_type_field (t, "float", bt->builtin_float);
      append_composite_type_field (t, "double", bt->builtin_double);
      TYPE_VECTOR (t) = 1;
      t->set_name ("builtin_type_fpreg_d");
      tdep->riscv_fpreg_d_type = t;
    }

  return tdep->riscv_fpreg_d_type;
}

/* Implement the register_type gdbarch method.  This is installed as an
   for the override setup by TDESC_USE_REGISTERS, for most registers we
   delegate the type choice to the target description, but for a few
   registers we try to improve the types if the target description has
   taken a simplistic approach.  */

static struct type *
riscv_register_type (struct gdbarch *gdbarch, int regnum)
{
  struct type *type = tdesc_register_type (gdbarch, regnum);
  int xlen = riscv_isa_xlen (gdbarch);

  /* We want to perform some specific type "fixes" in cases where we feel
     that we really can do better than the target description.  For all
     other cases we just return what the target description says.  */
  if (riscv_is_fp_regno_p (regnum))
    {
      /* This spots the case for RV64 where the double is defined as
         either 'ieee_double' or 'float' (which is the generic name that
         converts to 'double' on 64-bit).  In these cases its better to
         present the registers using a union type.  */
      int flen = riscv_isa_flen (gdbarch);
      if (flen == 8
          && type->code () == TYPE_CODE_FLT
          && TYPE_LENGTH (type) == flen
          && (strcmp (type->name (), "builtin_type_ieee_double") == 0
              || strcmp (type->name (), "double") == 0))
        type = riscv_fpreg_d_type (gdbarch);
    }

  if ((regnum == gdbarch_pc_regnum (gdbarch)
       || regnum == RISCV_RA_REGNUM
       || regnum == RISCV_FP_REGNUM
       || regnum == RISCV_SP_REGNUM
       || regnum == RISCV_GP_REGNUM
       || regnum == RISCV_TP_REGNUM)
      && type->code () == TYPE_CODE_INT
      && TYPE_LENGTH (type) == xlen)
    {
      /* This spots the case where some interesting registers are defined
         as simple integers of the expected size, we force these registers
         to be pointers as we believe that is more useful.  */
      if (regnum == gdbarch_pc_regnum (gdbarch)
          || regnum == RISCV_RA_REGNUM)
        type = builtin_type (gdbarch)->builtin_func_ptr;
      else if (regnum == RISCV_FP_REGNUM
               || regnum == RISCV_SP_REGNUM
               || regnum == RISCV_GP_REGNUM
               || regnum == RISCV_TP_REGNUM)
	type = builtin_type (gdbarch)->builtin_data_ptr;
    }

  return type;
}

/* Helper for riscv_print_registers_info, prints info for a single register
   REGNUM.  */

static void
riscv_print_one_register_info (struct gdbarch *gdbarch,
			       struct ui_file *file,
			       struct frame_info *frame,
			       int regnum)
{
  const char *name = gdbarch_register_name (gdbarch, regnum);
  struct value *val;
  struct type *regtype;
  int print_raw_format;
  enum tab_stops { value_column_1 = 15 };

  fputs_filtered (name, file);
  print_spaces_filtered (value_column_1 - strlen (name), file);

  try
    {
      val = value_of_register (regnum, frame);
      regtype = value_type (val);
    }
  catch (const gdb_exception_error &ex)
    {
      /* Handle failure to read a register without interrupting the entire
         'info registers' flow.  */
      fprintf_filtered (file, "%s\n", ex.what ());
      return;
    }

  print_raw_format = (value_entirely_available (val)
		      && !value_optimized_out (val));

  if (regtype->code () == TYPE_CODE_FLT
      || (regtype->code () == TYPE_CODE_UNION
	  && regtype->num_fields () == 2
	  && regtype->field (0).type ()->code () == TYPE_CODE_FLT
	  && regtype->field (1).type ()->code () == TYPE_CODE_FLT)
      || (regtype->code () == TYPE_CODE_UNION
	  && regtype->num_fields () == 3
	  && regtype->field (0).type ()->code () == TYPE_CODE_FLT
	  && regtype->field (1).type ()->code () == TYPE_CODE_FLT
	  && regtype->field (2).type ()->code () == TYPE_CODE_FLT))
    {
      struct value_print_options opts;
      const gdb_byte *valaddr = value_contents_for_printing (val);
      enum bfd_endian byte_order = type_byte_order (regtype);

      get_user_print_options (&opts);
      opts.deref_ref = 1;

      common_val_print (val, file, 0, &opts, current_language);

      if (print_raw_format)
	{
	  fprintf_filtered (file, "\t(raw ");
	  print_hex_chars (file, valaddr, TYPE_LENGTH (regtype), byte_order,
			   true);
	  fprintf_filtered (file, ")");
	}
    }
  else
    {
      struct value_print_options opts;

      /* Print the register in hex.  */
      get_formatted_print_options (&opts, 'x');
      opts.deref_ref = 1;
      common_val_print (val, file, 0, &opts, current_language);

      if (print_raw_format)
	{
	  if (regnum == RISCV_CSR_MSTATUS_REGNUM)
	    {
	      LONGEST d;
	      int size = register_size (gdbarch, regnum);
	      unsigned xlen;

	      /* The SD field is always in the upper bit of MSTATUS, regardless
		 of the number of bits in MSTATUS.  */
	      d = value_as_long (val);
	      xlen = size * 8;
	      fprintf_filtered (file,
				"\tSD:%X VM:%02X MXR:%X PUM:%X MPRV:%X XS:%X "
				"FS:%X MPP:%x HPP:%X SPP:%X MPIE:%X HPIE:%X "
				"SPIE:%X UPIE:%X MIE:%X HIE:%X SIE:%X UIE:%X",
				(int) ((d >> (xlen - 1)) & 0x1),
				(int) ((d >> 24) & 0x1f),
				(int) ((d >> 19) & 0x1),
				(int) ((d >> 18) & 0x1),
				(int) ((d >> 17) & 0x1),
				(int) ((d >> 15) & 0x3),
				(int) ((d >> 13) & 0x3),
				(int) ((d >> 11) & 0x3),
				(int) ((d >> 9) & 0x3),
				(int) ((d >> 8) & 0x1),
				(int) ((d >> 7) & 0x1),
				(int) ((d >> 6) & 0x1),
				(int) ((d >> 5) & 0x1),
				(int) ((d >> 4) & 0x1),
				(int) ((d >> 3) & 0x1),
				(int) ((d >> 2) & 0x1),
				(int) ((d >> 1) & 0x1),
				(int) ((d >> 0) & 0x1));
	    }
	  else if (regnum == RISCV_CSR_MISA_REGNUM)
	    {
	      int base;
	      unsigned xlen, i;
	      LONGEST d;
	      int size = register_size (gdbarch, regnum);

	      /* The MXL field is always in the upper two bits of MISA,
		 regardless of the number of bits in MISA.  Mask out other
		 bits to ensure we have a positive value.  */
	      d = value_as_long (val);
	      base = (d >> ((size * 8) - 2)) & 0x3;
	      xlen = 16;

	      for (; base > 0; base--)
		xlen *= 2;
	      fprintf_filtered (file, "\tRV%d", xlen);

	      for (i = 0; i < 26; i++)
		{
		  if (d & (1 << i))
		    fprintf_filtered (file, "%c", 'A' + i);
		}
	    }
	  else if (regnum == RISCV_CSR_FCSR_REGNUM
		   || regnum == RISCV_CSR_FFLAGS_REGNUM
		   || regnum == RISCV_CSR_FRM_REGNUM)
	    {
	      LONGEST d;

	      d = value_as_long (val);

	      fprintf_filtered (file, "\t");
	      if (regnum != RISCV_CSR_FRM_REGNUM)
		fprintf_filtered (file,
				  "RD:%01X NV:%d DZ:%d OF:%d UF:%d NX:%d",
				  (int) ((d >> 5) & 0x7),
				  (int) ((d >> 4) & 0x1),
				  (int) ((d >> 3) & 0x1),
				  (int) ((d >> 2) & 0x1),
				  (int) ((d >> 1) & 0x1),
				  (int) ((d >> 0) & 0x1));

	      if (regnum != RISCV_CSR_FFLAGS_REGNUM)
		{
		  static const char * const sfrm[] =
		    {
		      "RNE (round to nearest; ties to even)",
		      "RTZ (Round towards zero)",
		      "RDN (Round down towards -INF)",
		      "RUP (Round up towards +INF)",
		      "RMM (Round to nearest; ties to max magnitude)",
		      "INVALID[5]",
		      "INVALID[6]",
		      "dynamic rounding mode",
		    };
		  int frm = ((regnum == RISCV_CSR_FCSR_REGNUM)
			     ? (d >> 5) : d) & 0x3;

		  fprintf_filtered (file, "%sFRM:%i [%s]",
				    (regnum == RISCV_CSR_FCSR_REGNUM
				     ? " " : ""),
				    frm, sfrm[frm]);
		}
	    }
	  else if (regnum == RISCV_PRIV_REGNUM)
	    {
	      LONGEST d;
	      uint8_t priv;

	      d = value_as_long (val);
	      priv = d & 0xff;

	      if (priv < 4)
		{
		  static const char * const sprv[] =
		    {
		      "User/Application",
		      "Supervisor",
		      "Hypervisor",
		      "Machine"
		    };
		  fprintf_filtered (file, "\tprv:%d [%s]",
				    priv, sprv[priv]);
		}
	      else
		fprintf_filtered (file, "\tprv:%d [INVALID]", priv);
	    }
	  else
	    {
	      /* If not a vector register, print it also according to its
		 natural format.  */
	      if (TYPE_VECTOR (regtype) == 0)
		{
		  get_user_print_options (&opts);
		  opts.deref_ref = 1;
		  fprintf_filtered (file, "\t");
		  common_val_print (val, file, 0, &opts, current_language);
		}
	    }
	}
    }
  fprintf_filtered (file, "\n");
}

/* Return true if REGNUM is a valid CSR register.  The CSR register space
   is sparsely populated, so not every number is a named CSR.  */

static bool
riscv_is_regnum_a_named_csr (int regnum)
{
  gdb_assert (regnum >= RISCV_FIRST_CSR_REGNUM
	      && regnum <= RISCV_LAST_CSR_REGNUM);

  switch (regnum)
    {
#define DECLARE_CSR(name, num, class, define_ver, abort_ver) case RISCV_ ## num ## _REGNUM:
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR
      return true;

    default:
      return false;
    }
}

/* Implement the register_reggroup_p gdbarch method.  Is REGNUM a member
   of REGGROUP?  */

static int
riscv_register_reggroup_p (struct gdbarch  *gdbarch, int regnum,
			   struct reggroup *reggroup)
{
  /* Used by 'info registers' and 'info registers <groupname>'.  */

  if (gdbarch_register_name (gdbarch, regnum) == NULL
      || gdbarch_register_name (gdbarch, regnum)[0] == '\0')
    return 0;

  if (regnum > RISCV_LAST_REGNUM)
    {
      /* Any extra registers from the CSR tdesc_feature (identified in
	 riscv_tdesc_unknown_reg) are removed from the save/restore groups
	 as some targets (QEMU) report CSRs which then can't be read.  */
      struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
      if ((reggroup == restore_reggroup || reggroup == save_reggroup)
	  && regnum >= tdep->unknown_csrs_first_regnum
	  && regnum < (tdep->unknown_csrs_first_regnum
		       + tdep->unknown_csrs_count))
	return 0;

      /* This is some other unknown register from the target description.
	 In this case we trust whatever the target description says about
	 which groups this register should be in.  */
      int ret = tdesc_register_in_reggroup_p (gdbarch, regnum, reggroup);
      if (ret != -1)
        return ret;

      return default_register_reggroup_p (gdbarch, regnum, reggroup);
    }

  if (reggroup == all_reggroup)
    {
      if (regnum < RISCV_FIRST_CSR_REGNUM || regnum == RISCV_PRIV_REGNUM)
	return 1;
      if (riscv_is_regnum_a_named_csr (regnum))
        return 1;
      return 0;
    }
  else if (reggroup == float_reggroup)
    return (riscv_is_fp_regno_p (regnum)
	    || regnum == RISCV_CSR_FCSR_REGNUM
	    || regnum == RISCV_CSR_FFLAGS_REGNUM
	    || regnum == RISCV_CSR_FRM_REGNUM);
  else if (reggroup == general_reggroup)
    return regnum < RISCV_FIRST_FP_REGNUM;
  else if (reggroup == restore_reggroup || reggroup == save_reggroup)
    {
      if (riscv_has_fp_regs (gdbarch))
	return (regnum <= RISCV_LAST_FP_REGNUM
		|| regnum == RISCV_CSR_FCSR_REGNUM
		|| regnum == RISCV_CSR_FFLAGS_REGNUM
		|| regnum == RISCV_CSR_FRM_REGNUM);
      else
	return regnum < RISCV_FIRST_FP_REGNUM;
    }
  else if (reggroup == system_reggroup || reggroup == csr_reggroup)
    {
      if (regnum == RISCV_PRIV_REGNUM)
	return 1;
      if (regnum < RISCV_FIRST_CSR_REGNUM || regnum > RISCV_LAST_CSR_REGNUM)
	return 0;
      if (riscv_is_regnum_a_named_csr (regnum))
        return 1;
      return 0;
    }
  else if (reggroup == vector_reggroup)
    return 0;
  else
    return 0;
}

/* Implement the print_registers_info gdbarch method.  This is used by
   'info registers' and 'info all-registers'.  */

static void
riscv_print_registers_info (struct gdbarch *gdbarch,
			    struct ui_file *file,
			    struct frame_info *frame,
			    int regnum, int print_all)
{
  if (regnum != -1)
    {
      /* Print one specified register.  */
      if (gdbarch_register_name (gdbarch, regnum) == NULL
	  || *(gdbarch_register_name (gdbarch, regnum)) == '\0')
        error (_("Not a valid register for the current processor type"));
      riscv_print_one_register_info (gdbarch, file, frame, regnum);
    }
  else
    {
      struct reggroup *reggroup;

      if (print_all)
	reggroup = all_reggroup;
      else
	reggroup = general_reggroup;

      for (regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); ++regnum)
	{
	  /* Zero never changes, so might as well hide by default.  */
	  if (regnum == RISCV_ZERO_REGNUM && !print_all)
	    continue;

	  /* Registers with no name are not valid on this ISA.  */
	  if (gdbarch_register_name (gdbarch, regnum) == NULL
	      || *(gdbarch_register_name (gdbarch, regnum)) == '\0')
	    continue;

	  /* Is the register in the group we're interested in?  */
	  if (!gdbarch_register_reggroup_p (gdbarch, regnum, reggroup))
	    continue;

	  riscv_print_one_register_info (gdbarch, file, frame, regnum);
	}
    }
}

/* Class that handles one decoded RiscV instruction.  */

class riscv_insn
{
public:

  /* Enum of all the opcodes that GDB cares about during the prologue scan.  */
  enum opcode
    {
      /* Unknown value is used at initialisation time.  */
      UNKNOWN = 0,

      /* These instructions are all the ones we are interested in during the
	 prologue scan.  */
      ADD,
      ADDI,
      ADDIW,
      ADDW,
      AUIPC,
      LUI,
      SD,
      SW,
      /* These are needed for software breakpoint support.  */
      JAL,
      JALR,
      BEQ,
      BNE,
      BLT,
      BGE,
      BLTU,
      BGEU,
      /* These are needed for stepping over atomic sequences.  */
      LR,
      SC,

      /* Other instructions are not interesting during the prologue scan, and
	 are ignored.  */
      OTHER
    };

  riscv_insn ()
    : m_length (0),
      m_opcode (OTHER),
      m_rd (0),
      m_rs1 (0),
      m_rs2 (0)
  {
    /* Nothing.  */
  }

  void decode (struct gdbarch *gdbarch, CORE_ADDR pc);

  /* Get the length of the instruction in bytes.  */
  int length () const
  { return m_length; }

  /* Get the opcode for this instruction.  */
  enum opcode opcode () const
  { return m_opcode; }

  /* Get destination register field for this instruction.  This is only
     valid if the OPCODE implies there is such a field for this
     instruction.  */
  int rd () const
  { return m_rd; }

  /* Get the RS1 register field for this instruction.  This is only valid
     if the OPCODE implies there is such a field for this instruction.  */
  int rs1 () const
  { return m_rs1; }

  /* Get the RS2 register field for this instruction.  This is only valid
     if the OPCODE implies there is such a field for this instruction.  */
  int rs2 () const
  { return m_rs2; }

  /* Get the immediate for this instruction in signed form.  This is only
     valid if the OPCODE implies there is such a field for this
     instruction.  */
  int imm_signed () const
  { return m_imm.s; }

private:

  /* Extract 5 bit register field at OFFSET from instruction OPCODE.  */
  int decode_register_index (unsigned long opcode, int offset)
  {
    return (opcode >> offset) & 0x1F;
  }

  /* Extract 5 bit register field at OFFSET from instruction OPCODE.  */
  int decode_register_index_short (unsigned long opcode, int offset)
  {
    return ((opcode >> offset) & 0x7) + 8;
  }

  /* Helper for DECODE, decode 32-bit R-type instruction.  */
  void decode_r_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = decode_register_index (ival, OP_SH_RD);
    m_rs1 = decode_register_index (ival, OP_SH_RS1);
    m_rs2 = decode_register_index (ival, OP_SH_RS2);
  }

  /* Helper for DECODE, decode 16-bit compressed R-type instruction.  */
  void decode_cr_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = m_rs1 = decode_register_index (ival, OP_SH_CRS1S);
    m_rs2 = decode_register_index (ival, OP_SH_CRS2);
  }

  /* Helper for DECODE, decode 32-bit I-type instruction.  */
  void decode_i_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = decode_register_index (ival, OP_SH_RD);
    m_rs1 = decode_register_index (ival, OP_SH_RS1);
    m_imm.s = EXTRACT_ITYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 16-bit compressed I-type instruction.  */
  void decode_ci_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = m_rs1 = decode_register_index (ival, OP_SH_CRS1S);
    m_imm.s = EXTRACT_RVC_IMM (ival);
  }

  /* Helper for DECODE, decode 32-bit S-type instruction.  */
  void decode_s_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rs1 = decode_register_index (ival, OP_SH_RS1);
    m_rs2 = decode_register_index (ival, OP_SH_RS2);
    m_imm.s = EXTRACT_STYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 16-bit CS-type instruction.  The immediate
     encoding is different for each CS format instruction, so extracting
     the immediate is left up to the caller, who should pass the extracted
     immediate value through in IMM.  */
  void decode_cs_type_insn (enum opcode opcode, ULONGEST ival, int imm)
  {
    m_opcode = opcode;
    m_imm.s = imm;
    m_rs1 = decode_register_index_short (ival, OP_SH_CRS1S);
    m_rs2 = decode_register_index_short (ival, OP_SH_CRS2S);
  }

  /* Helper for DECODE, decode 16-bit CSS-type instruction.  The immediate
     encoding is different for each CSS format instruction, so extracting
     the immediate is left up to the caller, who should pass the extracted
     immediate value through in IMM.  */
  void decode_css_type_insn (enum opcode opcode, ULONGEST ival, int imm)
  {
    m_opcode = opcode;
    m_imm.s = imm;
    m_rs1 = RISCV_SP_REGNUM;
    /* Not a compressed register number in this case.  */
    m_rs2 = decode_register_index (ival, OP_SH_CRS2);
  }

  /* Helper for DECODE, decode 32-bit U-type instruction.  */
  void decode_u_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = decode_register_index (ival, OP_SH_RD);
    m_imm.s = EXTRACT_UTYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 32-bit J-type instruction.  */
  void decode_j_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = decode_register_index (ival, OP_SH_RD);
    m_imm.s = EXTRACT_UJTYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 32-bit J-type instruction.  */
  void decode_cj_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_imm.s = EXTRACT_RVC_J_IMM (ival);
  }

  void decode_b_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rs1 = decode_register_index (ival, OP_SH_RS1);
    m_rs2 = decode_register_index (ival, OP_SH_RS2);
    m_imm.s = EXTRACT_SBTYPE_IMM (ival);
  }

  void decode_cb_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rs1 = decode_register_index_short (ival, OP_SH_CRS1S);
    m_imm.s = EXTRACT_RVC_B_IMM (ival);
  }

  /* Fetch instruction from target memory at ADDR, return the content of
     the instruction, and update LEN with the instruction length.  */
  static ULONGEST fetch_instruction (struct gdbarch *gdbarch,
				     CORE_ADDR addr, int *len);

  /* The length of the instruction in bytes.  Should be 2 or 4.  */
  int m_length;

  /* The instruction opcode.  */
  enum opcode m_opcode;

  /* The three possible registers an instruction might reference.  Not
     every instruction fills in all of these registers.  Which fields are
     valid depends on the opcode.  The naming of these fields matches the
     naming in the riscv isa manual.  */
  int m_rd;
  int m_rs1;
  int m_rs2;

  /* Possible instruction immediate.  This is only valid if the instruction
     format contains an immediate, not all instruction, whether this is
     valid depends on the opcode.  Despite only having one format for now
     the immediate is packed into a union, later instructions might require
     an unsigned formatted immediate, having the union in place now will
     reduce the need for code churn later.  */
  union riscv_insn_immediate
  {
    riscv_insn_immediate ()
      : s (0)
    {
      /* Nothing.  */
    }

    int s;
  } m_imm;
};

/* Fetch instruction from target memory at ADDR, return the content of the
   instruction, and update LEN with the instruction length.  */

ULONGEST
riscv_insn::fetch_instruction (struct gdbarch *gdbarch,
			       CORE_ADDR addr, int *len)
{
  enum bfd_endian byte_order = gdbarch_byte_order_for_code (gdbarch);
  gdb_byte buf[8];
  int instlen, status;

  /* All insns are at least 16 bits.  */
  status = target_read_memory (addr, buf, 2);
  if (status)
    memory_error (TARGET_XFER_E_IO, addr);

  /* If we need more, grab it now.  */
  instlen = riscv_insn_length (buf[0]);
  gdb_assert (instlen <= sizeof (buf));
  *len = instlen;

  if (instlen > 2)
    {
      status = target_read_memory (addr + 2, buf + 2, instlen - 2);
      if (status)
	memory_error (TARGET_XFER_E_IO, addr + 2);
    }

  return extract_unsigned_integer (buf, instlen, byte_order);
}

/* Fetch from target memory an instruction at PC and decode it.  This can
   throw an error if the memory access fails, callers are responsible for
   handling this error if that is appropriate.  */

void
riscv_insn::decode (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  ULONGEST ival;

  /* Fetch the instruction, and the instructions length.  */
  ival = fetch_instruction (gdbarch, pc, &m_length);

  if (m_length == 4)
    {
      if (is_add_insn (ival))
	decode_r_type_insn (ADD, ival);
      else if (is_addw_insn (ival))
	decode_r_type_insn (ADDW, ival);
      else if (is_addi_insn (ival))
	decode_i_type_insn (ADDI, ival);
      else if (is_addiw_insn (ival))
	decode_i_type_insn (ADDIW, ival);
      else if (is_auipc_insn (ival))
	decode_u_type_insn (AUIPC, ival);
      else if (is_lui_insn (ival))
	decode_u_type_insn (LUI, ival);
      else if (is_sd_insn (ival))
	decode_s_type_insn (SD, ival);
      else if (is_sw_insn (ival))
	decode_s_type_insn (SW, ival);
      else if (is_jal_insn (ival))
	decode_j_type_insn (JAL, ival);
      else if (is_jalr_insn (ival))
	decode_i_type_insn (JALR, ival);
      else if (is_beq_insn (ival))
	decode_b_type_insn (BEQ, ival);
      else if (is_bne_insn (ival))
	decode_b_type_insn (BNE, ival);
      else if (is_blt_insn (ival))
	decode_b_type_insn (BLT, ival);
      else if (is_bge_insn (ival))
	decode_b_type_insn (BGE, ival);
      else if (is_bltu_insn (ival))
	decode_b_type_insn (BLTU, ival);
      else if (is_bgeu_insn (ival))
	decode_b_type_insn (BGEU, ival);
      else if (is_lr_w_insn (ival))
	decode_r_type_insn (LR, ival);
      else if (is_lr_d_insn (ival))
	decode_r_type_insn (LR, ival);
      else if (is_sc_w_insn (ival))
	decode_r_type_insn (SC, ival);
      else if (is_sc_d_insn (ival))
	decode_r_type_insn (SC, ival);
      else
	/* None of the other fields are valid in this case.  */
	m_opcode = OTHER;
    }
  else if (m_length == 2)
    {
      int xlen = riscv_isa_xlen (gdbarch);

      /* C_ADD and C_JALR have the same opcode.  If RS2 is 0, then this is a
	 C_JALR.  So must try to match C_JALR first as it has more bits in
	 mask.  */
      if (is_c_jalr_insn (ival))
	decode_cr_type_insn (JALR, ival);
      else if (is_c_add_insn (ival))
	decode_cr_type_insn (ADD, ival);
      /* C_ADDW is RV64 and RV128 only.  */
      else if (xlen != 4 && is_c_addw_insn (ival))
	decode_cr_type_insn (ADDW, ival);
      else if (is_c_addi_insn (ival))
	decode_ci_type_insn (ADDI, ival);
      /* C_ADDIW and C_JAL have the same opcode.  C_ADDIW is RV64 and RV128
	 only and C_JAL is RV32 only.  */
      else if (xlen != 4 && is_c_addiw_insn (ival))
	decode_ci_type_insn (ADDIW, ival);
      else if (xlen == 4 && is_c_jal_insn (ival))
	decode_cj_type_insn (JAL, ival);
      /* C_ADDI16SP and C_LUI have the same opcode.  If RD is 2, then this is a
	 C_ADDI16SP.  So must try to match C_ADDI16SP first as it has more bits
	 in mask.  */
      else if (is_c_addi16sp_insn (ival))
	{
	  m_opcode = ADDI;
	  m_rd = m_rs1 = decode_register_index (ival, OP_SH_RD);
	  m_imm.s = EXTRACT_RVC_ADDI16SP_IMM (ival);
	}
      else if (is_c_addi4spn_insn (ival))
	{
	  m_opcode = ADDI;
	  m_rd = decode_register_index_short (ival, OP_SH_CRS2S);
	  m_rs1 = RISCV_SP_REGNUM;
	  m_imm.s = EXTRACT_RVC_ADDI4SPN_IMM (ival);
	}
      else if (is_c_lui_insn (ival))
        {
          m_opcode = LUI;
          m_rd = decode_register_index (ival, OP_SH_CRS1S);
          m_imm.s = EXTRACT_RVC_LUI_IMM (ival);
        }
      /* C_SD and C_FSW have the same opcode.  C_SD is RV64 and RV128 only,
	 and C_FSW is RV32 only.  */
      else if (xlen != 4 && is_c_sd_insn (ival))
	decode_cs_type_insn (SD, ival, EXTRACT_RVC_LD_IMM (ival));
      else if (is_c_sw_insn (ival))
	decode_cs_type_insn (SW, ival, EXTRACT_RVC_LW_IMM (ival));
      else if (is_c_swsp_insn (ival))
	decode_css_type_insn (SW, ival, EXTRACT_RVC_SWSP_IMM (ival));
      else if (xlen != 4 && is_c_sdsp_insn (ival))
	decode_css_type_insn (SW, ival, EXTRACT_RVC_SDSP_IMM (ival));
      /* C_JR and C_MV have the same opcode.  If RS2 is 0, then this is a C_JR.
	 So must try to match C_JR first as it ahs more bits in mask.  */
      else if (is_c_jr_insn (ival))
	decode_cr_type_insn (JALR, ival);
      else if (is_c_j_insn (ival))
	decode_cj_type_insn (JAL, ival);
      else if (is_c_beqz_insn (ival))
	decode_cb_type_insn (BEQ, ival);
      else if (is_c_bnez_insn (ival))
	decode_cb_type_insn (BNE, ival);
      else
	/* None of the other fields of INSN are valid in this case.  */
	m_opcode = OTHER;
    }
  else
    {
      /* This must be a 6 or 8 byte instruction, we don't currently decode
	 any of these, so just ignore it.  */
      gdb_assert (m_length == 6 || m_length == 8);
      m_opcode = OTHER;
    }
}

/* The prologue scanner.  This is currently only used for skipping the
   prologue of a function when the DWARF information is not sufficient.
   However, it is written with filling of the frame cache in mind, which
   is why different groups of stack setup instructions are split apart
   during the core of the inner loop.  In the future, the intention is to
   extend this function to fully support building up a frame cache that
   can unwind register values when there is no DWARF information.  */

static CORE_ADDR
riscv_scan_prologue (struct gdbarch *gdbarch,
		     CORE_ADDR start_pc, CORE_ADDR end_pc,
		     struct riscv_unwind_cache *cache)
{
  CORE_ADDR cur_pc, next_pc, after_prologue_pc;
  CORE_ADDR end_prologue_addr = 0;

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  after_prologue_pc = skip_prologue_using_sal (gdbarch, start_pc);
  if (after_prologue_pc == 0)
    after_prologue_pc = start_pc + 100;   /* Arbitrary large number.  */
  if (after_prologue_pc < end_pc)
    end_pc = after_prologue_pc;

  pv_t regs[RISCV_NUM_INTEGER_REGS]; /* Number of GPR.  */
  for (int regno = 0; regno < RISCV_NUM_INTEGER_REGS; regno++)
    regs[regno] = pv_register (regno, 0);
  pv_area stack (RISCV_SP_REGNUM, gdbarch_addr_bit (gdbarch));

  if (riscv_debug_unwinder)
    fprintf_unfiltered
      (gdb_stdlog,
       "Prologue scan for function starting at %s (limit %s)\n",
       core_addr_to_string (start_pc),
       core_addr_to_string (end_pc));

  for (next_pc = cur_pc = start_pc; cur_pc < end_pc; cur_pc = next_pc)
    {
      struct riscv_insn insn;

      /* Decode the current instruction, and decide where the next
	 instruction lives based on the size of this instruction.  */
      insn.decode (gdbarch, cur_pc);
      gdb_assert (insn.length () > 0);
      next_pc = cur_pc + insn.length ();

      /* Look for common stack adjustment insns.  */
      if ((insn.opcode () == riscv_insn::ADDI
	   || insn.opcode () == riscv_insn::ADDIW)
	  && insn.rd () == RISCV_SP_REGNUM
	  && insn.rs1 () == RISCV_SP_REGNUM)
	{
	  /* Handle: addi sp, sp, -i
	     or:     addiw sp, sp, -i  */
          gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
          gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
          regs[insn.rd ()]
            = pv_add_constant (regs[insn.rs1 ()], insn.imm_signed ());
	}
      else if ((insn.opcode () == riscv_insn::SW
		|| insn.opcode () == riscv_insn::SD)
	       && (insn.rs1 () == RISCV_SP_REGNUM
		   || insn.rs1 () == RISCV_FP_REGNUM))
	{
	  /* Handle: sw reg, offset(sp)
	     or:     sd reg, offset(sp)
	     or:     sw reg, offset(s0)
	     or:     sd reg, offset(s0)  */
	  /* Instruction storing a register onto the stack.  */
          gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
          gdb_assert (insn.rs2 () < RISCV_NUM_INTEGER_REGS);
          stack.store (pv_add_constant (regs[insn.rs1 ()], insn.imm_signed ()),
                        (insn.opcode () == riscv_insn::SW ? 4 : 8),
                        regs[insn.rs2 ()]);
	}
      else if (insn.opcode () == riscv_insn::ADDI
	       && insn.rd () == RISCV_FP_REGNUM
	       && insn.rs1 () == RISCV_SP_REGNUM)
	{
	  /* Handle: addi s0, sp, size  */
	  /* Instructions setting up the frame pointer.  */
          gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
          gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
          regs[insn.rd ()]
            = pv_add_constant (regs[insn.rs1 ()], insn.imm_signed ());
	}
      else if ((insn.opcode () == riscv_insn::ADD
		|| insn.opcode () == riscv_insn::ADDW)
	       && insn.rd () == RISCV_FP_REGNUM
	       && insn.rs1 () == RISCV_SP_REGNUM
	       && insn.rs2 () == RISCV_ZERO_REGNUM)
	{
	  /* Handle: add s0, sp, 0
	     or:     addw s0, sp, 0  */
	  /* Instructions setting up the frame pointer.  */
          gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
          gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
          regs[insn.rd ()] = pv_add_constant (regs[insn.rs1 ()], 0);
	}
      else if ((insn.opcode () == riscv_insn::ADDI
                && insn.rd () == RISCV_ZERO_REGNUM
                && insn.rs1 () == RISCV_ZERO_REGNUM
                && insn.imm_signed () == 0))
	{
	  /* Handle: add x0, x0, 0   (NOP)  */
	}
      else if (insn.opcode () == riscv_insn::AUIPC)
        {
          gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
          regs[insn.rd ()] = pv_constant (cur_pc + insn.imm_signed ());
        }
      else if (insn.opcode () == riscv_insn::LUI)
        {
	  /* Handle: lui REG, n
             Where REG is not gp register.  */
          gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
          regs[insn.rd ()] = pv_constant (insn.imm_signed ());
        }
      else if (insn.opcode () == riscv_insn::ADDI)
        {
          /* Handle: addi REG1, REG2, IMM  */
          gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
          gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
          regs[insn.rd ()]
            = pv_add_constant (regs[insn.rs1 ()], insn.imm_signed ());
        }
      else if (insn.opcode () == riscv_insn::ADD)
        {
          /* Handle: addi REG1, REG2, IMM  */
          gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
          gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
          gdb_assert (insn.rs2 () < RISCV_NUM_INTEGER_REGS);
          regs[insn.rd ()] = pv_add (regs[insn.rs1 ()], regs[insn.rs2 ()]);
        }
      else
	{
	  end_prologue_addr = cur_pc;
	  break;
	}
    }

  if (end_prologue_addr == 0)
    end_prologue_addr = cur_pc;

  if (riscv_debug_unwinder)
    fprintf_unfiltered (gdb_stdlog, "End of prologue at %s\n",
			core_addr_to_string (end_prologue_addr));

  if (cache != NULL)
    {
      /* Figure out if it is a frame pointer or just a stack pointer.  Also
         the offset held in the pv_t is from the original register value to
         the current value, which for a grows down stack means a negative
         value.  The FRAME_BASE_OFFSET is the negation of this, how to get
         from the current value to the original value.  */
      if (pv_is_register (regs[RISCV_FP_REGNUM], RISCV_SP_REGNUM))
	{
          cache->frame_base_reg = RISCV_FP_REGNUM;
          cache->frame_base_offset = -regs[RISCV_FP_REGNUM].k;
	}
      else
	{
          cache->frame_base_reg = RISCV_SP_REGNUM;
          cache->frame_base_offset = -regs[RISCV_SP_REGNUM].k;
	}

      /* Assign offset from old SP to all saved registers.  As we don't
         have the previous value for the frame base register at this
         point, we store the offset as the address in the trad_frame, and
         then convert this to an actual address later.  */
      for (int i = 0; i <= RISCV_NUM_INTEGER_REGS; i++)
	{
	  CORE_ADDR offset;
	  if (stack.find_reg (gdbarch, i, &offset))
            {
              if (riscv_debug_unwinder)
		{
		  /* Display OFFSET as a signed value, the offsets are from
		     the frame base address to the registers location on
		     the stack, with a descending stack this means the
		     offsets are always negative.  */
		  fprintf_unfiltered (gdb_stdlog,
				      "Register $%s at stack offset %s\n",
				      gdbarch_register_name (gdbarch, i),
				      plongest ((LONGEST) offset));
		}
              trad_frame_set_addr (cache->regs, i, offset);
            }
	}
    }

  return end_prologue_addr;
}

/* Implement the riscv_skip_prologue gdbarch method.  */

static CORE_ADDR
riscv_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr;

  /* See if we can determine the end of the prologue via the symbol
     table.  If so, then return either PC, or the PC after the
     prologue, whichever is greater.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);

      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  Pass -1 for the end address to indicate the prologue
     scanner can scan as far as it needs to find the end of the prologue.  */
  return riscv_scan_prologue (gdbarch, pc, ((CORE_ADDR) -1), NULL);
}

/* Implement the gdbarch push dummy code callback.  */

static CORE_ADDR
riscv_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp,
		       CORE_ADDR funaddr, struct value **args, int nargs,
		       struct type *value_type, CORE_ADDR *real_pc,
		       CORE_ADDR *bp_addr, struct regcache *regcache)
{
  /* A nop instruction is 'add x0, x0, 0'.  */
  static const gdb_byte nop_insn[] = { 0x13, 0x00, 0x00, 0x00 };

  /* Allocate space for a breakpoint, and keep the stack correctly
     aligned.  The space allocated here must be at least big enough to
     accommodate the NOP_INSN defined above.  */
  sp -= 16;
  *bp_addr = sp;
  *real_pc = funaddr;

  /* When we insert a breakpoint we select whether to use a compressed
     breakpoint or not based on the existing contents of the memory.

     If the breakpoint is being placed onto the stack as part of setting up
     for an inferior call from GDB, then the existing stack contents may
     randomly appear to be a compressed instruction, causing GDB to insert
     a compressed breakpoint.  If this happens on a target that does not
     support compressed instructions then this could cause problems.

     To prevent this issue we write an uncompressed nop onto the stack at
     the location where the breakpoint will be inserted.  In this way we
     ensure that we always use an uncompressed breakpoint, which should
     work on all targets.

     We call TARGET_WRITE_MEMORY here so that if the write fails we don't
     throw an exception.  Instead we ignore the error and move on.  The
     assumption is that either GDB will error later when actually trying to
     insert a software breakpoint, or GDB will use hardware breakpoints and
     there will be no need to write to memory later.  */
  int status = target_write_memory (*bp_addr, nop_insn, sizeof (nop_insn));

  if (riscv_debug_breakpoints || riscv_debug_infcall)
    fprintf_unfiltered (gdb_stdlog,
			"Writing %s-byte nop instruction to %s: %s\n",
			plongest (sizeof (nop_insn)),
			paddress (gdbarch, *bp_addr),
			(status == 0 ? "success" : "failed"));

  return sp;
}

/* Implement the gdbarch type alignment method, overrides the generic
   alignment algorithm for anything that is RISC-V specific.  */

static ULONGEST
riscv_type_align (gdbarch *gdbarch, type *type)
{
  type = check_typedef (type);
  if (type->code () == TYPE_CODE_ARRAY && TYPE_VECTOR (type))
    return std::min (TYPE_LENGTH (type), (ULONGEST) BIGGEST_ALIGNMENT);

  /* Anything else will be aligned by the generic code.  */
  return 0;
}

/* Holds information about a single argument either being passed to an
   inferior function, or returned from an inferior function.  This includes
   information about the size, type, etc of the argument, and also
   information about how the argument will be passed (or returned).  */

struct riscv_arg_info
{
  /* Contents of the argument.  */
  const gdb_byte *contents;

  /* Length of argument.  */
  int length;

  /* Alignment required for an argument of this type.  */
  int align;

  /* The type for this argument.  */
  struct type *type;

  /* Each argument can have either 1 or 2 locations assigned to it.  Each
     location describes where part of the argument will be placed.  The
     second location is valid based on the LOC_TYPE and C_LENGTH fields
     of the first location (which is always valid).  */
  struct location
  {
    /* What type of location this is.  */
    enum location_type
      {
       /* Argument passed in a register.  */
       in_reg,

       /* Argument passed as an on stack argument.  */
       on_stack,

       /* Argument passed by reference.  The second location is always
	  valid for a BY_REF argument, and describes where the address
	  of the BY_REF argument should be placed.  */
       by_ref
      } loc_type;

    /* Information that depends on the location type.  */
    union
    {
      /* Which register number to use.  */
      int regno;

      /* The offset into the stack region.  */
      int offset;
    } loc_data;

    /* The length of contents covered by this location.  If this is less
       than the total length of the argument, then the second location
       will be valid, and will describe where the rest of the argument
       will go.  */
    int c_length;

    /* The offset within CONTENTS for this part of the argument.  This can
       be non-zero even for the first part (the first field of a struct can
       have a non-zero offset due to padding).  For the second part of the
       argument, this might be the C_LENGTH value of the first part,
       however, if we are passing a structure in two registers, and there's
       is padding between the first and second field, then this offset
       might be greater than the length of the first argument part.  When
       the second argument location is not holding part of the argument
       value, but is instead holding the address of a reference argument,
       then this offset will be set to 0.  */
    int c_offset;
  } argloc[2];

  /* TRUE if this is an unnamed argument.  */
  bool is_unnamed;
};

/* Information about a set of registers being used for passing arguments as
   part of a function call.  The register set must be numerically
   sequential from NEXT_REGNUM to LAST_REGNUM.  The register set can be
   disabled from use by setting NEXT_REGNUM greater than LAST_REGNUM.  */

struct riscv_arg_reg
{
  riscv_arg_reg (int first, int last)
    : next_regnum (first),
      last_regnum (last)
  {
    /* Nothing.  */
  }

  /* The GDB register number to use in this set.  */
  int next_regnum;

  /* The last GDB register number to use in this set.  */
  int last_regnum;
};

/* Arguments can be passed as on stack arguments, or by reference.  The
   on stack arguments must be in a continuous region starting from $sp,
   while the by reference arguments can be anywhere, but we'll put them
   on the stack after (at higher address) the on stack arguments.

   This might not be the right approach to take.  The ABI is clear that
   an argument passed by reference can be modified by the callee, which
   us placing the argument (temporarily) onto the stack will not achieve
   (changes will be lost).  There's also the possibility that very large
   arguments could overflow the stack.

   This struct is used to track offset into these two areas for where
   arguments are to be placed.  */
struct riscv_memory_offsets
{
  riscv_memory_offsets ()
    : arg_offset (0),
      ref_offset (0)
  {
    /* Nothing.  */
  }

  /* Offset into on stack argument area.  */
  int arg_offset;

  /* Offset into the pass by reference area.  */
  int ref_offset;
};

/* Holds information about where arguments to a call will be placed.  This
   is updated as arguments are added onto the call, and can be used to
   figure out where the next argument should be placed.  */

struct riscv_call_info
{
  riscv_call_info (struct gdbarch *gdbarch)
    : int_regs (RISCV_A0_REGNUM, RISCV_A0_REGNUM + 7),
      float_regs (RISCV_FA0_REGNUM, RISCV_FA0_REGNUM + 7)
  {
    xlen = riscv_abi_xlen (gdbarch);
    flen = riscv_abi_flen (gdbarch);

    /* Disable use of floating point registers if needed.  */
    if (!riscv_has_fp_abi (gdbarch))
      float_regs.next_regnum = float_regs.last_regnum + 1;
  }

  /* Track the memory areas used for holding in-memory arguments to a
     call.  */
  struct riscv_memory_offsets memory;

  /* Holds information about the next integer register to use for passing
     an argument.  */
  struct riscv_arg_reg int_regs;

  /* Holds information about the next floating point register to use for
     passing an argument.  */
  struct riscv_arg_reg float_regs;

  /* The XLEN and FLEN are copied in to this structure for convenience, and
     are just the results of calling RISCV_ABI_XLEN and RISCV_ABI_FLEN.  */
  int xlen;
  int flen;
};

/* Return the number of registers available for use as parameters in the
   register set REG.  Returned value can be 0 or more.  */

static int
riscv_arg_regs_available (struct riscv_arg_reg *reg)
{
  if (reg->next_regnum > reg->last_regnum)
    return 0;

  return (reg->last_regnum - reg->next_regnum + 1);
}

/* If there is at least one register available in the register set REG then
   the next register from REG is assigned to LOC and the length field of
   LOC is updated to LENGTH.  The register set REG is updated to indicate
   that the assigned register is no longer available and the function
   returns true.

   If there are no registers available in REG then the function returns
   false, and LOC and REG are unchanged.  */

static bool
riscv_assign_reg_location (struct riscv_arg_info::location *loc,
			   struct riscv_arg_reg *reg,
			   int length, int offset)
{
  if (reg->next_regnum <= reg->last_regnum)
    {
      loc->loc_type = riscv_arg_info::location::in_reg;
      loc->loc_data.regno = reg->next_regnum;
      reg->next_regnum++;
      loc->c_length = length;
      loc->c_offset = offset;
      return true;
    }

  return false;
}

/* Assign LOC a location as the next stack parameter, and update MEMORY to
   record that an area of stack has been used to hold the parameter
   described by LOC.

   The length field of LOC is updated to LENGTH, the length of the
   parameter being stored, and ALIGN is the alignment required by the
   parameter, which will affect how memory is allocated out of MEMORY.  */

static void
riscv_assign_stack_location (struct riscv_arg_info::location *loc,
			     struct riscv_memory_offsets *memory,
			     int length, int align)
{
  loc->loc_type = riscv_arg_info::location::on_stack;
  memory->arg_offset
    = align_up (memory->arg_offset, align);
  loc->loc_data.offset = memory->arg_offset;
  memory->arg_offset += length;
  loc->c_length = length;

  /* Offset is always 0, either we're the first location part, in which
     case we're reading content from the start of the argument, or we're
     passing the address of a reference argument, so 0.  */
  loc->c_offset = 0;
}

/* Update AINFO, which describes an argument that should be passed or
   returned using the integer ABI.  The argloc fields within AINFO are
   updated to describe the location in which the argument will be passed to
   a function, or returned from a function.

   The CINFO structure contains the ongoing call information, the holds
   information such as which argument registers are remaining to be
   assigned to parameter, and how much memory has been used by parameters
   so far.

   By examining the state of CINFO a suitable location can be selected,
   and assigned to AINFO.  */

static void
riscv_call_arg_scalar_int (struct riscv_arg_info *ainfo,
			   struct riscv_call_info *cinfo)
{
  if (ainfo->length > (2 * cinfo->xlen))
    {
      /* Argument is going to be passed by reference.  */
      ainfo->argloc[0].loc_type
	= riscv_arg_info::location::by_ref;
      cinfo->memory.ref_offset
	= align_up (cinfo->memory.ref_offset, ainfo->align);
      ainfo->argloc[0].loc_data.offset = cinfo->memory.ref_offset;
      cinfo->memory.ref_offset += ainfo->length;
      ainfo->argloc[0].c_length = ainfo->length;

      /* The second location for this argument is given over to holding the
	 address of the by-reference data.  Pass 0 for the offset as this
	 is not part of the actual argument value.  */
      if (!riscv_assign_reg_location (&ainfo->argloc[1],
				      &cinfo->int_regs,
				      cinfo->xlen, 0))
	riscv_assign_stack_location (&ainfo->argloc[1],
				     &cinfo->memory, cinfo->xlen,
				     cinfo->xlen);
    }
  else
    {
      int len = std::min (ainfo->length, cinfo->xlen);
      int align = std::max (ainfo->align, cinfo->xlen);

      /* Unnamed arguments in registers that require 2*XLEN alignment are
	 passed in an aligned register pair.  */
      if (ainfo->is_unnamed && (align == cinfo->xlen * 2)
	  && cinfo->int_regs.next_regnum & 1)
	cinfo->int_regs.next_regnum++;

      if (!riscv_assign_reg_location (&ainfo->argloc[0],
				      &cinfo->int_regs, len, 0))
	riscv_assign_stack_location (&ainfo->argloc[0],
				     &cinfo->memory, len, align);

      if (len < ainfo->length)
	{
	  len = ainfo->length - len;
	  if (!riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->int_regs, len,
					  cinfo->xlen))
	    riscv_assign_stack_location (&ainfo->argloc[1],
					 &cinfo->memory, len, cinfo->xlen);
	}
    }
}

/* Like RISCV_CALL_ARG_SCALAR_INT, except the argument described by AINFO
   is being passed with the floating point ABI.  */

static void
riscv_call_arg_scalar_float (struct riscv_arg_info *ainfo,
			     struct riscv_call_info *cinfo)
{
  if (ainfo->length > cinfo->flen || ainfo->is_unnamed)
    return riscv_call_arg_scalar_int (ainfo, cinfo);
  else
    {
      if (!riscv_assign_reg_location (&ainfo->argloc[0],
				      &cinfo->float_regs,
				      ainfo->length, 0))
	return riscv_call_arg_scalar_int (ainfo, cinfo);
    }
}

/* Like RISCV_CALL_ARG_SCALAR_INT, except the argument described by AINFO
   is a complex floating point argument, and is therefore handled
   differently to other argument types.  */

static void
riscv_call_arg_complex_float (struct riscv_arg_info *ainfo,
			      struct riscv_call_info *cinfo)
{
  if (ainfo->length <= (2 * cinfo->flen)
      && riscv_arg_regs_available (&cinfo->float_regs) >= 2
      && !ainfo->is_unnamed)
    {
      bool result;
      int len = ainfo->length / 2;

      result = riscv_assign_reg_location (&ainfo->argloc[0],
					  &cinfo->float_regs, len, 0);
      gdb_assert (result);

      result = riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->float_regs, len, len);
      gdb_assert (result);
    }
  else
    return riscv_call_arg_scalar_int (ainfo, cinfo);
}

/* A structure used for holding information about a structure type within
   the inferior program.  The RiscV ABI has special rules for handling some
   structures with a single field or with two fields.  The counting of
   fields here is done after flattening out all nested structures.  */

class riscv_struct_info
{
public:
  riscv_struct_info ()
    : m_number_of_fields (0),
      m_types { nullptr, nullptr },
      m_offsets { 0, 0 }
  {
    /* Nothing.  */
  }

  /* Analyse TYPE descending into nested structures, count the number of
     scalar fields and record the types of the first two fields found.  */
  void analyse (struct type *type)
  {
    analyse_inner (type, 0);
  }

  /* The number of scalar fields found in the analysed type.  This is
     currently only accurate if the value returned is 0, 1, or 2 as the
     analysis stops counting when the number of fields is 3.  This is
     because the RiscV ABI only has special cases for 1 or 2 fields,
     anything else we just don't care about.  */
  int number_of_fields () const
  { return m_number_of_fields; }

  /* Return the type for scalar field INDEX within the analysed type.  Will
     return nullptr if there is no field at that index.  Only INDEX values
     0 and 1 can be requested as the RiscV ABI only has special cases for
     structures with 1 or 2 fields.  */
  struct type *field_type (int index) const
  {
    gdb_assert (index < (sizeof (m_types) / sizeof (m_types[0])));
    return m_types[index];
  }

  /* Return the offset of scalar field INDEX within the analysed type. Will
     return 0 if there is no field at that index.  Only INDEX values 0 and
     1 can be requested as the RiscV ABI only has special cases for
     structures with 1 or 2 fields.  */
  int field_offset (int index) const
  {
    gdb_assert (index < (sizeof (m_offsets) / sizeof (m_offsets[0])));
    return m_offsets[index];
  }

private:
  /* The number of scalar fields found within the structure after recursing
     into nested structures.  */
  int m_number_of_fields;

  /* The types of the first two scalar fields found within the structure
     after recursing into nested structures.  */
  struct type *m_types[2];

  /* The offsets of the first two scalar fields found within the structure
     after recursing into nested structures.  */
  int m_offsets[2];

  /* Recursive core for ANALYSE, the OFFSET parameter tracks the byte
     offset from the start of the top level structure being analysed.  */
  void analyse_inner (struct type *type, int offset);
};

/* See description in class declaration.  */

void
riscv_struct_info::analyse_inner (struct type *type, int offset)
{
  unsigned int count = type->num_fields ();
  unsigned int i;

  for (i = 0; i < count; ++i)
    {
      if (TYPE_FIELD_LOC_KIND (type, i) != FIELD_LOC_KIND_BITPOS)
	continue;

      struct type *field_type = type->field (i).type ();
      field_type = check_typedef (field_type);
      int field_offset
	= offset + TYPE_FIELD_BITPOS (type, i) / TARGET_CHAR_BIT;

      switch (field_type->code ())
	{
	case TYPE_CODE_STRUCT:
	  analyse_inner (field_type, field_offset);
	  break;

	default:
	  /* RiscV only flattens out structures.  Anything else does not
	     need to be flattened, we just record the type, and when we
	     look at the analysis results we'll realise this is not a
	     structure we can special case, and pass the structure in
	     memory.  */
	  if (m_number_of_fields < 2)
	    {
	      m_types[m_number_of_fields] = field_type;
	      m_offsets[m_number_of_fields] = field_offset;
	    }
	  m_number_of_fields++;
	  break;
	}

      /* RiscV only has special handling for structures with 1 or 2 scalar
	 fields, any more than that and the structure is just passed in
	 memory.  We can safely drop out early when we find 3 or more
	 fields then.  */

      if (m_number_of_fields > 2)
	return;
    }
}

/* Like RISCV_CALL_ARG_SCALAR_INT, except the argument described by AINFO
   is a structure.  Small structures on RiscV have some special case
   handling in order that the structure might be passed in register.
   Larger structures are passed in memory.  After assigning location
   information to AINFO, CINFO will have been updated.  */

static void
riscv_call_arg_struct (struct riscv_arg_info *ainfo,
		       struct riscv_call_info *cinfo)
{
  if (riscv_arg_regs_available (&cinfo->float_regs) >= 1)
    {
      struct riscv_struct_info sinfo;

      sinfo.analyse (ainfo->type);
      if (sinfo.number_of_fields () == 1
	  && sinfo.field_type(0)->code () == TYPE_CODE_COMPLEX)
	{
	  /* The following is similar to RISCV_CALL_ARG_COMPLEX_FLOAT,
	     except we use the type of the complex field instead of the
	     type from AINFO, and the first location might be at a non-zero
	     offset.  */
	  if (TYPE_LENGTH (sinfo.field_type (0)) <= (2 * cinfo->flen)
	      && riscv_arg_regs_available (&cinfo->float_regs) >= 2
	      && !ainfo->is_unnamed)
	    {
	      bool result;
	      int len = TYPE_LENGTH (sinfo.field_type (0)) / 2;
	      int offset = sinfo.field_offset (0);

	      result = riscv_assign_reg_location (&ainfo->argloc[0],
						  &cinfo->float_regs, len,
						  offset);
	      gdb_assert (result);

	      result = riscv_assign_reg_location (&ainfo->argloc[1],
						  &cinfo->float_regs, len,
						  (offset + len));
	      gdb_assert (result);
	    }
	  else
	    riscv_call_arg_scalar_int (ainfo, cinfo);
	  return;
	}

      if (sinfo.number_of_fields () == 1
	  && sinfo.field_type(0)->code () == TYPE_CODE_FLT)
	{
	  /* The following is similar to RISCV_CALL_ARG_SCALAR_FLOAT,
	     except we use the type of the first scalar field instead of
	     the type from AINFO.  Also the location might be at a non-zero
	     offset.  */
	  if (TYPE_LENGTH (sinfo.field_type (0)) > cinfo->flen
	      || ainfo->is_unnamed)
	    riscv_call_arg_scalar_int (ainfo, cinfo);
	  else
	    {
	      int offset = sinfo.field_offset (0);
	      int len = TYPE_LENGTH (sinfo.field_type (0));

	      if (!riscv_assign_reg_location (&ainfo->argloc[0],
					      &cinfo->float_regs,
					      len, offset))
		riscv_call_arg_scalar_int (ainfo, cinfo);
	    }
	  return;
	}

      if (sinfo.number_of_fields () == 2
	  && sinfo.field_type(0)->code () == TYPE_CODE_FLT
	  && TYPE_LENGTH (sinfo.field_type (0)) <= cinfo->flen
	  && sinfo.field_type(1)->code () == TYPE_CODE_FLT
	  && TYPE_LENGTH (sinfo.field_type (1)) <= cinfo->flen
	  && riscv_arg_regs_available (&cinfo->float_regs) >= 2)
	{
	  int len0 = TYPE_LENGTH (sinfo.field_type (0));
	  int offset = sinfo.field_offset (0);
	  if (!riscv_assign_reg_location (&ainfo->argloc[0],
					  &cinfo->float_regs, len0, offset))
	    error (_("failed during argument setup"));

	  int len1 = TYPE_LENGTH (sinfo.field_type (1));
	  offset = sinfo.field_offset (1);
	  gdb_assert (len1 <= (TYPE_LENGTH (ainfo->type)
			       - TYPE_LENGTH (sinfo.field_type (0))));

	  if (!riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->float_regs,
					  len1, offset))
	    error (_("failed during argument setup"));
	  return;
	}

      if (sinfo.number_of_fields () == 2
	  && riscv_arg_regs_available (&cinfo->int_regs) >= 1
	  && (sinfo.field_type(0)->code () == TYPE_CODE_FLT
	      && TYPE_LENGTH (sinfo.field_type (0)) <= cinfo->flen
	      && is_integral_type (sinfo.field_type (1))
	      && TYPE_LENGTH (sinfo.field_type (1)) <= cinfo->xlen))
	{
	  int  len0 = TYPE_LENGTH (sinfo.field_type (0));
	  int offset = sinfo.field_offset (0);
	  if (!riscv_assign_reg_location (&ainfo->argloc[0],
					  &cinfo->float_regs, len0, offset))
	    error (_("failed during argument setup"));

	  int len1 = TYPE_LENGTH (sinfo.field_type (1));
	  offset = sinfo.field_offset (1);
	  gdb_assert (len1 <= cinfo->xlen);
	  if (!riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->int_regs, len1, offset))
	    error (_("failed during argument setup"));
	  return;
	}

      if (sinfo.number_of_fields () == 2
	  && riscv_arg_regs_available (&cinfo->int_regs) >= 1
	  && (is_integral_type (sinfo.field_type (0))
	      && TYPE_LENGTH (sinfo.field_type (0)) <= cinfo->xlen
	      && sinfo.field_type(1)->code () == TYPE_CODE_FLT
	      && TYPE_LENGTH (sinfo.field_type (1)) <= cinfo->flen))
	{
	  int len0 = TYPE_LENGTH (sinfo.field_type (0));
	  int len1 = TYPE_LENGTH (sinfo.field_type (1));

	  gdb_assert (len0 <= cinfo->xlen);
	  gdb_assert (len1 <= cinfo->flen);

	  int offset = sinfo.field_offset (0);
	  if (!riscv_assign_reg_location (&ainfo->argloc[0],
					  &cinfo->int_regs, len0, offset))
	    error (_("failed during argument setup"));

	  offset = sinfo.field_offset (1);
	  if (!riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->float_regs,
					  len1, offset))
	    error (_("failed during argument setup"));

	  return;
	}
    }

  /* Non of the structure flattening cases apply, so we just pass using
     the integer ABI.  */
  riscv_call_arg_scalar_int (ainfo, cinfo);
}

/* Assign a location to call (or return) argument AINFO, the location is
   selected from CINFO which holds information about what call argument
   locations are available for use next.  The TYPE is the type of the
   argument being passed, this information is recorded into AINFO (along
   with some additional information derived from the type).  IS_UNNAMED
   is true if this is an unnamed (stdarg) argument, this info is also
   recorded into AINFO.

   After assigning a location to AINFO, CINFO will have been updated.  */

static void
riscv_arg_location (struct gdbarch *gdbarch,
		    struct riscv_arg_info *ainfo,
		    struct riscv_call_info *cinfo,
		    struct type *type, bool is_unnamed)
{
  ainfo->type = type;
  ainfo->length = TYPE_LENGTH (ainfo->type);
  ainfo->align = type_align (ainfo->type);
  ainfo->is_unnamed = is_unnamed;
  ainfo->contents = nullptr;
  ainfo->argloc[0].c_length = 0;
  ainfo->argloc[1].c_length = 0;

  switch (ainfo->type->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_PTR:
      if (ainfo->length <= cinfo->xlen)
	{
	  ainfo->type = builtin_type (gdbarch)->builtin_long;
	  ainfo->length = cinfo->xlen;
	}
      else if (ainfo->length <= (2 * cinfo->xlen))
	{
	  ainfo->type = builtin_type (gdbarch)->builtin_long_long;
	  ainfo->length = 2 * cinfo->xlen;
	}

      /* Recalculate the alignment requirement.  */
      ainfo->align = type_align (ainfo->type);
      riscv_call_arg_scalar_int (ainfo, cinfo);
      break;

    case TYPE_CODE_FLT:
      riscv_call_arg_scalar_float (ainfo, cinfo);
      break;

    case TYPE_CODE_COMPLEX:
      riscv_call_arg_complex_float (ainfo, cinfo);
      break;

    case TYPE_CODE_STRUCT:
      riscv_call_arg_struct (ainfo, cinfo);
      break;

    default:
      riscv_call_arg_scalar_int (ainfo, cinfo);
      break;
    }
}

/* Used for printing debug information about the call argument location in
   INFO to STREAM.  The addresses in SP_REFS and SP_ARGS are the base
   addresses for the location of pass-by-reference and
   arguments-on-the-stack memory areas.  */

static void
riscv_print_arg_location (ui_file *stream, struct gdbarch *gdbarch,
			  struct riscv_arg_info *info,
			  CORE_ADDR sp_refs, CORE_ADDR sp_args)
{
  fprintf_unfiltered (stream, "type: '%s', length: 0x%x, alignment: 0x%x",
		      TYPE_SAFE_NAME (info->type), info->length, info->align);
  switch (info->argloc[0].loc_type)
    {
    case riscv_arg_info::location::in_reg:
      fprintf_unfiltered
	(stream, ", register %s",
	 gdbarch_register_name (gdbarch, info->argloc[0].loc_data.regno));
      if (info->argloc[0].c_length < info->length)
	{
	  switch (info->argloc[1].loc_type)
	    {
	    case riscv_arg_info::location::in_reg:
	      fprintf_unfiltered
		(stream, ", register %s",
		 gdbarch_register_name (gdbarch,
					info->argloc[1].loc_data.regno));
	      break;

	    case riscv_arg_info::location::on_stack:
	      fprintf_unfiltered (stream, ", on stack at offset 0x%x",
				  info->argloc[1].loc_data.offset);
	      break;

	    case riscv_arg_info::location::by_ref:
	    default:
	      /* The second location should never be a reference, any
		 argument being passed by reference just places its address
		 in the first location and is done.  */
	      error (_("invalid argument location"));
	      break;
	    }

	  if (info->argloc[1].c_offset > info->argloc[0].c_length)
	    fprintf_unfiltered (stream, " (offset 0x%x)",
				info->argloc[1].c_offset);
	}
      break;

    case riscv_arg_info::location::on_stack:
      fprintf_unfiltered (stream, ", on stack at offset 0x%x",
			  info->argloc[0].loc_data.offset);
      break;

    case riscv_arg_info::location::by_ref:
      fprintf_unfiltered
	(stream, ", by reference, data at offset 0x%x (%s)",
	 info->argloc[0].loc_data.offset,
	 core_addr_to_string (sp_refs + info->argloc[0].loc_data.offset));
      if (info->argloc[1].loc_type
	  == riscv_arg_info::location::in_reg)
	fprintf_unfiltered
	  (stream, ", address in register %s",
	   gdbarch_register_name (gdbarch, info->argloc[1].loc_data.regno));
      else
	{
	  gdb_assert (info->argloc[1].loc_type
		      == riscv_arg_info::location::on_stack);
	  fprintf_unfiltered
	    (stream, ", address on stack at offset 0x%x (%s)",
	     info->argloc[1].loc_data.offset,
	     core_addr_to_string (sp_args + info->argloc[1].loc_data.offset));
	}
      break;

    default:
      gdb_assert_not_reached (_("unknown argument location type"));
    }
}

/* Wrapper around REGCACHE->cooked_write.  Places the LEN bytes of DATA
   into a buffer that is at least as big as the register REGNUM, padding
   out the DATA with either 0x00, or 0xff.  For floating point registers
   0xff is used, for everyone else 0x00 is used.  */

static void
riscv_regcache_cooked_write (int regnum, const gdb_byte *data, int len,
			     struct regcache *regcache, int flen)
{
  gdb_byte tmp [sizeof (ULONGEST)];

  /* FP values in FP registers must be NaN-boxed.  */
  if (riscv_is_fp_regno_p (regnum) && len < flen)
    memset (tmp, -1, sizeof (tmp));
  else
    memset (tmp, 0, sizeof (tmp));
  memcpy (tmp, data, len);
  regcache->cooked_write (regnum, tmp);
}

/* Implement the push dummy call gdbarch callback.  */

static CORE_ADDR
riscv_push_dummy_call (struct gdbarch *gdbarch,
		       struct value *function,
		       struct regcache *regcache,
		       CORE_ADDR bp_addr,
		       int nargs,
		       struct value **args,
		       CORE_ADDR sp,
		       function_call_return_method return_method,
		       CORE_ADDR struct_addr)
{
  int i;
  CORE_ADDR sp_args, sp_refs;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  struct riscv_arg_info *arg_info =
    (struct riscv_arg_info *) alloca (nargs * sizeof (struct riscv_arg_info));

  struct riscv_call_info call_info (gdbarch);

  CORE_ADDR osp = sp;

  struct type *ftype = check_typedef (value_type (function));

  if (ftype->code () == TYPE_CODE_PTR)
    ftype = check_typedef (TYPE_TARGET_TYPE (ftype));

  /* We'll use register $a0 if we're returning a struct.  */
  if (return_method == return_method_struct)
    ++call_info.int_regs.next_regnum;

  for (i = 0; i < nargs; ++i)
    {
      struct value *arg_value;
      struct type *arg_type;
      struct riscv_arg_info *info = &arg_info[i];

      arg_value = args[i];
      arg_type = check_typedef (value_type (arg_value));

      riscv_arg_location (gdbarch, info, &call_info, arg_type,
			  TYPE_VARARGS (ftype) && i >= ftype->num_fields ());

      if (info->type != arg_type)
	arg_value = value_cast (info->type, arg_value);
      info->contents = value_contents (arg_value);
    }

  /* Adjust the stack pointer and align it.  */
  sp = sp_refs = align_down (sp - call_info.memory.ref_offset, SP_ALIGNMENT);
  sp = sp_args = align_down (sp - call_info.memory.arg_offset, SP_ALIGNMENT);

  if (riscv_debug_infcall > 0)
    {
      fprintf_unfiltered (gdb_stdlog, "dummy call args:\n");
      fprintf_unfiltered (gdb_stdlog, ": floating point ABI %s in use\n",
	       (riscv_has_fp_abi (gdbarch) ? "is" : "is not"));
      fprintf_unfiltered (gdb_stdlog, ": xlen: %d\n: flen: %d\n",
	       call_info.xlen, call_info.flen);
      if (return_method == return_method_struct)
	fprintf_unfiltered (gdb_stdlog,
			    "[*] struct return pointer in register $A0\n");
      for (i = 0; i < nargs; ++i)
	{
	  struct riscv_arg_info *info = &arg_info [i];

	  fprintf_unfiltered (gdb_stdlog, "[%2d] ", i);
	  riscv_print_arg_location (gdb_stdlog, gdbarch, info, sp_refs, sp_args);
	  fprintf_unfiltered (gdb_stdlog, "\n");
	}
      if (call_info.memory.arg_offset > 0
	  || call_info.memory.ref_offset > 0)
	{
	  fprintf_unfiltered (gdb_stdlog, "              Original sp: %s\n",
			      core_addr_to_string (osp));
	  fprintf_unfiltered (gdb_stdlog, "Stack required (for args): 0x%x\n",
			      call_info.memory.arg_offset);
	  fprintf_unfiltered (gdb_stdlog, "Stack required (for refs): 0x%x\n",
			      call_info.memory.ref_offset);
	  fprintf_unfiltered (gdb_stdlog, "          Stack allocated: %s\n",
			      core_addr_to_string_nz (osp - sp));
	}
    }

  /* Now load the argument into registers, or onto the stack.  */

  if (return_method == return_method_struct)
    {
      gdb_byte buf[sizeof (LONGEST)];

      store_unsigned_integer (buf, call_info.xlen, byte_order, struct_addr);
      regcache->cooked_write (RISCV_A0_REGNUM, buf);
    }

  for (i = 0; i < nargs; ++i)
    {
      CORE_ADDR dst;
      int second_arg_length = 0;
      const gdb_byte *second_arg_data;
      struct riscv_arg_info *info = &arg_info [i];

      gdb_assert (info->length > 0);

      switch (info->argloc[0].loc_type)
	{
	case riscv_arg_info::location::in_reg:
	  {
	    gdb_assert (info->argloc[0].c_length <= info->length);

	    riscv_regcache_cooked_write (info->argloc[0].loc_data.regno,
					 (info->contents
					  + info->argloc[0].c_offset),
					 info->argloc[0].c_length,
					 regcache, call_info.flen);
	    second_arg_length =
	      (((info->argloc[0].c_length + info->argloc[0].c_offset) < info->length)
	       ? info->argloc[1].c_length : 0);
	    second_arg_data = info->contents + info->argloc[1].c_offset;
	  }
	  break;

	case riscv_arg_info::location::on_stack:
	  dst = sp_args + info->argloc[0].loc_data.offset;
	  write_memory (dst, info->contents, info->length);
	  second_arg_length = 0;
	  break;

	case riscv_arg_info::location::by_ref:
	  dst = sp_refs + info->argloc[0].loc_data.offset;
	  write_memory (dst, info->contents, info->length);

	  second_arg_length = call_info.xlen;
	  second_arg_data = (gdb_byte *) &dst;
	  break;

	default:
	  gdb_assert_not_reached (_("unknown argument location type"));
	}

      if (second_arg_length > 0)
	{
	  switch (info->argloc[1].loc_type)
	    {
	    case riscv_arg_info::location::in_reg:
	      {
		gdb_assert ((riscv_is_fp_regno_p (info->argloc[1].loc_data.regno)
			     && second_arg_length <= call_info.flen)
			    || second_arg_length <= call_info.xlen);
		riscv_regcache_cooked_write (info->argloc[1].loc_data.regno,
					     second_arg_data,
					     second_arg_length,
					     regcache, call_info.flen);
	      }
	      break;

	    case riscv_arg_info::location::on_stack:
	      {
		CORE_ADDR arg_addr;

		arg_addr = sp_args + info->argloc[1].loc_data.offset;
		write_memory (arg_addr, second_arg_data, second_arg_length);
		break;
	      }

	    case riscv_arg_info::location::by_ref:
	    default:
	      /* The second location should never be a reference, any
		 argument being passed by reference just places its address
		 in the first location and is done.  */
	      error (_("invalid argument location"));
	      break;
	    }
	}
    }

  /* Set the dummy return value to bp_addr.
     A dummy breakpoint will be setup to execute the call.  */

  if (riscv_debug_infcall > 0)
    fprintf_unfiltered (gdb_stdlog, ": writing $ra = %s\n",
			core_addr_to_string (bp_addr));
  regcache_cooked_write_unsigned (regcache, RISCV_RA_REGNUM, bp_addr);

  /* Finally, update the stack pointer.  */

  if (riscv_debug_infcall > 0)
    fprintf_unfiltered (gdb_stdlog, ": writing $sp = %s\n",
			core_addr_to_string (sp));
  regcache_cooked_write_unsigned (regcache, RISCV_SP_REGNUM, sp);

  return sp;
}

/* Implement the return_value gdbarch method.  */

static enum return_value_convention
riscv_return_value (struct gdbarch  *gdbarch,
		    struct value *function,
		    struct type *type,
		    struct regcache *regcache,
		    gdb_byte *readbuf,
		    const gdb_byte *writebuf)
{
  struct riscv_call_info call_info (gdbarch);
  struct riscv_arg_info info;
  struct type *arg_type;

  arg_type = check_typedef (type);
  riscv_arg_location (gdbarch, &info, &call_info, arg_type, false);

  if (riscv_debug_infcall > 0)
    {
      fprintf_unfiltered (gdb_stdlog, "riscv return value:\n");
      fprintf_unfiltered (gdb_stdlog, "[R] ");
      riscv_print_arg_location (gdb_stdlog, gdbarch, &info, 0, 0);
      fprintf_unfiltered (gdb_stdlog, "\n");
    }

  if (readbuf != nullptr || writebuf != nullptr)
    {
	unsigned int arg_len;
	struct value *abi_val;
	gdb_byte *old_readbuf = nullptr;
	int regnum;

	/* We only do one thing at a time.  */
	gdb_assert (readbuf == nullptr || writebuf == nullptr);

	/* In some cases the argument is not returned as the declared type,
	   and we need to cast to or from the ABI type in order to
	   correctly access the argument.  When writing to the machine we
	   do the cast here, when reading from the machine the cast occurs
	   later, after extracting the value.  As the ABI type can be
	   larger than the declared type, then the read or write buffers
	   passed in might be too small.  Here we ensure that we are using
	   buffers of sufficient size.  */
	if (writebuf != nullptr)
	  {
	    struct value *arg_val = value_from_contents (arg_type, writebuf);
	    abi_val = value_cast (info.type, arg_val);
	    writebuf = value_contents_raw (abi_val);
	  }
	else
	  {
	    abi_val = allocate_value (info.type);
	    old_readbuf = readbuf;
	    readbuf = value_contents_raw (abi_val);
	  }
	arg_len = TYPE_LENGTH (info.type);

	switch (info.argloc[0].loc_type)
	  {
	    /* Return value in register(s).  */
	  case riscv_arg_info::location::in_reg:
	    {
	      regnum = info.argloc[0].loc_data.regno;
              gdb_assert (info.argloc[0].c_length <= arg_len);
              gdb_assert (info.argloc[0].c_length
			  <= register_size (gdbarch, regnum));

	      if (readbuf)
		{
		  gdb_byte *ptr = readbuf + info.argloc[0].c_offset;
		  regcache->cooked_read_part (regnum, 0,
					      info.argloc[0].c_length,
					      ptr);
		}

	      if (writebuf)
		{
		  const gdb_byte *ptr = writebuf + info.argloc[0].c_offset;
		  riscv_regcache_cooked_write (regnum, ptr,
					       info.argloc[0].c_length,
					       regcache, call_info.flen);
		}

	      /* A return value in register can have a second part in a
		 second register.  */
	      if (info.argloc[1].c_length > 0)
		{
		  switch (info.argloc[1].loc_type)
		    {
		    case riscv_arg_info::location::in_reg:
		      regnum = info.argloc[1].loc_data.regno;

                      gdb_assert ((info.argloc[0].c_length
				   + info.argloc[1].c_length) <= arg_len);
                      gdb_assert (info.argloc[1].c_length
				  <= register_size (gdbarch, regnum));

		      if (readbuf)
			{
			  readbuf += info.argloc[1].c_offset;
			  regcache->cooked_read_part (regnum, 0,
						      info.argloc[1].c_length,
						      readbuf);
			}

		      if (writebuf)
			{
			  const gdb_byte *ptr
			    = writebuf + info.argloc[1].c_offset;
			  riscv_regcache_cooked_write
			    (regnum, ptr, info.argloc[1].c_length,
			     regcache, call_info.flen);
			}
		      break;

		    case riscv_arg_info::location::by_ref:
		    case riscv_arg_info::location::on_stack:
		    default:
		      error (_("invalid argument location"));
		      break;
		    }
		}
	    }
	    break;

	    /* Return value by reference will have its address in A0.  */
	  case riscv_arg_info::location::by_ref:
	    {
	      ULONGEST addr;

	      regcache_cooked_read_unsigned (regcache, RISCV_A0_REGNUM,
					     &addr);
	      if (readbuf != nullptr)
		read_memory (addr, readbuf, info.length);
	      if (writebuf != nullptr)
		write_memory (addr, writebuf, info.length);
	    }
	    break;

	  case riscv_arg_info::location::on_stack:
	  default:
	    error (_("invalid argument location"));
	    break;
	  }

	/* This completes the cast from abi type back to the declared type
	   in the case that we are reading from the machine.  See the
	   comment at the head of this block for more details.  */
	if (readbuf != nullptr)
	  {
	    struct value *arg_val = value_cast (arg_type, abi_val);
	    memcpy (old_readbuf, value_contents_raw (arg_val),
		    TYPE_LENGTH (arg_type));
	  }
    }

  switch (info.argloc[0].loc_type)
    {
    case riscv_arg_info::location::in_reg:
      return RETURN_VALUE_REGISTER_CONVENTION;
    case riscv_arg_info::location::by_ref:
      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    case riscv_arg_info::location::on_stack:
    default:
      error (_("invalid argument location"));
    }
}

/* Implement the frame_align gdbarch method.  */

static CORE_ADDR
riscv_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 16);
}

/* Generate, or return the cached frame cache for the RiscV frame
   unwinder.  */

static struct riscv_unwind_cache *
riscv_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  CORE_ADDR pc, start_addr;
  struct riscv_unwind_cache *cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int numregs, regno;

  if ((*this_cache) != NULL)
    return (struct riscv_unwind_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct riscv_unwind_cache);
  cache->regs = trad_frame_alloc_saved_regs (this_frame);
  (*this_cache) = cache;

  /* Scan the prologue, filling in the cache.  */
  start_addr = get_frame_func (this_frame);
  pc = get_frame_pc (this_frame);
  riscv_scan_prologue (gdbarch, start_addr, pc, cache);

  /* We can now calculate the frame base address.  */
  cache->frame_base
    = (get_frame_register_signed (this_frame, cache->frame_base_reg)
       + cache->frame_base_offset);
  if (riscv_debug_unwinder)
    fprintf_unfiltered (gdb_stdlog, "Frame base is %s ($%s + 0x%x)\n",
                        core_addr_to_string (cache->frame_base),
                        gdbarch_register_name (gdbarch,
                                               cache->frame_base_reg),
                        cache->frame_base_offset);

  /* The prologue scanner sets the address of registers stored to the stack
     as the offset of that register from the frame base.  The prologue
     scanner doesn't know the actual frame base value, and so is unable to
     compute the exact address.  We do now know the frame base value, so
     update the address of registers stored to the stack.  */
  numregs = gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);
  for (regno = 0; regno < numregs; ++regno)
    {
      if (trad_frame_addr_p (cache->regs, regno))
	cache->regs[regno].addr += cache->frame_base;
    }

  /* The previous $pc can be found wherever the $ra value can be found.
     The previous $ra value is gone, this would have been stored be the
     previous frame if required.  */
  cache->regs[gdbarch_pc_regnum (gdbarch)] = cache->regs[RISCV_RA_REGNUM];
  trad_frame_set_unknown (cache->regs, RISCV_RA_REGNUM);

  /* Build the frame id.  */
  cache->this_id = frame_id_build (cache->frame_base, start_addr);

  /* The previous $sp value is the frame base value.  */
  trad_frame_set_value (cache->regs, gdbarch_sp_regnum (gdbarch),
			cache->frame_base);

  return cache;
}

/* Implement the this_id callback for RiscV frame unwinder.  */

static void
riscv_frame_this_id (struct frame_info *this_frame,
		     void **prologue_cache,
		     struct frame_id *this_id)
{
  struct riscv_unwind_cache *cache;

  try
    {
      cache = riscv_frame_cache (this_frame, prologue_cache);
      *this_id = cache->this_id;
    }
  catch (const gdb_exception_error &ex)
    {
      /* Ignore errors, this leaves the frame id as the predefined outer
         frame id which terminates the backtrace at this point.  */
    }
}

/* Implement the prev_register callback for RiscV frame unwinder.  */

static struct value *
riscv_frame_prev_register (struct frame_info *this_frame,
			   void **prologue_cache,
			   int regnum)
{
  struct riscv_unwind_cache *cache;

  cache = riscv_frame_cache (this_frame, prologue_cache);
  return trad_frame_get_prev_register (this_frame, cache->regs, regnum);
}

/* Structure defining the RiscV normal frame unwind functions.  Since we
   are the fallback unwinder (DWARF unwinder is used first), we use the
   default frame sniffer, which always accepts the frame.  */

static const struct frame_unwind riscv_frame_unwind =
{
  /*.type          =*/ NORMAL_FRAME,
  /*.stop_reason   =*/ default_frame_unwind_stop_reason,
  /*.this_id       =*/ riscv_frame_this_id,
  /*.prev_register =*/ riscv_frame_prev_register,
  /*.unwind_data   =*/ NULL,
  /*.sniffer       =*/ default_frame_sniffer,
  /*.dealloc_cache =*/ NULL,
  /*.prev_arch     =*/ NULL,
};

/* Extract a set of required target features out of INFO, specifically the
   bfd being executed is examined to see what target features it requires.
   IF there is no current bfd, or the bfd doesn't indicate any useful
   features then a RISCV_GDBARCH_FEATURES is returned in its default state.  */

static struct riscv_gdbarch_features
riscv_features_from_gdbarch_info (const struct gdbarch_info info)
{
  struct riscv_gdbarch_features features;

  /* Now try to improve on the defaults by looking at the binary we are
     going to execute.  We assume the user knows what they are doing and
     that the target will match the binary.  Remember, this code path is
     only used at all if the target hasn't given us a description, so this
     is really a last ditched effort to do something sane before giving
     up.  */
  if (info.abfd != NULL
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    {
      unsigned char eclass = elf_elfheader (info.abfd)->e_ident[EI_CLASS];
      int e_flags = elf_elfheader (info.abfd)->e_flags;

      if (eclass == ELFCLASS32)
	features.xlen = 4;
      else if (eclass == ELFCLASS64)
	features.xlen = 8;
      else
	internal_error (__FILE__, __LINE__,
			_("unknown ELF header class %d"), eclass);

      if (e_flags & EF_RISCV_FLOAT_ABI_DOUBLE)
	features.flen = 8;
      else if (e_flags & EF_RISCV_FLOAT_ABI_SINGLE)
	features.flen = 4;
    }

  return features;
}

/* Find a suitable default target description.  Use the contents of INFO,
   specifically the bfd object being executed, to guide the selection of a
   suitable default target description.  */

static const struct target_desc *
riscv_find_default_target_description (const struct gdbarch_info info)
{
  /* Extract desired feature set from INFO.  */
  struct riscv_gdbarch_features features
    = riscv_features_from_gdbarch_info (info);

  /* If the XLEN field is still 0 then we got nothing useful from INFO.  In
     this case we fall back to a minimal useful target, 8-byte x-registers,
     with no floating point.  */
  if (features.xlen == 0)
    features.xlen = 8;

  /* Now build a target description based on the feature set.  */
  return riscv_lookup_target_description (features);
}

/* All of the registers in REG_SET are checked for in FEATURE, TDESC_DATA
   is updated with the register numbers for each register as listed in
   REG_SET.  If any register marked as required in REG_SET is not found in
   FEATURE then this function returns false, otherwise, it returns true.  */

static bool
riscv_check_tdesc_feature (struct tdesc_arch_data *tdesc_data,
                           const struct tdesc_feature *main_feature,
                           const struct tdesc_feature *csr_feature,
                           const struct riscv_register_feature *reg_set,
                           std::vector<riscv_pending_register_alias> *aliases)
{
  for (const auto &reg : reg_set->registers)
    {
      bool found = reg.check (tdesc_data, main_feature, reg_set, aliases);

      if (!found && reg.required != RISCV_REG_OPTIONAL)
	{
	  if (reg.required == RISCV_REG_REQUIRED_MAYBE_CSR
	      && csr_feature != nullptr)
	    {
	      gdb_assert (main_feature != csr_feature);
	      found = reg.check (tdesc_data, csr_feature,  reg_set, aliases);
	    }
	  if (!found)
	    return false;
	}
    }

  return true;
}

/* Add all the expected register sets into GDBARCH.  */

static void
riscv_add_reggroups (struct gdbarch *gdbarch)
{
  /* Add predefined register groups.  */
  reggroup_add (gdbarch, all_reggroup);
  reggroup_add (gdbarch, save_reggroup);
  reggroup_add (gdbarch, restore_reggroup);
  reggroup_add (gdbarch, system_reggroup);
  reggroup_add (gdbarch, vector_reggroup);
  reggroup_add (gdbarch, general_reggroup);
  reggroup_add (gdbarch, float_reggroup);

  /* Add RISC-V specific register groups.  */
  reggroup_add (gdbarch, csr_reggroup);
}

/* Implement the "dwarf2_reg_to_regnum" gdbarch method.  */

static int
riscv_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (reg < RISCV_DWARF_REGNUM_X31)
    return RISCV_ZERO_REGNUM + (reg - RISCV_DWARF_REGNUM_X0);

  else if (reg < RISCV_DWARF_REGNUM_F31)
    return RISCV_FIRST_FP_REGNUM + (reg - RISCV_DWARF_REGNUM_F0);

  return -1;
}

/* Implement the gcc_target_options method.  We have to select the arch and abi
   from the feature info.  We have enough feature info to select the abi, but
   not enough info for the arch given all of the possible architecture
   extensions.  So choose reasonable defaults for now.  */

static std::string
riscv_gcc_target_options (struct gdbarch *gdbarch)
{
  int isa_xlen = riscv_isa_xlen (gdbarch);
  int isa_flen = riscv_isa_flen (gdbarch);
  int abi_xlen = riscv_abi_xlen (gdbarch);
  int abi_flen = riscv_abi_flen (gdbarch);
  std::string target_options;

  target_options = "-march=rv";
  if (isa_xlen == 8)
    target_options += "64";
  else
    target_options += "32";
  if (isa_flen == 8)
    target_options += "gc";
  else if (isa_flen == 4)
    target_options += "imafc";
  else
    target_options += "imac";

  target_options += " -mabi=";
  if (abi_xlen == 8)
    target_options += "lp64";
  else
    target_options += "ilp32";
  if (abi_flen == 8)
    target_options += "d";
  else if (abi_flen == 4)
    target_options += "f";

  /* The gdb loader doesn't handle link-time relaxation relocations.  */
  target_options += " -mno-relax";

  return target_options;
}

/* Call back from tdesc_use_registers, called for each unknown register
   found in the target description.

   See target-description.h (typedef tdesc_unknown_register_ftype) for a
   discussion of the arguments and return values.  */

static int
riscv_tdesc_unknown_reg (struct gdbarch *gdbarch, tdesc_feature *feature,
			 const char *reg_name, int possible_regnum)
{
  /* At one point in time GDB had an incorrect default target description
     that duplicated the fflags, frm, and fcsr registers in both the FPU
     and CSR register sets.

     Some targets (QEMU) copied these target descriptions into their source
     tree, and so we're currently stuck working with some targets that
     declare the same registers twice.

     There's not much we can do about this any more.  Assuming the target
     will direct a request for either register number to the correct
     underlying hardware register then it doesn't matter which one GDB
     uses, so long as we (GDB) are consistent (so that we don't end up with
     invalid cache misses).

     As we always scan the FPU registers first, then the CSRs, if the
     target has included the offending registers in both sets then we will
     always see the FPU copies here, as the CSR versions will replace them
     in the register list.

     To prevent these duplicates showing up in any of the register list,
     record their register numbers here.  */
  if (strcmp (tdesc_feature_name (feature), riscv_freg_feature.name) == 0)
    {
      struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
      int *regnum_ptr = nullptr;

      if (strcmp (reg_name, "fflags") == 0)
	regnum_ptr = &tdep->duplicate_fflags_regnum;
      else if (strcmp (reg_name, "frm") == 0)
	regnum_ptr = &tdep->duplicate_frm_regnum;
      else if (strcmp (reg_name, "fcsr") == 0)
	regnum_ptr = &tdep->duplicate_fcsr_regnum;

      if (regnum_ptr != nullptr)
	{
	  /* This means the register appears more than twice in the target
	     description.  Just let GDB add this as another register.
	     We'll have duplicates in the register name list, but there's
	     not much more we can do.  */
	  if (*regnum_ptr != -1)
	    return -1;

	  /* Record the number assigned to this register, then return the
	     number (so it actually gets assigned to this register).  */
	  *regnum_ptr = possible_regnum;
	  return possible_regnum;
	}
    }

  /* Any unknown registers in the CSR feature are recorded within a single
     block so we can easily identify these registers when making choices
     about register groups in riscv_register_reggroup_p.  */
  if (strcmp (tdesc_feature_name (feature), riscv_csr_feature.name) == 0)
    {
      struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
      if (tdep->unknown_csrs_first_regnum == -1)
	tdep->unknown_csrs_first_regnum = possible_regnum;
      gdb_assert (tdep->unknown_csrs_first_regnum
		  + tdep->unknown_csrs_count == possible_regnum);
      tdep->unknown_csrs_count++;
      return possible_regnum;
    }

  /* Some other unknown register.  Don't assign this a number now, it will
     be assigned a number automatically later by the target description
     handling code.  */
  return -1;
}

/* Implement the gnu_triplet_regexp method.  A single compiler supports both
   32-bit and 64-bit code, and may be named riscv32 or riscv64 or (not
   recommended) riscv.  */

static const char *
riscv_gnu_triplet_regexp (struct gdbarch *gdbarch)
{
  return "riscv(32|64)?";
}

/* Initialize the current architecture based on INFO.  If possible,
   re-use an architecture from ARCHES, which is a list of
   architectures already created during this debugging session.

   Called e.g. at program startup, when reading a core file, and when
   reading a binary file.  */

static struct gdbarch *
riscv_gdbarch_init (struct gdbarch_info info,
		    struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;
  struct riscv_gdbarch_features features;
  const struct target_desc *tdesc = info.target_desc;

  /* Ensure we always have a target description.  */
  if (!tdesc_has_registers (tdesc))
    tdesc = riscv_find_default_target_description (info);
  gdb_assert (tdesc);

  if (riscv_debug_gdbarch)
    fprintf_unfiltered (gdb_stdlog, "Have got a target description\n");

  const struct tdesc_feature *feature_cpu
    = tdesc_find_feature (tdesc, riscv_xreg_feature.name);
  const struct tdesc_feature *feature_fpu
    = tdesc_find_feature (tdesc, riscv_freg_feature.name);
  const struct tdesc_feature *feature_virtual
    = tdesc_find_feature (tdesc, riscv_virtual_feature.name);
  const struct tdesc_feature *feature_csr
    = tdesc_find_feature (tdesc, riscv_csr_feature.name);

  if (feature_cpu == NULL)
    return NULL;

  struct tdesc_arch_data *tdesc_data = tdesc_data_alloc ();
  std::vector<riscv_pending_register_alias> pending_aliases;

  bool valid_p = riscv_check_tdesc_feature (tdesc_data,
                                            feature_cpu, feature_csr,
                                            &riscv_xreg_feature,
                                            &pending_aliases);
  if (valid_p)
    {
      /* Check that all of the core cpu registers have the same bitsize.  */
      int xlen_bitsize = tdesc_register_bitsize (feature_cpu, "pc");

      for (auto &tdesc_reg : feature_cpu->registers)
        valid_p &= (tdesc_reg->bitsize == xlen_bitsize);

      if (riscv_debug_gdbarch)
        fprintf_filtered
          (gdb_stdlog,
           "From target-description, xlen = %d\n", xlen_bitsize);

      features.xlen = (xlen_bitsize / 8);
    }

  if (feature_fpu != NULL)
    {
      valid_p &= riscv_check_tdesc_feature (tdesc_data, feature_fpu,
					    feature_csr,
                                            &riscv_freg_feature,
                                            &pending_aliases);

      /* Search for the first floating point register (by any alias), to
         determine the bitsize.  */
      int bitsize = -1;
      const auto &fp0 = riscv_freg_feature.registers[0];

      for (const char *name : fp0.names)
	{
	  if (tdesc_unnumbered_register (feature_fpu, name))
	    {
	      bitsize = tdesc_register_bitsize (feature_fpu, name);
	      break;
	    }
	}

      gdb_assert (bitsize != -1);
      features.flen = (bitsize / 8);

      if (riscv_debug_gdbarch)
        fprintf_filtered
          (gdb_stdlog,
           "From target-description, flen = %d\n", bitsize);
    }
  else
    {
      features.flen = 0;

      if (riscv_debug_gdbarch)
        fprintf_filtered
          (gdb_stdlog,
           "No FPU in target-description, assume soft-float ABI\n");
    }

  if (feature_virtual)
    riscv_check_tdesc_feature (tdesc_data, feature_virtual, feature_csr,
                               &riscv_virtual_feature,
                               &pending_aliases);

  if (feature_csr)
    riscv_check_tdesc_feature (tdesc_data, feature_csr, nullptr,
                               &riscv_csr_feature,
                               &pending_aliases);

  if (!valid_p)
    {
      if (riscv_debug_gdbarch)
        fprintf_unfiltered (gdb_stdlog, "Target description is not valid\n");
      tdesc_data_cleanup (tdesc_data);
      return NULL;
    }

  /* Have a look at what the supplied (if any) bfd object requires of the
     target, then check that this matches with what the target is
     providing.  */
  struct riscv_gdbarch_features abi_features
    = riscv_features_from_gdbarch_info (info);
  /* In theory a binary compiled for RV32 could run on an RV64 target,
     however, this has not been tested in GDB yet, so for now we require
     that the requested xlen match the targets xlen.  */
  if (abi_features.xlen != 0 && abi_features.xlen != features.xlen)
    error (_("bfd requires xlen %d, but target has xlen %d"),
            abi_features.xlen, features.xlen);
  /* We do support running binaries compiled for 32-bit float on targets
     with 64-bit float, so we only complain if the binary requires more
     than the target has available.  */
  if (abi_features.flen > features.flen)
    error (_("bfd requires flen %d, but target has flen %d"),
            abi_features.flen, features.flen);

  /* If the ABI_FEATURES xlen is 0 then this indicates we got no useful abi
     features from the INFO object.  In this case we assume that the xlen
     abi matches the hardware.  */
  if (abi_features.xlen == 0)
    abi_features.xlen = features.xlen;

  /* Find a candidate among the list of pre-declared architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      /* Check that the feature set of the ARCHES matches the feature set
         we are looking for.  If it doesn't then we can't reuse this
         gdbarch.  */
      struct gdbarch_tdep *other_tdep = gdbarch_tdep (arches->gdbarch);

      if (other_tdep->isa_features != features
	  || other_tdep->abi_features != abi_features)
        continue;

      break;
    }

  if (arches != NULL)
    {
      tdesc_data_cleanup (tdesc_data);
      return arches->gdbarch;
    }

  /* None found, so create a new architecture from the information provided.  */
  tdep = new (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);
  tdep->isa_features = features;
  tdep->abi_features = abi_features;

  /* Target data types.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, riscv_isa_xlen (gdbarch) * 8);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_long_double_format (gdbarch, floatformats_ia64_quad);
  set_gdbarch_ptr_bit (gdbarch, riscv_isa_xlen (gdbarch) * 8);
  set_gdbarch_char_signed (gdbarch, 0);
  set_gdbarch_type_align (gdbarch, riscv_type_align);

  /* Information about the target architecture.  */
  set_gdbarch_return_value (gdbarch, riscv_return_value);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, riscv_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, riscv_sw_breakpoint_from_kind);
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  /* Functions to analyze frames.  */
  set_gdbarch_skip_prologue (gdbarch, riscv_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_frame_align (gdbarch, riscv_frame_align);

  /* Functions handling dummy frames.  */
  set_gdbarch_call_dummy_location (gdbarch, ON_STACK);
  set_gdbarch_push_dummy_code (gdbarch, riscv_push_dummy_code);
  set_gdbarch_push_dummy_call (gdbarch, riscv_push_dummy_call);

  /* Frame unwinders.  Use DWARF debug info if available, otherwise use our own
     unwinder.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &riscv_frame_unwind);

  /* Register architecture.  */
  riscv_add_reggroups (gdbarch);

  /* Internal <-> external register number maps.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, riscv_dwarf_reg_to_regnum);

  /* We reserve all possible register numbers for the known registers.
     This means the target description mechanism will add any target
     specific registers after this number.  This helps make debugging GDB
     just a little easier.  */
  set_gdbarch_num_regs (gdbarch, RISCV_LAST_REGNUM + 1);

  /* We don't have to provide the count of 0 here (its the default) but
     include this line to make it explicit that, right now, we don't have
     any pseudo registers on RISC-V.  */
  set_gdbarch_num_pseudo_regs (gdbarch, 0);

  /* Some specific register numbers GDB likes to know about.  */
  set_gdbarch_sp_regnum (gdbarch, RISCV_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, RISCV_PC_REGNUM);

  set_gdbarch_print_registers_info (gdbarch, riscv_print_registers_info);

  /* Finalise the target description registers.  */
  tdesc_use_registers (gdbarch, tdesc, tdesc_data, riscv_tdesc_unknown_reg);

  /* Override the register type callback setup by the target description
     mechanism.  This allows us to provide special type for floating point
     registers.  */
  set_gdbarch_register_type (gdbarch, riscv_register_type);

  /* Override the register name callback setup by the target description
     mechanism.  This allows us to force our preferred names for the
     registers, no matter what the target description called them.  */
  set_gdbarch_register_name (gdbarch, riscv_register_name);

  /* Override the register group callback setup by the target description
     mechanism.  This allows us to force registers into the groups we
     want, ignoring what the target tells us.  */
  set_gdbarch_register_reggroup_p (gdbarch, riscv_register_reggroup_p);

  /* Create register aliases for alternative register names.  We only
     create aliases for registers which were mentioned in the target
     description.  */
  for (const auto &alias : pending_aliases)
    alias.create (gdbarch);

  /* Compile command hooks.  */
  set_gdbarch_gcc_target_options (gdbarch, riscv_gcc_target_options);
  set_gdbarch_gnu_triplet_regexp (gdbarch, riscv_gnu_triplet_regexp);

  /* Hook in OS ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  register_riscv_ravenscar_ops (gdbarch);

  return gdbarch;
}

/* This decodes the current instruction and determines the address of the
   next instruction.  */

static CORE_ADDR
riscv_next_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct riscv_insn insn;
  CORE_ADDR next_pc;

  insn.decode (gdbarch, pc);
  next_pc = pc + insn.length ();

  if (insn.opcode () == riscv_insn::JAL)
    next_pc = pc + insn.imm_signed ();
  else if (insn.opcode () == riscv_insn::JALR)
    {
      LONGEST source;
      regcache->cooked_read (insn.rs1 (), &source);
      next_pc = (source + insn.imm_signed ()) & ~(CORE_ADDR) 0x1;
    }
  else if (insn.opcode () == riscv_insn::BEQ)
    {
      LONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 == src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BNE)
    {
      LONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 != src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BLT)
    {
      LONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 < src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BGE)
    {
      LONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 >= src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BLTU)
    {
      ULONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 < src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BGEU)
    {
      ULONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 >= src2)
	next_pc = pc + insn.imm_signed ();
    }

  return next_pc;
}

/* We can't put a breakpoint in the middle of a lr/sc atomic sequence, so look
   for the end of the sequence and put the breakpoint there.  */

static bool
riscv_next_pc_atomic_sequence (struct regcache *regcache, CORE_ADDR pc,
			       CORE_ADDR *next_pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct riscv_insn insn;
  CORE_ADDR cur_step_pc = pc;
  CORE_ADDR last_addr = 0;

  /* First instruction has to be a load reserved.  */
  insn.decode (gdbarch, cur_step_pc);
  if (insn.opcode () != riscv_insn::LR)
    return false;
  cur_step_pc = cur_step_pc + insn.length ();

  /* Next instruction should be branch to exit.  */
  insn.decode (gdbarch, cur_step_pc);
  if (insn.opcode () != riscv_insn::BNE)
    return false;
  last_addr = cur_step_pc + insn.imm_signed ();
  cur_step_pc = cur_step_pc + insn.length ();

  /* Next instruction should be store conditional.  */
  insn.decode (gdbarch, cur_step_pc);
  if (insn.opcode () != riscv_insn::SC)
    return false;
  cur_step_pc = cur_step_pc + insn.length ();

  /* Next instruction should be branch to start.  */
  insn.decode (gdbarch, cur_step_pc);
  if (insn.opcode () != riscv_insn::BNE)
    return false;
  if (pc != (cur_step_pc + insn.imm_signed ()))
    return false;
  cur_step_pc = cur_step_pc + insn.length ();

  /* We should now be at the end of the sequence.  */
  if (cur_step_pc != last_addr)
    return false;

  *next_pc = cur_step_pc;
  return true;
}

/* This is called just before we want to resume the inferior, if we want to
   single-step it but there is no hardware or kernel single-step support.  We
   find the target of the coming instruction and breakpoint it.  */

std::vector<CORE_ADDR>
riscv_software_single_step (struct regcache *regcache)
{
  CORE_ADDR pc, next_pc;

  pc = regcache_read_pc (regcache);

  if (riscv_next_pc_atomic_sequence (regcache, pc, &next_pc))
    return {next_pc};

  next_pc = riscv_next_pc (regcache, pc);

  return {next_pc};
}

/* Create RISC-V specific reggroups.  */

static void
riscv_init_reggroups ()
{
  csr_reggroup = reggroup_new ("csr", USER_REGGROUP);
}

void _initialize_riscv_tdep ();
void
_initialize_riscv_tdep ()
{
  riscv_create_csr_aliases ();
  riscv_init_reggroups ();

  gdbarch_register (bfd_arch_riscv, riscv_gdbarch_init, NULL);

  /* Add root prefix command for all "set debug riscv" and "show debug
     riscv" commands.  */
  add_basic_prefix_cmd ("riscv", no_class,
			_("RISC-V specific debug commands."),
			&setdebugriscvcmdlist, "set debug riscv ", 0,
			&setdebuglist);

  add_show_prefix_cmd ("riscv", no_class,
		       _("RISC-V specific debug commands."),
		       &showdebugriscvcmdlist, "show debug riscv ", 0,
		       &showdebuglist);

  add_setshow_zuinteger_cmd ("breakpoints", class_maintenance,
			     &riscv_debug_breakpoints,  _("\
Set riscv breakpoint debugging."), _("\
Show riscv breakpoint debugging."), _("\
When non-zero, print debugging information for the riscv specific parts\n\
of the breakpoint mechanism."),
			     NULL,
			     show_riscv_debug_variable,
			     &setdebugriscvcmdlist, &showdebugriscvcmdlist);

  add_setshow_zuinteger_cmd ("infcall", class_maintenance,
			     &riscv_debug_infcall,  _("\
Set riscv inferior call debugging."), _("\
Show riscv inferior call debugging."), _("\
When non-zero, print debugging information for the riscv specific parts\n\
of the inferior call mechanism."),
			     NULL,
			     show_riscv_debug_variable,
			     &setdebugriscvcmdlist, &showdebugriscvcmdlist);

  add_setshow_zuinteger_cmd ("unwinder", class_maintenance,
			     &riscv_debug_unwinder,  _("\
Set riscv stack unwinding debugging."), _("\
Show riscv stack unwinding debugging."), _("\
When non-zero, print debugging information for the riscv specific parts\n\
of the stack unwinding mechanism."),
			     NULL,
			     show_riscv_debug_variable,
			     &setdebugriscvcmdlist, &showdebugriscvcmdlist);

  add_setshow_zuinteger_cmd ("gdbarch", class_maintenance,
			     &riscv_debug_gdbarch,  _("\
Set riscv gdbarch initialisation debugging."), _("\
Show riscv gdbarch initialisation debugging."), _("\
When non-zero, print debugging information for the riscv gdbarch\n\
initialisation process."),
			     NULL,
			     show_riscv_debug_variable,
			     &setdebugriscvcmdlist, &showdebugriscvcmdlist);

  /* Add root prefix command for all "set riscv" and "show riscv" commands.  */
  add_basic_prefix_cmd ("riscv", no_class,
			_("RISC-V specific commands."),
			&setriscvcmdlist, "set riscv ", 0, &setlist);

  add_show_prefix_cmd ("riscv", no_class,
		       _("RISC-V specific commands."),
		       &showriscvcmdlist, "show riscv ", 0, &showlist);


  use_compressed_breakpoints = AUTO_BOOLEAN_AUTO;
  add_setshow_auto_boolean_cmd ("use-compressed-breakpoints", no_class,
				&use_compressed_breakpoints,
				_("\
Set debugger's use of compressed breakpoints."), _("	\
Show debugger's use of compressed breakpoints."), _("\
Debugging compressed code requires compressed breakpoints to be used. If\n\
left to 'auto' then gdb will use them if the existing instruction is a\n\
compressed instruction. If that doesn't give the correct behavior, then\n\
this option can be used."),
				NULL,
				show_use_compressed_breakpoints,
				&setriscvcmdlist,
				&showriscvcmdlist);
}
