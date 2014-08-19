/* Subroutines used for code generation on Renesas RL78 processors.
   Copyright (C) 2011-2013 Free Software Foundation, Inc.
   Contributed by Red Hat.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "rtl.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "insn-config.h"
#include "conditions.h"
#include "output.h"
#include "insn-attr.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "optabs.h"
#include "libfuncs.h"
#include "recog.h"
#include "diagnostic-core.h"
#include "toplev.h"
#include "reload.h"
#include "df.h"
#include "ggc.h"
#include "tm_p.h"
#include "debug.h"
#include "target.h"
#include "target-def.h"
#include "langhooks.h"
#include "rl78-protos.h"
#include "dumpfile.h"
#include "tree-pass.h"

static inline bool is_interrupt_func (const_tree decl);
static inline bool is_brk_interrupt_func (const_tree decl);
static void rl78_reorg (void);


/* Debugging statements are tagged with DEBUG0 only so that they can
   be easily enabled individually, by replacing the '0' with '1' as
   needed.  */
#define DEBUG0 0
#define DEBUG1 1

/* REGISTER_NAMES has the names for individual 8-bit registers, but
   these have the names we need to use when referring to 16-bit
   register pairs.  */
static const char * const word_regnames[] =
{
  "ax", "AX", "bc", "BC", "de", "DE", "hl", "HL",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
  "sp", "ap", "psw", "es", "cs"
};

struct GTY(()) machine_function
{
  /* If set, the rest of the fields have been computed.  */
  int computed;
  /* Which register pairs need to be pushed in the prologue.  */
  int need_to_push [FIRST_PSEUDO_REGISTER / 2];

  /* These fields describe the frame layout...  */
  /* arg pointer */
  /* 4 bytes for saved PC */
  int framesize_regs;
  /* frame pointer */
  int framesize_locals;
  int framesize_outgoing;
  /* stack pointer */
  int framesize;

  /* If set, recog is allowed to match against the "real" patterns.  */
  int real_insns_ok;
  /* If set, recog is allowed to match against the "virtual" patterns.  */
  int virt_insns_ok;
  /* Set if the current function needs to clean up any trampolines.  */
  int trampolines_used;
};

/* This is our init_machine_status, as set in
   rl78_option_override.  */
static struct machine_function *
rl78_init_machine_status (void)
{
  struct machine_function *m;

  m = ggc_alloc_cleared_machine_function ();
  m->virt_insns_ok = 1;

  return m;
}

/* Returns whether to run the devirtualization pass.  */
static bool
devirt_gate (void)
{
  return true;
}

/* Runs the devirtualization pass.  */
static unsigned int
devirt_pass (void)
{
  rl78_reorg ();
  return 0;
}

/* This pass converts virtual instructions using virtual registers, to
   real instructions using real registers.  Rather than run it as
   reorg, we reschedule it before vartrack to help with debugging.  */
static struct opt_pass rl78_devirt_pass =
{
  RTL_PASS,
  "devirt",
  OPTGROUP_NONE,                        /* optinfo_flags */
  devirt_gate,
  devirt_pass,
  NULL,
  NULL,
  212,
  TV_MACH_DEP,
  0, 0, 0,
  0,
  0
};

static struct register_pass_info rl78_devirt_info =
{
  & rl78_devirt_pass,
  "vartrack",
  1,
  PASS_POS_INSERT_BEFORE
};

#undef  TARGET_ASM_FILE_START
#define TARGET_ASM_FILE_START rl78_asm_file_start

static void
rl78_asm_file_start (void)
{
  int i;

  for (i = 0; i < 8; i++)
    {
      fprintf (asm_out_file, "r%d\t=\t0x%x\n", 8 + i, 0xffef0 + i);
      fprintf (asm_out_file, "r%d\t=\t0x%x\n", 16 + i, 0xffee8 + i);
    }

  register_pass (& rl78_devirt_info);
}


#undef  TARGET_OPTION_OVERRIDE
#define TARGET_OPTION_OVERRIDE		rl78_option_override

static void
rl78_option_override (void)
{
  flag_omit_frame_pointer = 1;
  flag_no_function_cse = 1;
  flag_split_wide_types = 0;

  init_machine_status = rl78_init_machine_status;
}

/* Most registers are 8 bits.  Some are 16 bits because, for example,
   gcc doesn't like dealing with $FP as a register pair.  This table
   maps register numbers to size in bytes.  */
static const int register_sizes[] =
{
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 2, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 1, 1, 1
};

/* Predicates used in the MD patterns.  This one is true when virtual
   insns may be matched, which typically means before (or during) the
   devirt pass.  */
bool
rl78_virt_insns_ok (void)
{
  if (cfun)
    return cfun->machine->virt_insns_ok;
  return true;
}

/* Predicates used in the MD patterns.  This one is true when real
   insns may be matched, which typically means after (or during) the
   devirt pass.  */
bool
rl78_real_insns_ok (void)
{
  if (cfun)
    return cfun->machine->real_insns_ok;
  return false;
}

/* Implements HARD_REGNO_NREGS.  */
int
rl78_hard_regno_nregs (int regno, enum machine_mode mode)
{
  int rs = register_sizes[regno];
  if (rs < 1)
    rs = 1;
  return ((GET_MODE_SIZE (mode) + rs - 1) / rs);
}

/* Implements HARD_REGNO_MODE_OK.  */
int
rl78_hard_regno_mode_ok (int regno, enum machine_mode mode)
{
  int s = GET_MODE_SIZE (mode);

  if (s < 1)
    return 0;
  /* These are not to be used by gcc.  */
  if (regno == 23 || regno == ES_REG || regno == CS_REG)
    return 0;
  /* $fp can alway sbe accessed as a 16-bit value.  */
  if (regno == FP_REG && s == 2)
    return 1;
  if (regno < SP_REG)
    {
      /* Since a reg-reg move is really a reg-mem move, we must
	 enforce alignment.  */
      if (s > 1 && (regno % 2))
	return 0;
      return 1;
    }
  if (s == CC_REGNUM)
    return (mode == BImode);
  /* All other registers must be accessed in their natural sizes.  */
  if (s == register_sizes [regno])
    return 1;
  return 0;
}

/* Simplify_gen_subreg() doesn't handle memory references the way we
   need it to below, so we use this function for when we must get a
   valid subreg in a "natural" state.  */
static rtx
rl78_subreg (enum machine_mode mode, rtx r, enum machine_mode omode, int byte)
{
  if (GET_CODE (r) == MEM)
    return adjust_address (r, mode, byte);
  else
    return simplify_gen_subreg (mode, r, omode, byte);
}

/* Used by movsi.  Split SImode moves into two HImode moves, using
   appropriate patterns for the upper and lower halves of symbols.  */
void
rl78_expand_movsi (rtx *operands)
{
  rtx op00, op02, op10, op12;

  op00 = rl78_subreg (HImode, operands[0], SImode, 0);
  op02 = rl78_subreg (HImode, operands[0], SImode, 2);
  if (GET_CODE (operands[1]) == CONST
      || GET_CODE (operands[1]) == SYMBOL_REF)
    {
      op10 = gen_rtx_ZERO_EXTRACT (HImode, operands[1], GEN_INT (16), GEN_INT (0));
      op10 = gen_rtx_CONST (HImode, op10);
      op12 = gen_rtx_ZERO_EXTRACT (HImode, operands[1], GEN_INT (16), GEN_INT (16));
      op12 = gen_rtx_CONST (HImode, op12);
    }
  else
    {
      op10 = rl78_subreg (HImode, operands[1], SImode, 0);
      op12 = rl78_subreg (HImode, operands[1], SImode, 2);
    }

  if (rtx_equal_p (operands[0], operands[1]))
    ;
  else if (rtx_equal_p (op00, op12))
    {
      emit_move_insn (op02, op12);
      emit_move_insn (op00, op10);
    }
  else
    {
      emit_move_insn (op00, op10);
      emit_move_insn (op02, op12);
    }
}

/* Used by various two-operand expanders which cannot accept all
   operands in the "far" namespace.  Force some such operands into
   registers so that each pattern has at most one far operand.  */
int
rl78_force_nonfar_2 (rtx *operands, rtx (*gen)(rtx,rtx))
{
  int did = 0;
  rtx temp_reg = NULL;

  /* FIXME: in the future, be smarter about only doing this if the
     other operand is also far, assuming the devirtualizer can also
     handle that.  */
  if (rl78_far_p (operands[0]))
    {
      temp_reg = operands[0];
      operands[0] = gen_reg_rtx (GET_MODE (operands[0]));
      did = 1;
    }
  if (!did)
    return 0;

  emit_insn (gen (operands[0], operands[1]));
  if (temp_reg)
    emit_move_insn (temp_reg, operands[0]);
  return 1;
}

/* Likewise, but for three-operand expanders.  */
int
rl78_force_nonfar_3 (rtx *operands, rtx (*gen)(rtx,rtx,rtx))
{
  int did = 0;
  rtx temp_reg = NULL;

  /* FIXME: Likewise.  */
  if (rl78_far_p (operands[1]))
    {
      rtx temp_reg = gen_reg_rtx (GET_MODE (operands[1]));
      emit_move_insn (temp_reg, operands[1]);
      operands[1] = temp_reg;
      did = 1;
    }
  if (rl78_far_p (operands[0]))
    {
      temp_reg = operands[0];
      operands[0] = gen_reg_rtx (GET_MODE (operands[0]));
      did = 1;
    }
  if (!did)
    return 0;

  emit_insn (gen (operands[0], operands[1], operands[2]));
  if (temp_reg)
    emit_move_insn (temp_reg, operands[0]);
  return 1;
}

#undef  TARGET_CAN_ELIMINATE
#define TARGET_CAN_ELIMINATE		rl78_can_eliminate

static bool
rl78_can_eliminate (const int from ATTRIBUTE_UNUSED, const int to ATTRIBUTE_UNUSED)
{
  return true;
}

/* Returns nonzero if the given register needs to be saved by the
   current function.  */
static int
need_to_save (int regno)
{
  if (is_interrupt_func (cfun->decl))
    {
      if (regno < 8)
	return 1; /* don't know what devirt will need */
      if (regno > 23)
	return 0; /* don't need to save interrupt registers */
      if (crtl->is_leaf)
	{
	  return df_regs_ever_live_p (regno);
	}
      else
	return 1;
    }
  if (regno == FRAME_POINTER_REGNUM && frame_pointer_needed)
    return 1;
  if (fixed_regs[regno])
    return 0;
  if (crtl->calls_eh_return)
    return 1;
  if (df_regs_ever_live_p (regno)
      && !call_used_regs[regno])
    return 1;
  return 0;
}

/* We use this to wrap all emitted insns in the prologue.  */
static rtx
F (rtx x)
{
  RTX_FRAME_RELATED_P (x) = 1;
  return x;
}

/* Compute all the frame-related fields in our machine_function
   structure.  */
static void
rl78_compute_frame_info (void)
{
  int i;

  cfun->machine->computed = 1;
  cfun->machine->framesize_regs = 0;
  cfun->machine->framesize_locals = get_frame_size ();
  cfun->machine->framesize_outgoing = crtl->outgoing_args_size;

  for (i = 0; i < 16; i ++)
    if (need_to_save (i * 2) || need_to_save (i * 2 + 1))
      {
	cfun->machine->need_to_push [i] = 1;
	cfun->machine->framesize_regs += 2;
      }
    else
      cfun->machine->need_to_push [i] = 0;

  if ((cfun->machine->framesize_locals + cfun->machine->framesize_outgoing) & 1)
    cfun->machine->framesize_locals ++;

  cfun->machine->framesize = (cfun->machine->framesize_regs
			      + cfun->machine->framesize_locals
			      + cfun->machine->framesize_outgoing);
}

/* Returns true if the provided function has the specified attribute.  */
static inline bool
has_func_attr (const_tree decl, const char * func_attr)
{
  if (decl == NULL_TREE)
    decl = current_function_decl;

  return lookup_attribute (func_attr, DECL_ATTRIBUTES (decl)) != NULL_TREE;
}

/* Returns true if the provided function has the "interrupt" attribute.  */
static inline bool
is_interrupt_func (const_tree decl)
{
  return has_func_attr (decl, "interrupt") || has_func_attr (decl, "brk_interrupt");
}

/* Returns true if the provided function has the "brk_interrupt" attribute.  */
static inline bool
is_brk_interrupt_func (const_tree decl)
{
  return has_func_attr (decl, "brk_interrupt");
}

/* Check "interrupt" attributes.  */
static tree
rl78_handle_func_attribute (tree * node,
			  tree   name,
			  tree   args,
			  int    flags ATTRIBUTE_UNUSED,
			  bool * no_add_attrs)
{
  gcc_assert (DECL_P (* node));
  gcc_assert (args == NULL_TREE);

  if (TREE_CODE (* node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute only applies to functions",
	       name);
      * no_add_attrs = true;
    }

  /* FIXME: We ought to check that the interrupt and exception
     handler attributes have been applied to void functions.  */
  return NULL_TREE;
}

#undef  TARGET_ATTRIBUTE_TABLE
#define TARGET_ATTRIBUTE_TABLE		rl78_attribute_table

/* Table of RL78-specific attributes.  */
const struct attribute_spec rl78_attribute_table[] =
{
  /* Name, min_len, max_len, decl_req, type_req, fn_type_req, handler,
     affects_type_identity.  */
  { "interrupt",      0, 0, true, false, false, rl78_handle_func_attribute,
    false },
  { "brk_interrupt",  0, 0, true, false, false, rl78_handle_func_attribute,
    false },
  { NULL,             0, 0, false, false, false, NULL, false }
};



/* Break down an address RTX into its component base/index/addend
   portions and return TRUE if the address is of a valid form, else
   FALSE.  */
static bool
characterize_address (rtx x, rtx *base, rtx *index, rtx *addend)
{
  *base = NULL_RTX;
  *index = NULL_RTX;
  *addend = NULL_RTX;

  if (GET_CODE (x) == REG)
    {
      *base = x;
      return true;
    }

  /* We sometimes get these without the CONST wrapper */
  if (GET_CODE (x) == PLUS
      && GET_CODE (XEXP (x, 0)) == SYMBOL_REF
      && GET_CODE (XEXP (x, 1)) == CONST_INT)
    {
      *addend = x;
      return true;
    }

  if (GET_CODE (x) == PLUS)
    {
      *base = XEXP (x, 0);
      x = XEXP (x, 1);

      if (GET_CODE (*base) != REG
	  && GET_CODE (x) == REG)
	{
	  rtx tmp = *base;
	  *base = x;
	  x = tmp;
	}

      if (GET_CODE (*base) != REG)
	return false;

      if (GET_CODE (x) == ZERO_EXTEND
	  && GET_CODE (XEXP (x, 0)) == REG)
	{
	  *index = XEXP (x, 0);
	  return false;
	}
    }

  switch (GET_CODE (x))
    {
    case PLUS:
      if (GET_CODE (XEXP (x, 0)) == SYMBOL_REF
	  && GET_CODE (XEXP (x, 0)) == CONST_INT)
	{
	  *addend = x;
	  return true;
	}
      /* fall through */
    case MEM:
    case REG:
      return false;

    case CONST:
    case SYMBOL_REF:
    case CONST_INT:
      *addend = x;
      return true;

    default:
      return false;
    }

  return false;
}

/* Used by the Whb constraint.  Match addresses that use HL+B or HL+C
   addressing.  */
bool
rl78_hl_b_c_addr_p (rtx op)
{
  rtx hl, bc;

  if (GET_CODE (op) != PLUS)
    return false;
  hl = XEXP (op, 0);
  bc = XEXP (op, 1);
  if (GET_CODE (hl) == ZERO_EXTEND)
    {
      rtx tmp = hl;
      hl = bc;
      bc = tmp;
    }
  if (GET_CODE (hl) != REG)
    return false;
  if (GET_CODE (bc) != ZERO_EXTEND)
    return false;
  bc = XEXP (bc, 0);
  if (GET_CODE (bc) != REG)
    return false;
  if (REGNO (hl) != HL_REG)
    return false;
  if (REGNO (bc) != B_REG && REGNO (bc) != C_REG)
    return false;

  return true;
}

#define REG_IS(r, regno) (((r) == (regno)) || ((r) >= FIRST_PSEUDO_REGISTER && !(strict)))

/* Used in various constraints and predicates to match operands in the
   "far" address space.  */
int
rl78_far_p (rtx x)
{
  if (GET_CODE (x) != MEM)
    return 0;
#if DEBUG0
  fprintf(stderr, "\033[35mrl78_far_p: "); debug_rtx(x);
  fprintf(stderr, " = %d\033[0m\n", MEM_ADDR_SPACE (x) == ADDR_SPACE_FAR);
#endif
  return MEM_ADDR_SPACE (x) == ADDR_SPACE_FAR;
}

/* Return the appropriate mode for a named address pointer.  */
#undef TARGET_ADDR_SPACE_POINTER_MODE
#define TARGET_ADDR_SPACE_POINTER_MODE rl78_addr_space_pointer_mode
static enum machine_mode
rl78_addr_space_pointer_mode (addr_space_t addrspace)
{
  switch (addrspace)
    {
    case ADDR_SPACE_GENERIC:
      return HImode;
    case ADDR_SPACE_FAR:
      return SImode;
    default:
      gcc_unreachable ();
    }
}

/* Return the appropriate mode for a named address address.  */
#undef TARGET_ADDR_SPACE_ADDRESS_MODE
#define TARGET_ADDR_SPACE_ADDRESS_MODE rl78_addr_space_address_mode
static enum machine_mode
rl78_addr_space_address_mode (addr_space_t addrspace)
{
  switch (addrspace)
    {
    case ADDR_SPACE_GENERIC:
      return HImode;
    case ADDR_SPACE_FAR:
      return SImode;
    default:
      gcc_unreachable ();
    }
}

#undef  TARGET_LEGITIMATE_CONSTANT_P
#define TARGET_LEGITIMATE_CONSTANT_P		rl78_is_legitimate_constant

static bool
rl78_is_legitimate_constant (enum machine_mode mode ATTRIBUTE_UNUSED, rtx x ATTRIBUTE_UNUSED)
{
  return true;
}

#undef  TARGET_ADDR_SPACE_LEGITIMATE_ADDRESS_P
#define TARGET_ADDR_SPACE_LEGITIMATE_ADDRESS_P	rl78_as_legitimate_address

bool
rl78_as_legitimate_address (enum machine_mode mode ATTRIBUTE_UNUSED, rtx x,
			    bool strict ATTRIBUTE_UNUSED, addr_space_t as ATTRIBUTE_UNUSED)
{
  rtx base, index, addend;

  if (as == ADDR_SPACE_GENERIC
      && GET_MODE (x) == SImode)
    return false;

  if (! characterize_address (x, &base, &index, &addend))
    return false;

  /* We can't extract the high/low portions of a PLUS address
     involving a register during devirtualization, so make sure all
     such __far addresses do not have addends.  This forces GCC to do
     the sum separately.  */
  if (addend && base && as == ADDR_SPACE_FAR)
    return false;

  if (base && index)
    {
      int ir = REGNO (index);
      int br = REGNO (base);

#define OK(test, debug) if (test) { /*fprintf(stderr, "%d: OK %s\n", __LINE__, debug);*/ return true; }
      OK (REG_IS (br, HL_REG) && REG_IS (ir, B_REG), "[hl+b]");
      OK (REG_IS (br, HL_REG) && REG_IS (ir, C_REG), "[hl+c]");
      return false;
    }

  if (strict && base && GET_CODE (base) == REG && REGNO (base) >= FIRST_PSEUDO_REGISTER)
    return false;

  return true;
}

/* Determine if one named address space is a subset of another.  */
#undef  TARGET_ADDR_SPACE_SUBSET_P
#define TARGET_ADDR_SPACE_SUBSET_P rl78_addr_space_subset_p
static bool
rl78_addr_space_subset_p (addr_space_t subset, addr_space_t superset)
{
  gcc_assert (subset == ADDR_SPACE_GENERIC || subset == ADDR_SPACE_FAR);
  gcc_assert (superset == ADDR_SPACE_GENERIC || superset == ADDR_SPACE_FAR);

  if (subset == superset)
    return true;

  else
    return (subset == ADDR_SPACE_GENERIC && superset == ADDR_SPACE_FAR);
}

#undef  TARGET_ADDR_SPACE_CONVERT
#define TARGET_ADDR_SPACE_CONVERT rl78_addr_space_convert
/* Convert from one address space to another.  */
static rtx
rl78_addr_space_convert (rtx op, tree from_type, tree to_type)
{
  addr_space_t from_as = TYPE_ADDR_SPACE (TREE_TYPE (from_type));
  addr_space_t to_as = TYPE_ADDR_SPACE (TREE_TYPE (to_type));
  rtx result;

  gcc_assert (from_as == ADDR_SPACE_GENERIC || from_as == ADDR_SPACE_FAR);
  gcc_assert (to_as == ADDR_SPACE_GENERIC || to_as == ADDR_SPACE_FAR);

  if (to_as == ADDR_SPACE_GENERIC && from_as == ADDR_SPACE_FAR)
    {
      /* This is unpredictable, as we're truncating off usable address
	 bits.  */

      result = gen_reg_rtx (HImode);
      emit_move_insn (result, simplify_subreg (HImode, op, SImode, 0));
      return result;
    }
  else if (to_as == ADDR_SPACE_FAR && from_as == ADDR_SPACE_GENERIC)
    {
      /* This always works.  */
      result = gen_reg_rtx (SImode);
      debug_rtx(result);
      debug_rtx(op);
      emit_move_insn (rl78_subreg (HImode, result, SImode, 0), op);
      emit_move_insn (rl78_subreg (HImode, result, SImode, 2), const0_rtx);
      return result;
    }
  else
    gcc_unreachable ();
}

/* Implements REGNO_MODE_CODE_OK_FOR_BASE_P.  */
bool
rl78_regno_mode_code_ok_for_base_p (int regno, enum machine_mode mode ATTRIBUTE_UNUSED,
				    addr_space_t address_space ATTRIBUTE_UNUSED,
				    int outer_code ATTRIBUTE_UNUSED, int index_code)
{
  if (regno <= SP_REG && regno >= 16)
    return true;
  if (index_code == REG)
    return (regno == HL_REG);
  if (regno == C_REG || regno == B_REG || regno == E_REG || regno == L_REG)
    return true;
  return false;
}

/* Implements MODE_CODE_BASE_REG_CLASS.  */
enum reg_class
rl78_mode_code_base_reg_class (enum machine_mode mode ATTRIBUTE_UNUSED,
			       addr_space_t address_space ATTRIBUTE_UNUSED,
			       int outer_code ATTRIBUTE_UNUSED,
			       int index_code ATTRIBUTE_UNUSED)
{
  return V_REGS;
}

/* Implements INITIAL_ELIMINATION_OFFSET.  The frame layout is
   described in the machine_Function struct definition, above.  */
int
rl78_initial_elimination_offset (int from, int to)
{
  int rv = 0; /* as if arg to arg */

  rl78_compute_frame_info ();

  switch (to)
    {
    case STACK_POINTER_REGNUM:
      rv += cfun->machine->framesize_outgoing;
      rv += cfun->machine->framesize_locals;
      /* Fall through.  */
    case FRAME_POINTER_REGNUM:
      rv += cfun->machine->framesize_regs;
      rv += 4;
      break;
    default:
      gcc_unreachable ();
    }

  switch (from)
    {
    case FRAME_POINTER_REGNUM:
      rv -= 4;
      rv -= cfun->machine->framesize_regs;
    case ARG_POINTER_REGNUM:
      break;
    default:
      gcc_unreachable ();
    }

  return rv;
}

/* Expand the function prologue (from the prologue pattern).  */
void
rl78_expand_prologue (void)
{
  int i, fs;
  rtx sp = gen_rtx_REG (HImode, STACK_POINTER_REGNUM);
  int rb = 0;

  if (!cfun->machine->computed)
    rl78_compute_frame_info ();

  if (flag_stack_usage_info)
    current_function_static_stack_size = cfun->machine->framesize;

  if (is_interrupt_func (cfun->decl))
    emit_insn (gen_sel_rb (GEN_INT (0)));

  for (i = 0; i < 16; i++)
    if (cfun->machine->need_to_push [i])
      {
	int need_bank = i/4;
	if (need_bank != rb)
	  {
	    emit_insn (gen_sel_rb (GEN_INT (need_bank)));
	    rb = need_bank;
	  }
	F (emit_insn (gen_push (gen_rtx_REG (HImode, i*2))));
      }
  if (rb != 0)
    emit_insn (gen_sel_rb (GEN_INT (0)));

  if (frame_pointer_needed)
    F (emit_move_insn (gen_rtx_REG (HImode, FRAME_POINTER_REGNUM),
		       gen_rtx_REG (HImode, STACK_POINTER_REGNUM)));

  fs = cfun->machine->framesize_locals + cfun->machine->framesize_outgoing;
  while (fs > 0)
    {
      int fs_byte = (fs > 254) ? 254 : fs;
      F (emit_insn (gen_subhi3 (sp, sp, GEN_INT (fs_byte))));
      fs -= fs_byte;
    }
}

/* Expand the function epilogue (from the epilogue pattern).  */
void
rl78_expand_epilogue (void)
{
  int i, fs;
  rtx sp = gen_rtx_REG (HImode, STACK_POINTER_REGNUM);
  int rb = 0;

  if (frame_pointer_needed)
    {
      emit_move_insn (gen_rtx_REG (HImode, STACK_POINTER_REGNUM),
		      gen_rtx_REG (HImode, FRAME_POINTER_REGNUM));
    }
  else
    {
      fs = cfun->machine->framesize_locals + cfun->machine->framesize_outgoing;
      while (fs > 0)
	{
	  int fs_byte = (fs > 254) ? 254 : fs;

	  emit_insn (gen_addhi3 (sp, sp, GEN_INT (fs_byte)));
	  fs -= fs_byte;
	}
    }

  for (i = 15; i >= 0; i--)
    if (cfun->machine->need_to_push [i])
      {
	int need_bank = i / 4;

	if (need_bank != rb)
	  {
	    emit_insn (gen_sel_rb (GEN_INT (need_bank)));
	    rb = need_bank;
	  }
	emit_insn (gen_pop (gen_rtx_REG (HImode, i * 2)));
      }

  if (rb != 0)
    emit_insn (gen_sel_rb (GEN_INT (0)));

  if (cfun->machine->trampolines_used)
    emit_insn (gen_trampoline_uninit ());

  if (is_brk_interrupt_func (cfun->decl))
    emit_jump_insn (gen_brk_interrupt_return ());
  else if (is_interrupt_func (cfun->decl))
    emit_jump_insn (gen_interrupt_return ());
  else
    emit_jump_insn (gen_rl78_return ());
}

/* Likewise, for exception handlers.  */
void
rl78_expand_eh_epilogue (rtx x ATTRIBUTE_UNUSED)
{
  /* FIXME - replace this with an indirect jump with stack adjust.  */
  emit_jump_insn (gen_rl78_return ());
}

#undef  TARGET_ASM_FUNCTION_PROLOGUE
#define TARGET_ASM_FUNCTION_PROLOGUE	rl78_start_function

/* We don't use this to actually emit the function prologue.  We use
   this to insert a comment in the asm file describing the
   function.  */
static void
rl78_start_function (FILE *file, HOST_WIDE_INT hwi_local ATTRIBUTE_UNUSED)
{
  int i;

  if (cfun->machine->framesize == 0)
    return;
  fprintf (file, "\t; start of function\n");

  if (cfun->machine->framesize_regs)
    {
      fprintf (file, "\t; push %d:", cfun->machine->framesize_regs);
      for (i = 0; i < 16; i ++)
	if (cfun->machine->need_to_push[i])
	  fprintf (file, " %s", word_regnames[i*2]);
      fprintf(file, "\n");
    }

  if (frame_pointer_needed)
    fprintf (file, "\t; $fp points here (r22)\n");

  if (cfun->machine->framesize_locals)
    fprintf (file, "\t; locals: %d byte%s\n", cfun->machine->framesize_locals,
	     cfun->machine->framesize_locals == 1 ? "" : "s");

  if (cfun->machine->framesize_outgoing)
    fprintf (file, "\t; outgoing: %d byte%s\n", cfun->machine->framesize_outgoing,
	     cfun->machine->framesize_outgoing == 1 ? "" : "s");
}

/* Return an RTL describing where a function return value of type RET_TYPE
   is held.  */

#undef  TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE		rl78_function_value

static rtx
rl78_function_value (const_tree ret_type,
		     const_tree fn_decl_or_type ATTRIBUTE_UNUSED,
		     bool       outgoing ATTRIBUTE_UNUSED)
{
  enum machine_mode mode = TYPE_MODE (ret_type);

  return gen_rtx_REG (mode, 8);
}

#undef  TARGET_PROMOTE_FUNCTION_MODE
#define TARGET_PROMOTE_FUNCTION_MODE rl78_promote_function_mode

static enum machine_mode
rl78_promote_function_mode (const_tree type ATTRIBUTE_UNUSED,
			    enum machine_mode mode,
			    int *punsignedp ATTRIBUTE_UNUSED,
			    const_tree funtype ATTRIBUTE_UNUSED, int for_return ATTRIBUTE_UNUSED)
{
  return mode;
}

/* Return an RTL expression describing the register holding a function
   parameter of mode MODE and type TYPE or NULL_RTX if the parameter should
   be passed on the stack.  CUM describes the previous parameters to the
   function and NAMED is false if the parameter is part of a variable
   parameter list, or the last named parameter before the start of a
   variable parameter list.  */

#undef  TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG     	rl78_function_arg

static rtx
rl78_function_arg (cumulative_args_t cum_v ATTRIBUTE_UNUSED,
		   enum machine_mode mode ATTRIBUTE_UNUSED,
		   const_tree type ATTRIBUTE_UNUSED,
		   bool named ATTRIBUTE_UNUSED)
{
  return NULL_RTX;
}

#undef  TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE     rl78_function_arg_advance

static void
rl78_function_arg_advance (cumulative_args_t cum_v, enum machine_mode mode, const_tree type,
			   bool named ATTRIBUTE_UNUSED)
{
  int rounded_size;
  CUMULATIVE_ARGS * cum = get_cumulative_args (cum_v);

  rounded_size = ((mode == BLKmode)
		  ? int_size_in_bytes (type) : GET_MODE_SIZE (mode));
  if (rounded_size & 1)
    rounded_size ++;
  (*cum) += rounded_size;
}

#undef  TARGET_FUNCTION_ARG_BOUNDARY
#define	TARGET_FUNCTION_ARG_BOUNDARY rl78_function_arg_boundary

static unsigned int
rl78_function_arg_boundary (enum machine_mode mode ATTRIBUTE_UNUSED,
			    const_tree type ATTRIBUTE_UNUSED)
{
  return 16;
}

/* Supported modifier letters:

   A - address of a MEM
   S - SADDR form of a real register
   v - real register corresponding to a virtual register
   m - minus - negative of CONST_INT value.
   c - inverse of a conditional (NE vs EQ for example)

   h - bottom HI of an SI
   H - top HI of an SI
   q - bottom QI of an HI
   Q - top QI of an HI
   e - third QI of an SI (i.e. where the ES register gets values from)

*/

/* Implements the bulk of rl78_print_operand, below.  We do it this
   way because we need to test for a constant at the top level and
   insert the '#', but not test for it anywhere else as we recurse
   down into the operand.  */
static void
rl78_print_operand_1 (FILE * file, rtx op, int letter)
{
  int need_paren;

  switch (GET_CODE (op))
    {
    case MEM:
      if (letter == 'A')
	rl78_print_operand_1 (file, XEXP (op, 0), letter);
      else
	{
	  if (rl78_far_p (op))
	    fprintf(file, "es:");
	  if (letter == 'H')
	    {
	      op = adjust_address (op, HImode, 2);
	      letter = 0;
	    }
	  if (letter == 'h')
	    {
	      op = adjust_address (op, HImode, 0);
	      letter = 0;
	    }
	  if (letter == 'Q')
	    {
	      op = adjust_address (op, QImode, 1);
	      letter = 0;
	    }
	  if (letter == 'q')
	    {
	      op = adjust_address (op, QImode, 0);
	      letter = 0;
	    }
	  if (letter == 'e')
	    {
	      op = adjust_address (op, QImode, 2);
	      letter = 0;
	    }
	  if (CONSTANT_P (XEXP (op, 0)))
	    {
	      fprintf(file, "!");
	      rl78_print_operand_1 (file, XEXP (op, 0), letter);
	    }
	  else if (GET_CODE (XEXP (op, 0)) == PLUS
		   && GET_CODE (XEXP (XEXP (op, 0), 0)) == SYMBOL_REF)
	    {
	      fprintf(file, "!");
	      rl78_print_operand_1 (file, XEXP (op, 0), letter);
	    }
	  else if (GET_CODE (XEXP (op, 0)) == PLUS
		   && GET_CODE (XEXP (XEXP (op, 0), 0)) == REG
		   && REGNO (XEXP (XEXP (op, 0), 0)) == 2)
	    {
	      rl78_print_operand_1 (file, XEXP (XEXP (op, 0), 1), 'u');
	      fprintf(file, "[");
	      rl78_print_operand_1 (file, XEXP (XEXP (op, 0), 0), 0);
	      fprintf(file, "]");
	    }
	  else
	    {
	      fprintf(file, "[");
	      rl78_print_operand_1 (file, XEXP (op, 0), letter);
	      fprintf(file, "]");
	    }
	}
      break;

    case REG:
      if (letter == 'Q')
	fprintf (file, "%s", reg_names [REGNO (op) | 1]);
      else if (letter == 'H')
	fprintf (file, "%s", reg_names [REGNO (op) + 2]);
      else if (letter == 'q')
	fprintf (file, "%s", reg_names [REGNO (op) & ~1]);
      else if (letter == 'e')
	fprintf (file, "%s", reg_names [REGNO (op) + 2]);
      else if (letter == 'S')
	fprintf (file, "0x%x", 0xffef8 + REGNO (op));
      else if (GET_MODE (op) == HImode
	       && ! (REGNO (op) & ~0xfe))
	{
	  if (letter == 'v')
	    fprintf (file, "%s", word_regnames [REGNO (op) % 8]);
	  else
	    fprintf (file, "%s", word_regnames [REGNO (op)]);
	}
      else
	fprintf (file, "%s", reg_names [REGNO (op)]);
      break;

    case CONST_INT:
      if (letter == 'Q')
	fprintf (file, "%ld", INTVAL (op) >> 8);
      else if (letter == 'H')
	fprintf (file, "%ld", INTVAL (op) >> 16);
      else if (letter == 'q')
	fprintf (file, "%ld", INTVAL (op) & 0xff);
      else if (letter == 'h')
	fprintf (file, "%ld", INTVAL (op) & 0xffff);
      else if (letter == 'e')
	fprintf (file, "%ld", (INTVAL (op) >> 16) & 0xff);
      else if (letter == 'm')
	fprintf (file, "%ld", - INTVAL (op));
      else
	fprintf(file, "%ld", INTVAL (op));
      break;

    case CONST:
      rl78_print_operand_1 (file, XEXP (op, 0), letter);
      break;

    case ZERO_EXTRACT:
      {
	int bits = INTVAL (XEXP (op, 1));
	int ofs = INTVAL (XEXP (op, 2));
	if (bits == 16 && ofs == 0)
	  fprintf (file, "%%lo16(");
	else if (bits == 16 && ofs == 16)
	  fprintf (file, "%%hi16(");
	else if (bits == 8 && ofs == 16)
	  fprintf (file, "%%hi8(");
	else
	  gcc_unreachable ();
	rl78_print_operand_1 (file, XEXP (op, 0), 0);
	fprintf (file, ")");
      }
      break;

    case ZERO_EXTEND:
      if (GET_CODE (XEXP (op, 0)) == REG)
	fprintf (file, "%s", reg_names [REGNO (XEXP (op, 0))]);
      else
	print_rtl (file, op);
      break;

    case PLUS:
      need_paren = 0;
      if (letter == 'H')
	{
	  fprintf (file, "%%hi16(");
	  need_paren = 1;
	  letter = 0;
	}
      if (letter == 'h')
	{
	  fprintf (file, "%%lo16(");
	  need_paren = 1;
	  letter = 0;
	}
      if (letter == 'e')
	{
	  fprintf (file, "%%hi8(");
	  need_paren = 1;
	  letter = 0;
	}
      if (letter == 'q' || letter == 'Q')
	output_operand_lossage ("q/Q modifiers invalid for symbol references");

      if (GET_CODE (XEXP (op, 0)) == ZERO_EXTEND)
	{
	  rl78_print_operand_1 (file, XEXP (op, 1), letter);
	  fprintf (file, "+");
	  rl78_print_operand_1 (file, XEXP (op, 0), letter);
	}
      else
	{
	  rl78_print_operand_1 (file, XEXP (op, 0), letter);
	  fprintf (file, "+");
	  rl78_print_operand_1 (file, XEXP (op, 1), letter);
	}
      if (need_paren)
	fprintf (file, ")");
      break;

    case SYMBOL_REF:
      need_paren = 0;
      if (letter == 'H')
	{
	  fprintf (file, "%%hi16(");
	  need_paren = 1;
	  letter = 0;
	}
      if (letter == 'h')
	{
	  fprintf (file, "%%lo16(");
	  need_paren = 1;
	  letter = 0;
	}
      if (letter == 'e')
	{
	  fprintf (file, "%%hi8(");
	  need_paren = 1;
	  letter = 0;
	}
      if (letter == 'q' || letter == 'Q')
	output_operand_lossage ("q/Q modifiers invalid for symbol references");

      output_addr_const (file, op);
      if (need_paren)
	fprintf (file, ")");
      break;

    case CODE_LABEL:
    case LABEL_REF:
      output_asm_label (op);
      break;

    case LTU:
      fprintf (file, letter == 'c' ? "nc" : "c");
      break;
    case LEU:
      fprintf (file, letter == 'c' ? "h" : "nh");
      break;
    case GEU:
      fprintf (file, letter == 'c' ? "c" : "nc");
      break;
    case GTU:
      fprintf (file, letter == 'c' ? "nh" : "h");
      break;
    case EQ:
      fprintf (file, letter == 'c' ? "nz" : "z");
      break;
    case NE:
      fprintf (file, letter == 'c' ? "z" : "nz");
      break;

    default:
      fprintf (file, "(%s)", GET_RTX_NAME (GET_CODE (op)));
      break;
    }
}

#undef  TARGET_PRINT_OPERAND
#define TARGET_PRINT_OPERAND		rl78_print_operand

static void
rl78_print_operand (FILE * file, rtx op, int letter)
{
  if (CONSTANT_P (op) && letter != 'u')
    fprintf (file, "#");
  rl78_print_operand_1 (file, op, letter);
}

#undef  TARGET_TRAMPOLINE_INIT
#define TARGET_TRAMPOLINE_INIT rl78_trampoline_init

/* Note that the RL78's addressing makes it very difficult to do
   trampolines on the stack.  So, libgcc has a small pool of
   trampolines from which one is allocated to this task.  */
static void
rl78_trampoline_init (rtx m_tramp, tree fndecl, rtx static_chain)
{
  rtx mov_addr, thunk_addr;
  rtx function = XEXP (DECL_RTL (fndecl), 0);

  mov_addr = adjust_address (m_tramp, HImode, 0);
  thunk_addr = gen_reg_rtx (HImode);

  function = force_reg (HImode, function);
  static_chain = force_reg (HImode, static_chain);

  emit_insn (gen_trampoline_init (thunk_addr, function, static_chain));
  emit_move_insn (mov_addr, thunk_addr);

  cfun->machine->trampolines_used = 1;
}

#undef  TARGET_TRAMPOLINE_ADJUST_ADDRESS
#define TARGET_TRAMPOLINE_ADJUST_ADDRESS rl78_trampoline_adjust_address

static rtx
rl78_trampoline_adjust_address (rtx m_tramp)
{
  rtx x = gen_rtx_MEM (HImode, m_tramp);
  return x;
}

/* Expander for cbranchqi4 and cbranchhi4.  RL78 is missing some of
   the "normal" compares, specifically, it only has unsigned compares,
   so we must synthesize the missing ones.  */
void
rl78_expand_compare (rtx *operands)
{
  /* RL78 does not have signed comparisons.  We must modify the
     operands to be in the unsigned range, and emit an unsigned
     comparison.  */

  enum machine_mode mode;
  rtx high_bit;
  int i;
  RTX_CODE new_cond;

  switch (GET_CODE (operands[0]))
    {
    case GE:
      new_cond = GEU;
      break;
    case LE:
      new_cond = LEU;
      break;
    case GT:
      new_cond = GTU;
      break;
    case LT:
      new_cond = LTU;
      break;
    default:
      return;
    }

#if DEBUG0
  fprintf (stderr, "\033[38;5;129mrl78_expand_compare\n");
  debug_rtx (operands[0]);
  fprintf (stderr, "\033[0m");
#endif

  mode = GET_MODE (operands[1]);
  if (mode == VOIDmode)
    mode = GET_MODE (operands[2]);
  high_bit = GEN_INT (~0 << (GET_MODE_BITSIZE (mode) - 1));

  /* 0: conditional 1,2: operands */
  for (i = 1; i <= 2; i ++)
    {
      rtx r = operands[i];

      if (GET_CODE (r) == CONST_INT)
	r = GEN_INT (INTVAL (r) ^ INTVAL (high_bit));
      else
	{
	  r = gen_rtx_PLUS (mode, operands[i], high_bit);
	  r = copy_to_mode_reg (mode, r);
	}
      operands[i] = r;
    }

  operands[0] = gen_rtx_fmt_ee (new_cond, GET_MODE (operands[0]), operands[1], operands[2]);

#if DEBUG0
  fprintf (stderr, "\033[38;5;142mrl78_expand_compare\n");
  debug_rtx (operands[0]);
  fprintf (stderr, "\033[0m");
#endif
}



/* Define this to 1 if you are debugging the peephole optimizers.  */
#define DEBUG_PEEP 0

/* Predicate used to enable the peephole2 patterns in rl78-virt.md.
   The default "word" size is a byte so we can effectively use all the
   registers, but we want to do 16-bit moves whenever possible.  This
   function determines when such a move is an option.  */
bool
rl78_peep_movhi_p (rtx *operands)
{
  int i;
  rtx m, a;

  /* (set (op0) (op1))
     (set (op2) (op3)) */

#if DEBUG_PEEP
  fprintf (stderr, "\033[33m");
  debug_rtx(operands[0]);
  debug_rtx(operands[1]);
  debug_rtx(operands[2]);
  debug_rtx(operands[3]);
  fprintf (stderr, "\033[0m");
#endif

  if (rtx_equal_p (operands[0], operands[3]))
    {
#if DEBUG_PEEP
      fprintf (stderr, "no peep: overlapping\n");
#endif
      return false;
    }

  for (i = 0; i < 2; i ++)
    {
      if (GET_CODE (operands[i]) != GET_CODE (operands[i+2]))
	{
#if DEBUG_PEEP
	  fprintf (stderr, "no peep: different codes\n");
#endif
	  return false;
	}
      if (GET_MODE (operands[i]) != GET_MODE (operands[i+2]))
	{
#if DEBUG_PEEP
	  fprintf (stderr, "no peep: different modes\n");
#endif
	  return false;
	}

      switch (GET_CODE (operands[i]))
	{
	case REG:
	  /*   LSB                      MSB  */
	  if (REGNO (operands[i]) + 1 != REGNO (operands[i+2])
	      || GET_MODE (operands[i]) != QImode)
	    {
#if DEBUG_PEEP
	      fprintf (stderr, "no peep: wrong regnos %d %d %d\n",
		       REGNO (operands[i]), REGNO (operands[i+2]),
		       i);
#endif
	      return false;
	    }
	  if (! rl78_hard_regno_mode_ok (REGNO (operands[i]), HImode))
	    {
#if DEBUG_PEEP
	      fprintf (stderr, "no peep: reg %d not HI\n", REGNO (operands[i]));
#endif
	      return false;
	    }
	  break;

	case CONST_INT:
	  break;

	case MEM:
	  if (GET_MODE (operands[i]) != QImode)
	    return false;
	  if (MEM_ALIGN (operands[i]) < 16)
	    return false;
	  a = XEXP (operands[i], 0);
	  if (GET_CODE (a) == CONST)
	    a = XEXP (a, 0);
	  if (GET_CODE (a) == PLUS)
	    a = XEXP (a, 1);
	  if (GET_CODE (a) == CONST_INT
	      && INTVAL (a) & 1)
	    {
#if DEBUG_PEEP
	      fprintf (stderr, "no peep: misaligned mem %d\n", i);
	      debug_rtx (operands[i]);
#endif
	      return false;
	    }
	  m = adjust_address (operands[i], QImode, 1);
	  if (! rtx_equal_p (m, operands[i+2]))
	    {
#if DEBUG_PEEP
	      fprintf (stderr, "no peep: wrong mem %d\n", i);
	      debug_rtx(m);
	      debug_rtx (operands[i+2]);
#endif
	      return false;
	    }
	  break;

	default:
#if DEBUG_PEEP
	  fprintf (stderr, "no peep: wrong rtx %d\n", i);
#endif
	  return false;
	}
    }
#if DEBUG_PEEP
  fprintf (stderr, "\033[32mpeep!\033[0m\n");
#endif
  return true;
}

/* Likewise, when a peephole is activated, this function helps compute
   the new operands.  */
void
rl78_setup_peep_movhi (rtx *operands)
{
  int i;

  for (i = 0; i < 2; i ++)
    {
      switch (GET_CODE (operands[i]))
	{
	case REG:
	  operands[i+4] = gen_rtx_REG (HImode, REGNO (operands[i]));
	  break;

	case CONST_INT:
	  operands[i+4] = GEN_INT ((INTVAL (operands[i]) & 0xff) + ((char)INTVAL (operands[i+2])) * 256);
	  break;

	case MEM:
	  operands[i+4] = adjust_address (operands[i], HImode, 0);
	  break;

	default:
	  break;
	}
    }
}

/*
	How Devirtualization works in the RL78 GCC port

Background

The RL78 is an 8-bit port with some 16-bit operations.  It has 32
bytes of register space, in four banks, memory-mapped.  One bank is
the "selected" bank and holds the registers used for primary
operations.  Since the registers are memory mapped, often you can
still refer to the unselected banks via memory accesses.

Virtual Registers

The GCC port uses bank 0 as the "selected" registers (A, X, BC, etc)
and refers to the other banks via their memory addresses, although
they're treated as regular registers internally.  These "virtual"
registers are R8 through R23 (bank3 is reserved for asm-based
interrupt handlers).

There are four machine description files:

rl78.md        - common register-independent patterns and definitions
rl78-expand.md - expanders
rl78-virt.md   - patterns that match BEFORE devirtualization
rl78-real.md   - patterns that match AFTER devirtualization

At least through register allocation and reload, gcc is told that it
can do pretty much anything - but may only use the virtual registers.
GCC cannot properly create the varying addressing modes that the RL78
supports in an efficient way.

Sometime after reload, the RL78 backend "devirtualizes" the RTL.  It
uses the "valloc" attribute in rl78-virt.md for determining the rules
by which it will replace virtual registers with real registers (or
not) and how to make up addressing modes.  For example, insns tagged
with "ro1" have a single read-only parameter, which may need to be
moved from memory/constant/vreg to a suitable real register.  As part
of devirtualization, a flag is toggled, disabling the rl78-virt.md
patterns and enabling the rl78-real.md patterns.  The new patterns'
constraints are used to determine the real registers used.  NOTE:
patterns in rl78-virt.md essentially ignore the constrains and rely on
predicates, where the rl78-real.md ones essentially ignore the
predicates and rely on the constraints.

The devirtualization pass is scheduled via the pass manager (despite
being called "rl78_reorg") so it can be scheduled prior to var-track
(the idea is to let gdb know about the new registers).  Ideally, it
would be scheduled right after pro/epilogue generation, so the
post-reload optimizers could operate on the real registers, but when I
tried that there were some issues building the target libraries.

During devirtualization, a simple register move optimizer is run.  It
would be better to run a full CSE/propogation pass on it through, or
re-run regmove, but that has not yet been attempted.

 */
#define DEBUG_ALLOC 0

/* Rescans an insn to see if it's recognized again.  This is done
   carefully to ensure that all the constraint information is accurate
   for the newly matched insn.  */
static bool
insn_ok_now (rtx insn)
{
  INSN_CODE (insn) = -1;
  if (recog (PATTERN (insn), insn, 0) > -1)
    {
      extract_insn (insn);
      if (constrain_operands (1))
	{
#if DEBUG_ALLOC
	  fprintf (stderr, "\033[32m");
	  debug_rtx (insn);
	  fprintf (stderr, "\033[0m");
#endif
	  return true;
	}
    }
  else
    {
      fprintf (stderr, "\033[41;30m Unrecognized insn \033[0m\n");
      debug_rtx (insn);
      gcc_unreachable ();
    }
#if DEBUG_ALLOC
  fprintf (stderr, "\033[31m");
  debug_rtx (insn);
  fprintf (stderr, "\033[0m");
#endif
  return false;
}

#if DEBUG_ALLOC
#define WORKED fprintf (stderr, "\033[48;5;22m Worked at line %d \033[0m\n", __LINE__)
#define FAILEDSOFAR fprintf (stderr, "\033[48;5;52m FAILED at line %d \033[0m\n", __LINE__)
#define FAILED fprintf (stderr, "\033[48;5;52m FAILED at line %d \033[0m\n", __LINE__), gcc_unreachable()
#define MAYBE_OK(insn) if (insn_ok_now (insn)) { WORKED; return; } else { FAILEDSOFAR; }
#else
#define WORKED
#define FAILEDSOFAR
#define FAILED gcc_unreachable ()
#define MAYBE_OK(insn) if (insn_ok_now (insn)) return;
#endif

/* Registers into which we move the contents of virtual registers.  */
#define X gen_rtx_REG (QImode, 0)
#define A gen_rtx_REG (QImode, 1)
#define C gen_rtx_REG (QImode, 2)
#define B gen_rtx_REG (QImode, 3)
#define E gen_rtx_REG (QImode, 4)
#define D gen_rtx_REG (QImode, 5)
#define L gen_rtx_REG (QImode, 6)
#define H gen_rtx_REG (QImode, 7)

#define AX gen_rtx_REG (HImode, 0)
#define BC gen_rtx_REG (HImode, 2)
#define DE gen_rtx_REG (HImode, 4)
#define HL gen_rtx_REG (HImode, 6)

#define OP(x) (*recog_data.operand_loc[x])

/* Returns TRUE if R is a virtual register.  */
static bool
is_virtual_register (rtx r)
{
  return (GET_CODE (r) == REG
	  && REGNO (r) >= 8
	  && REGNO (r) < 24);
}

/* In all these alloc routines, we expect the following: the insn
   pattern is unshared, the insn was previously recognized and failed
   due to predicates or constraints, and the operand data is in
   recog_data.  */

static int virt_insn_was_frame;

/* Hook for all insns we emit.  Re-mark them as FRAME_RELATED if
   needed.  */
static rtx
EM2 (int line ATTRIBUTE_UNUSED, rtx r)
{
#if DEBUG_ALLOC
  fprintf (stderr, "\033[36m%d: ", line);
  debug_rtx(r);
  fprintf (stderr, "\033[0m");
#endif
  /*SCHED_GROUP_P (r) = 1;*/
  if (virt_insn_was_frame)
    RTX_FRAME_RELATED_P (r) = 1;
  return r;
}

#define EM(x) EM2 (__LINE__, x)

/* Return a suitable RTX for the low half of a __far address.  */
static rtx
rl78_lo16 (rtx addr)
{
  if (GET_CODE (addr) == SYMBOL_REF
      || GET_CODE (addr) == CONST)
    {
      rtx r = gen_rtx_ZERO_EXTRACT (HImode, addr, GEN_INT (16), GEN_INT (0));
      r = gen_rtx_CONST (HImode, r);
      return r;
    }
  return rl78_subreg (HImode, addr, SImode, 0);
}

/* Return a suitable RTX for the high half's lower byte of a __far address.  */
static rtx
rl78_hi8 (rtx addr)
{
  if (GET_CODE (addr) == SYMBOL_REF
      || GET_CODE (addr) == CONST)
    {
      rtx r = gen_rtx_ZERO_EXTRACT (QImode, addr, GEN_INT (8), GEN_INT (16));
      r = gen_rtx_CONST (QImode, r);
      return r;
    }
  return rl78_subreg (QImode, addr, SImode, 2);
}

/* Copy any register values into real registers and return an RTX for
   the same memory, now addressed by real registers.  Any needed insns
   are emitted before BEFORE.  */
static rtx
transcode_memory_rtx (rtx m, rtx newbase, rtx before)
{
  rtx base, index, addendr;
  int addend = 0;

  if (GET_CODE (m) != MEM)
    return m;

  if (GET_MODE (XEXP (m, 0)) == SImode)
    {
      rtx seg = rl78_hi8 (XEXP (m, 0));
#if DEBUG_ALLOC
      fprintf (stderr, "setting ES:\n");
      debug_rtx(seg);
#endif
      emit_insn_before (EM(gen_movqi (A, seg)), before);
      emit_insn_before (EM(gen_movqi_es (A)), before);
      m = change_address (m, GET_MODE (m), rl78_lo16 (XEXP (m, 0)));
    }

  characterize_address (XEXP (m, 0), &base, &index, &addendr);
  gcc_assert (index == NULL_RTX);

#if DEBUG_ALLOC
  fprintf (stderr, "\033[33m"); debug_rtx(m); fprintf (stderr, "\033[0m");
  debug_rtx (base);
#endif
  if (base == NULL_RTX)
    return m;

  if (addendr && GET_CODE (addendr) == CONST_INT)
    addend = INTVAL (addendr);

  if (REGNO (base) == SP_REG)
    {
      if (addend >= 0 && addend  <= 255)
	return m;
    }

  /* BASE should be a virtual register.  We copy it to NEWBASE.  If
     the addend is out of range for DE/HL, we use AX to compute the full
     address.  */

  if (addend < 0
      || (addend > 255 && REGNO (newbase) != 2)
      || (addendr && GET_CODE (addendr) != CONST_INT))
    {
      /* mov ax, vreg
	 add ax, #imm
	 mov hl, ax	*/
      EM (emit_insn_before (gen_movhi (AX, base), before));
      EM (emit_insn_before (gen_addhi3 (AX, AX, addendr), before));
      EM (emit_insn_before (gen_movhi (newbase, AX), before));
      base = newbase;
      addend = 0;
    }
  else
    {
      EM (emit_insn_before (gen_movhi (newbase, base), before));
      base = newbase;
    }

  if (addend)
    base = gen_rtx_PLUS (HImode, base, GEN_INT (addend));

#if DEBUG_ALLOC
  fprintf (stderr, "\033[33m");
  debug_rtx (m);
#endif
  m = change_address (m, GET_MODE (m), base);
#if DEBUG_ALLOC
  debug_rtx (m);
  fprintf (stderr, "\033[0m");
#endif
  return m;
}

/* Copy SRC to accumulator (A or AX), placing any generated insns
   before BEFORE.  Returns accumulator RTX.  */

static rtx
move_to_acc (int opno, rtx before)
{
  rtx src = OP(opno);
  enum machine_mode mode = GET_MODE (src);

  if (GET_CODE (src) == REG
      && REGNO (src) < 2)
    return src;

  if (mode == VOIDmode)
    mode = recog_data.operand_mode[opno];

  if (mode == QImode)
    {
      EM (emit_insn_before (gen_movqi (A, src), before));
      return A;
    }
  else
    {
      EM (emit_insn_before (gen_movhi (AX, src), before));
      return AX;
    }
}

/* Copy accumulator (A or AX) to DEST, placing any generated insns
   after AFTER.  Returns accumulator RTX.  */

static rtx
move_from_acc (rtx dest, rtx after)
{
  enum machine_mode mode = GET_MODE (dest);

  if (REG_P (dest) && REGNO (dest) < 2)
    return dest;

  if (mode == QImode)
    {
      EM (emit_insn_after (gen_movqi (dest, A), after));
      return A;
    }
  else
    {
      EM (emit_insn_after (gen_movhi (dest, AX), after));
      return AX;
    }
}

/* Copy accumulator (A or AX) to REGNO, placing any generated insns
   before BEFORE.  Returns reg RTX.  */

static rtx
move_acc_to_reg (rtx acc, int regno, rtx before)
{
  enum machine_mode mode = GET_MODE (acc);
  rtx reg;

  reg = gen_rtx_REG (mode, regno);

  if (mode == QImode)
    {
      EM (emit_insn_before (gen_movqi (reg, A), before));
      return reg;
    }
  else
    {
      EM (emit_insn_before (gen_movhi (reg, AX), before));
      return reg;
    }
}

/* Copy SRC to X, placing any generated insns before BEFORE.
   Returns X RTX.  */

static rtx
move_to_x (int opno, rtx before)
{
  rtx src = OP(opno);
  enum machine_mode mode = GET_MODE (src);
  rtx reg;

  if (mode == VOIDmode)
    mode = recog_data.operand_mode[opno];
  reg = (mode == QImode) ? X : AX;

  if (mode == QImode || ! is_virtual_register (OP (opno)))
    {
      OP(opno) = move_to_acc (opno, before);
      OP(opno) = move_acc_to_reg (OP(opno), X_REG, before);
      return reg;
    }

  if (mode == QImode)
    EM (emit_insn_before (gen_movqi (reg, src), before));
  else
    EM (emit_insn_before (gen_movhi (reg, src), before));

  return reg;
}

/* Copy OP(opno) to H or HL, placing any generated insns before BEFORE.
   Returns H/HL RTX.  */

static rtx
move_to_hl (int opno, rtx before)
{
  rtx src = OP (opno);
  enum machine_mode mode = GET_MODE (src);
  rtx reg;

  if (mode == VOIDmode)
    mode = recog_data.operand_mode[opno];
  reg = (mode == QImode) ? L : HL;

  if (mode == QImode || ! is_virtual_register (OP (opno)))
    {
      OP (opno) = move_to_acc (opno, before);
      OP (opno) = move_acc_to_reg (OP (opno), L_REG, before);
      return reg;
    }

  if (mode == QImode)
    EM (emit_insn_before (gen_movqi (reg, src), before));
  else
    EM (emit_insn_before (gen_movhi (reg, src), before));

  return reg;
}

/* Copy OP(opno) to E or DE, placing any generated insns before BEFORE.
   Returns E/DE RTX.  */

static rtx
move_to_de (int opno, rtx before)
{
  rtx src = OP (opno);
  enum machine_mode mode = GET_MODE (src);
  rtx reg;

  if (mode == VOIDmode)
    mode = recog_data.operand_mode[opno];

  reg = (mode == QImode) ? E : DE;

  if (mode == QImode || ! is_virtual_register (OP (opno)))
    {
      OP (opno) = move_to_acc (opno, before);
      OP (opno) = move_acc_to_reg (OP (opno), E_REG, before);
    }
  else
    {
      rtx move = mode == QImode ? gen_movqi (reg, src) : gen_movhi (reg, src);

      EM (emit_insn_before (move, before));
    }

  return reg;
}

/* Devirtualize an insn of the form (SET (op) (unop (op))).  */
static void
rl78_alloc_physical_registers_op1 (rtx insn)
{
  /* op[0] = func op[1] */

  /* We first try using A as the destination, then copying it
     back.  */
  if (rtx_equal_p (OP(0), OP(1)))
    {
      OP(0) =
      OP(1) = transcode_memory_rtx (OP(1), DE, insn);
    }
  else
    {
      OP(0) = transcode_memory_rtx (OP(0), BC, insn);
      OP(1) = transcode_memory_rtx (OP(1), HL, insn);
    }

  MAYBE_OK (insn);

  OP(0) = move_from_acc (OP(0), insn);

  MAYBE_OK (insn);

  /* Try copying the src to acc first, then.  This is for, for
     example, ZERO_EXTEND or NOT.  */
  OP(1) = move_to_acc (1, insn);

  MAYBE_OK (insn);

  FAILED;
}

/* Devirtualize an insn of the form (SET (op) (unop (op) (op))).  */
static void
rl78_alloc_physical_registers_op2 (rtx insn)
{
  /* op[0] = op[1] func op[2] */
  rtx prev = prev_nonnote_nondebug_insn (insn);
  rtx first;
  bool hl_used;

  if (rtx_equal_p (OP(0), OP(1)))
    {
      OP(0) =
      OP(1) = transcode_memory_rtx (OP(1), DE, insn);
      prev = next_nonnote_nondebug_insn (prev);
      OP(2) = transcode_memory_rtx (OP(2), HL, insn);
      prev = prev_nonnote_nondebug_insn (prev);
    }
  else if (rtx_equal_p (OP(0), OP(2)))
    {
      OP(1) = transcode_memory_rtx (OP(1), DE, insn);
      prev = next_nonnote_nondebug_insn (prev);
      OP(0) =
      OP(2) = transcode_memory_rtx (OP(2), HL, insn);
      prev = prev_nonnote_nondebug_insn (prev);
    }
  else
    {
      OP(0) = transcode_memory_rtx (OP(0), BC, insn);
      OP(1) = transcode_memory_rtx (OP(1), DE, insn);
      prev = next_nonnote_nondebug_insn (prev);
      OP(2) = transcode_memory_rtx (OP(2), HL, insn);
    }

  MAYBE_OK (insn);

  prev = prev_nonnote_nondebug_insn (insn);
  if (recog_data.constraints[1][0] == '%'
      && is_virtual_register (OP (1))
      && ! is_virtual_register (OP (2))
      && ! CONSTANT_P (OP (2)))
    {
      rtx tmp = OP (1);
      OP (1) = OP (2);
      OP (2) = tmp;
    }

  /* Make a note of wether (H)L is being used.  It matters
     because if OP(2) alsoneeds reloading, then we must take
     care not to corrupt HL.  */
  hl_used = reg_mentioned_p (L, OP (0)) || reg_mentioned_p (L, OP (1));

  OP(0) = move_from_acc (OP (0), insn);
  OP(1) = move_to_acc (1, insn);

  MAYBE_OK (insn);

  /* We have to copy op2 to HL, but that involves AX, which
     already has a live value.  Emit it before those insns.  */

  if (prev)
    first = next_nonnote_nondebug_insn (prev);
  else
    for (first = insn; prev_nonnote_nondebug_insn (first); first = prev_nonnote_nondebug_insn (first))
      ;

  OP (2) = hl_used ? move_to_de (2, first) : move_to_hl (2, first);
  
  MAYBE_OK (insn);
  
  FAILED;
}

/* Devirtualize an insn of the form (SET () (unop (op))).  */

static void
rl78_alloc_physical_registers_ro1 (rtx insn)
{
  /* (void) op[0] */
  OP(0) = transcode_memory_rtx (OP(0), BC, insn);

  MAYBE_OK (insn);

  OP(0) = move_to_acc (0, insn);

  MAYBE_OK (insn);

  FAILED;
}

/* Devirtualize a compare insn.  */
static void
rl78_alloc_physical_registers_cmp (rtx insn)
{
  /* op[1] cmp_op[0] op[2] */
  rtx prev = prev_nonnote_nondebug_insn (insn);
  rtx first;

  OP(1) = transcode_memory_rtx (OP(1), DE, insn);
  OP(2) = transcode_memory_rtx (OP(2), HL, insn);

  MAYBE_OK (insn);

  OP(1) = move_to_acc (1, insn);

  MAYBE_OK (insn);

  /* We have to copy op2 to HL, but that involves the acc, which
     already has a live value.  Emit it before those insns.  */

  if (prev)
    first = next_nonnote_nondebug_insn (prev);
  else
    for (first = insn; prev_nonnote_nondebug_insn (first); first = prev_nonnote_nondebug_insn (first))
      ;
  OP(2) = move_to_hl (2, first);

  MAYBE_OK (insn);

  FAILED;
}

/* Like op2, but AX = A op X.  */
static void
rl78_alloc_physical_registers_umul (rtx insn)
{
  /* op[0] = op[1] func op[2] */
  rtx prev = prev_nonnote_nondebug_insn (insn);
  rtx first;

  OP(0) = transcode_memory_rtx (OP(0), BC, insn);
  OP(1) = transcode_memory_rtx (OP(1), DE, insn);
  OP(2) = transcode_memory_rtx (OP(2), HL, insn);

  MAYBE_OK (insn);

  if (recog_data.constraints[1][0] == '%'
      && is_virtual_register (OP(1))
      && !is_virtual_register (OP(2))
      && !CONSTANT_P (OP(2)))
    {
      rtx tmp = OP(1);
      OP(1) = OP(2);
      OP(2) = tmp;
    }

  OP(0) = move_from_acc (OP(0), insn);
  OP(1) = move_to_acc (1, insn);

  MAYBE_OK (insn);

  /* We have to copy op2 to X, but that involves the acc, which
     already has a live value.  Emit it before those insns.  */

  if (prev)
    first = next_nonnote_nondebug_insn (prev);
  else
    for (first = insn; prev_nonnote_nondebug_insn (first); first = prev_nonnote_nondebug_insn (first))
      ;
  OP(2) = move_to_x (2, first);

  MAYBE_OK (insn);

  FAILED;
}

/* Scan all insns and devirtualize them.  */
static void
rl78_alloc_physical_registers (void)
{
  /* During most of the compile, gcc is dealing with virtual
     registers.  At this point, we need to assign physical registers
     to the vitual ones, and copy in/out as needed.  */

  rtx insn, curr;
  enum attr_valloc valloc_method;

  for (insn = get_insns (); insn; insn = curr)
    {
      int i;

      curr = next_nonnote_nondebug_insn (insn);

      if (INSN_P (insn)
	  && (GET_CODE (PATTERN (insn)) == SET
	      || GET_CODE (PATTERN (insn)) == CALL)
	  && INSN_CODE (insn) == -1)
	{
	  if (GET_CODE (SET_SRC (PATTERN (insn))) == ASM_OPERANDS)
	    continue;
	  i = recog (PATTERN (insn), insn, 0);
	  if (i == -1)
	    {
	      debug_rtx (insn);
	      gcc_unreachable ();
	    }
	  INSN_CODE (insn) = i;
	}
    }

  cfun->machine->virt_insns_ok = 0;
  cfun->machine->real_insns_ok = 1;

  for (insn = get_insns (); insn; insn = curr)
    {
      curr = insn ? next_nonnote_nondebug_insn (insn) : NULL;

      if (!INSN_P (insn))
	continue;
      if (GET_CODE (PATTERN (insn)) != SET
	  && GET_CODE (PATTERN (insn)) != CALL)
	  continue;

      if (GET_CODE (PATTERN (insn)) == SET
	  && GET_CODE (SET_SRC (PATTERN (insn))) == ASM_OPERANDS)
	continue;

      valloc_method = get_attr_valloc (insn);

      PATTERN (insn)= copy_rtx_if_shared (PATTERN (insn));

      if (insn_ok_now (insn))
	continue;

      INSN_CODE (insn) = -1;

      if (RTX_FRAME_RELATED_P (insn))
	virt_insn_was_frame = 1;
      else
	virt_insn_was_frame = 0;

      switch (valloc_method)
	{
	case VALLOC_OP1:
	  rl78_alloc_physical_registers_op1 (insn);
	  break;
	case VALLOC_OP2:
	  rl78_alloc_physical_registers_op2 (insn);
	  break;
	case VALLOC_RO1:
	  rl78_alloc_physical_registers_ro1 (insn);
	  break;
	case VALLOC_CMP:
	  rl78_alloc_physical_registers_cmp (insn);
	  break;
	case VALLOC_UMUL:
	  rl78_alloc_physical_registers_umul (insn);
	  break;
	case VALLOC_MACAX:
	  /* Macro that clobbers AX */
	  break;
	}
    }
#if DEBUG_ALLOC
  fprintf (stderr, "\033[0m");
#endif
}

/* Add REG_DEAD notes using DEAD[reg] for rtx S which is part of INSN.
   This function scans for uses of registers; the last use (i.e. first
   encounter when scanning backwards) triggers a REG_DEAD note if the
   reg was previously in DEAD[].  */
static void
rl78_note_reg_uses (char *dead, rtx s, rtx insn)
{
  const char *fmt;
  int i, r;
  enum rtx_code code;

  if (!s)
    return;

  code = GET_CODE (s);

  switch (code)
    {
      /* Compare registers by number.  */
    case REG:
      r = REGNO (s);
      if (dump_file)
	{
	  fprintf (dump_file, "note use reg %d size %d on insn %d\n",
		   r, GET_MODE_SIZE (GET_MODE (s)), INSN_UID (insn));
	  print_rtl_single (dump_file, s);
	}
      if (dead [r])
	add_reg_note (insn, REG_DEAD, gen_rtx_REG (GET_MODE (s), r));
      for (i = 0; i < GET_MODE_SIZE (GET_MODE (s)); i ++)
	dead [r + i] = 0;
      return;

      /* These codes have no constituent expressions
	 and are unique.  */
    case SCRATCH:
    case CC0:
    case PC:
      return;

    case CONST_INT:
    case CONST_VECTOR:
    case CONST_DOUBLE:
    case CONST_FIXED:
      /* These are kept unique for a given value.  */
      return;

    default:
      break;
    }

  fmt = GET_RTX_FORMAT (code);

  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'E')
	{
	  int j;
	  for (j = XVECLEN (s, i) - 1; j >= 0; j--)
	    rl78_note_reg_uses (dead, XVECEXP (s, i, j), insn);
	}
      else if (fmt[i] == 'e')
	rl78_note_reg_uses (dead, XEXP (s, i), insn);
    }
}

/* Like the previous function, but scan for SETs instead.  */
static void
rl78_note_reg_set (char *dead, rtx d, rtx insn)
{
  int r, i;

  if (GET_CODE (d) != REG)
    return;

  r = REGNO (d);
  if (dead [r])
    add_reg_note (insn, REG_UNUSED, gen_rtx_REG (GET_MODE (d), r));
  if (dump_file)
    fprintf (dump_file, "note set reg %d size %d\n", r, GET_MODE_SIZE (GET_MODE (d)));
  for (i = 0; i < GET_MODE_SIZE (GET_MODE (d)); i ++)
    dead [r + i] = 1;
}

/* This is a rather crude register death pass.  Death status is reset
   at every jump or call insn.  */
static void
rl78_calculate_death_notes (void)
{
  char dead[FIRST_PSEUDO_REGISTER];
  rtx insn, p, s, d;
  int i;

  memset (dead, 0, sizeof (dead));

  for (insn = get_last_insn ();
       insn;
       insn = prev_nonnote_nondebug_insn (insn))
    {
      if (dump_file)
	{
	  fprintf (dump_file, "\n--------------------------------------------------");
	  fprintf (dump_file, "\nDead:");
	  for (i = 0; i < FIRST_PSEUDO_REGISTER; i ++)
	    if (dead[i])
	      fprintf(dump_file, " %s", reg_names[i]);
	  fprintf (dump_file, "\n");
	  print_rtl_single (dump_file, insn);
	}

      switch (GET_CODE (insn))
	{
	case INSN:
	  p = PATTERN (insn);
	  switch (GET_CODE (p))
	    {
	    case SET:
	      s = SET_SRC (p);
	      d = SET_DEST (p);
	      rl78_note_reg_set (dead, d, insn);
	      rl78_note_reg_uses (dead, s, insn);
	      break;

	    case USE:
	      rl78_note_reg_uses (dead, p, insn);
	      break;

	    default:
	      break;
	    }
	  break;

	case JUMP_INSN:
	  if (INSN_CODE (insn) == CODE_FOR_rl78_return)
	    {
	      memset (dead, 1, sizeof (dead));
	      /* We expect a USE just prior to this, which will mark
		 the actual return registers.  The USE will have a
		 death note, but we aren't going to be modifying it
		 after this pass.  */
	      break;
	    }
	case CALL_INSN:
	  memset (dead, 0, sizeof (dead));
	  break;

	default:
	  break;
	}
      if (dump_file)
	print_rtl_single (dump_file, insn);
    }
}

/* Helper function to reset the origins in RP and the age in AGE for
   all registers.  */
static void
reset_origins (int *rp, int *age)
{
  int i;
  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    {
      rp[i] = i;
      age[i] = 0;
    }
}

/* The idea behind this optimization is to look for cases where we
   move data from A to B to C, and instead move from A to B, and A to
   C.  If B is a virtual register or memory, this is a big win on its
   own.  If B turns out to be unneeded after this, it's a bigger win.
   For each register, we try to determine where it's value originally
   came from, if it's propogated purely through moves (and not
   computes).  The ORIGINS[] array has the regno for the "origin" of
   the value in the [regno] it's indexed by.  */
static void
rl78_propogate_register_origins (void)
{
  int origins[FIRST_PSEUDO_REGISTER];
  int age[FIRST_PSEUDO_REGISTER];
  int i;
  rtx insn, ninsn = NULL_RTX;
  rtx pat;

  reset_origins (origins, age);

  for (insn = get_insns (); insn; insn = ninsn)
    {
      ninsn = next_nonnote_nondebug_insn (insn);

      if (dump_file)
	{
	  fprintf (dump_file, "\n");
	  fprintf (dump_file, "Origins:");
	  for (i = 0; i < FIRST_PSEUDO_REGISTER; i ++)
	    if (origins[i] != i)
	      fprintf (dump_file, " r%d=r%d", i, origins[i]);
	  fprintf (dump_file, "\n");
	  print_rtl_single (dump_file, insn);
	}

      switch (GET_CODE (insn))
	{
	case CODE_LABEL:
	case BARRIER:
	case CALL_INSN:
	case JUMP_INSN:
	  reset_origins (origins, age);
	  break;

	default:
	  break;

	case INSN:
	  pat = PATTERN (insn);

	  if (GET_CODE (pat) == PARALLEL)
	    {
	      rtx clobber = XVECEXP (pat, 0, 1);
	      pat = XVECEXP (pat, 0, 0);
	      if (GET_CODE (clobber) == CLOBBER)
		{
		  int cr = REGNO (XEXP (clobber, 0));
		  int mb = GET_MODE_SIZE (GET_MODE (XEXP (clobber, 0)));
		  if (dump_file)
		    fprintf (dump_file, "reset origins of %d regs at %d\n", mb, cr);
		  for (i = 0; i < mb; i++)
		    {
		      origins[cr + i] = cr + i;
		      age[cr + i] = 0;
		    }
		}
	      else
		break;
	    }

	  if (GET_CODE (pat) == SET)
	    {
	      rtx src = SET_SRC (pat);
	      rtx dest = SET_DEST (pat);
	      int mb = GET_MODE_SIZE (GET_MODE (dest));

	      if (GET_CODE (dest) == REG)
		{
		  int dr = REGNO (dest);

		  if (GET_CODE (src) == REG)
		    {
		      int sr = REGNO (src);
		      int same = 1;
		      int best_age, best_reg;

		      /* See if the copy is not needed.  */
		      for (i = 0; i < mb; i ++)
			if (origins[dr + i] != origins[sr + i])
			  same = 0;
		      if (same)
			{
			  if (dump_file)
			    fprintf (dump_file, "deleting because dest already has correct value\n");
			  delete_insn (insn);
			  break;
			}

		      if (dr < 8 || sr >= 8)
			{
			  int ar;

			  best_age = -1;
			  best_reg = -1;
			  /* See if the copy can be made from another
			     bank 0 register instead, instead of the
			     virtual src register.  */
			  for (ar = 0; ar < 8; ar += mb)
			    {
			      same = 1;
			      for (i = 0; i < mb; i ++)
				if (origins[ar + i] != origins[sr + i])
				  same = 0;

			      /* The chip has some reg-reg move limitations.  */
			      if (mb == 1 && dr > 3)
				same = 0;

			      if (same)
				{
				  if (best_age == -1 || best_age > age[sr + i])
				    {
				      best_age = age[sr + i];
				      best_reg = sr;
				    }
				}
			    }

			  if (best_reg != -1)
			    {
			      /* FIXME: copy debug info too.  */
			      SET_SRC (pat) = gen_rtx_REG (GET_MODE (src), best_reg);
			      sr = best_reg;
			    }
			}

		      for (i = 0; i < mb; i++)
			{
			  origins[dr + i] = origins[sr + i];
			  age[dr + i] = age[sr + i] + 1;
			}
		    }
		  else
		    {
		      /* The destination is computed, its origin is itself.  */
		      if (dump_file)
			fprintf (dump_file, "resetting origin of r%d for %d byte%s\n",
				 dr, mb, mb == 1 ? "" : "s");
		      for (i = 0; i < mb; i ++)
			{
			  origins[dr + i] = dr + i;
			  age[dr + i] = 0;
			}
		    }

		  /* Any registers marked with that reg as an origin are reset.  */
		  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
		    if (origins[i] >= dr && origins[i] < dr + mb)
		      {
			origins[i] = i;
			age[i] = 0;
		      }
		}

	      /* Special case - our ADDSI3 macro uses AX */
	      if (get_attr_valloc (insn) == VALLOC_MACAX)
		{
		  if (dump_file)
		    fprintf (dump_file, "Resetting origin of AX for macro.\n");
		  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
		    if (i <= 1 || origins[i] <= 1)
		      {
			origins[i] = i;
			age[i] = 0;
		      }
		}

	      if (GET_CODE (src) == ASHIFT
		  || GET_CODE (src) == ASHIFTRT
		  || GET_CODE (src) == LSHIFTRT)
		{
		  rtx count = XEXP (src, 1);
		  if (GET_CODE (count) == REG)
		    {
		      /* Special case - our pattern clobbers the count register.  */
		      int r = REGNO (count);
		      if (dump_file)
			fprintf (dump_file, "Resetting origin of r%d for shift.\n", r);
		      for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
			if (i == r || origins[i] == r)
			  {
			    origins[i] = i;
			    age[i] = 0;
			  }
		    }
		}
	    }
	}
    }
}

/* Remove any SETs where the destination is unneeded.  */
static void
rl78_remove_unused_sets (void)
{
  rtx insn, ninsn = NULL_RTX;
  rtx dest;

  for (insn = get_insns (); insn; insn = ninsn)
    {
      ninsn = next_nonnote_nondebug_insn (insn);

      if ((insn = single_set (insn)) == NULL_RTX)
	continue;

      dest = SET_DEST (insn);

      if (GET_CODE (dest) != REG || REGNO (dest) > 23)
	continue;

      if (find_regno_note (insn, REG_UNUSED, REGNO (dest)))
	delete_insn (insn);
    }
}

#undef  xTARGET_MACHINE_DEPENDENT_REORG
#define xTARGET_MACHINE_DEPENDENT_REORG  rl78_reorg

/* This is the top of the devritualization pass.  */
static void
rl78_reorg (void)
{
  rl78_alloc_physical_registers ();

  if (dump_file)
    {
      fprintf (dump_file, "\n================DEVIRT:=AFTER=ALLOC=PHYSICAL=REGISTERS================\n");
      print_rtl_with_bb (dump_file, get_insns (), 0);
    }

  rl78_propogate_register_origins ();
  rl78_calculate_death_notes ();

  if (dump_file)
    {
      fprintf (dump_file, "\n================DEVIRT:=AFTER=PROPOGATION=============================\n");
      print_rtl_with_bb (dump_file, get_insns (), 0);
      fprintf (dump_file, "\n======================================================================\n");
    }

  rl78_remove_unused_sets ();

  /* The code after devirtualizing has changed so much that at this point
     we might as well just rescan everything.  Note that
     df_rescan_all_insns is not going to help here because it does not
     touch the artificial uses and defs.  */
  df_finish_pass (true);
  if (optimize > 1)
    df_live_add_problem ();
  df_scan_alloc (NULL);
  df_scan_blocks ();

  if (optimize)
    df_analyze ();
}

#undef TARGET_RETURN_IN_MEMORY
#define TARGET_RETURN_IN_MEMORY rl78_return_in_memory

static bool
rl78_return_in_memory (const_tree type, const_tree fntype ATTRIBUTE_UNUSED)
{
  const HOST_WIDE_INT size = int_size_in_bytes (type);
  return (size == -1 || size > 8);
}


struct gcc_target targetm = TARGET_INITIALIZER;

#include "gt-rl78.h"
