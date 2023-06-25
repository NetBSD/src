/* Generated automatically by the program `genoutput'
   from the machine description file `md'.  */

#define IN_TARGET_CODE 1
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "predict.h"
#include "tree.h"
#include "rtl.h"
#include "flags.h"
#include "alias.h"
#include "varasm.h"
#include "stor-layout.h"
#include "calls.h"
#include "insn-config.h"
#include "expmed.h"
#include "dojump.h"
#include "explow.h"
#include "memmodel.h"
#include "emit-rtl.h"
#include "stmt.h"
#include "expr.h"
#include "insn-codes.h"
#include "tm_p.h"
#include "regs.h"
#include "conditions.h"
#include "insn-attr.h"

#include "recog.h"

#include "diagnostic-core.h"
#include "output.h"
#include "target.h"
#include "tm-constrs.h"

static const char * const output_1[] = {
  "fmove%.d %f1,%0",
  "#",
};

static const char *
output_3 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 328 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = m68k_find_flags_value (operands[0], const0_rtx, EQ);
  if (code == EQ)
    return "jeq %l1";
  if (which_alternative == 2)
    return "move%.l %0,%2\n\tor%.l %0,%2\n\tjeq %l1";
  if (GET_CODE (operands[0]) == REG)
    operands[3] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  else
    operands[3] = adjust_address (operands[0], SImode, 4);
  if (! ADDRESS_REG_P (operands[0]))
    {
      if (reg_overlap_mentioned_p (operands[2], operands[0]))
	{
	  if (reg_overlap_mentioned_p (operands[2], operands[3]))
	    return "or%.l %0,%2\n\tjeq %l1";
	  else
	    return "or%.l %3,%2\n\tjeq %l1";
	}
      return "move%.l %0,%2\n\tor%.l %3,%2\n\tjeq %l1";
    }
  operands[4] = gen_label_rtx();
  if (TARGET_68020 || TARGET_COLDFIRE)
    output_asm_insn ("tst%.l %0\n\tjne %l4\n\ttst%.l %3\n\tjeq %l1", operands);
  else
    output_asm_insn ("cmp%.w #0,%0\n\tjne %l4\n\tcmp%.w #0,%3\n\tjeq %l1", operands);
  (*targetm.asm_out.internal_label) (asm_out_file, "L",
				CODE_LABEL_NUMBER (operands[4]));
  return "";
}
}

static const char *
output_4 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 367 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = m68k_find_flags_value (operands[0], const0_rtx, NE);
  if (code == NE)
    return "jne %l1";
  if (GET_CODE (operands[0]) == REG)
    operands[3] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  else
    operands[3] = adjust_address (operands[0], SImode, 4);
  if (!ADDRESS_REG_P (operands[0]))
    {
      if (reg_overlap_mentioned_p (operands[2], operands[0]))
	{
	  if (reg_overlap_mentioned_p (operands[2], operands[3]))
	    return "or%.l %0,%2\n\tjne %l1";
	  else
	    return "or%.l %3,%2\n\tjne %l1";
	}
      return "move%.l %0,%2\n\tor%.l %3,%2\n\tjne %l1";
    }
  if (TARGET_68020 || TARGET_COLDFIRE)
    return "tst%.l %0\n\tjne %l1\n\ttst%.l %3\n\tjne %l1";
  else
    return "cmp%.w #0,%0\n\tjne %l1\n\tcmp%.w #0,%3\n\tjne %l1";
}
}

static const char *
output_5 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 402 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_di (operands[2], operands[3], operands[5], operands[0], insn, code);
  operands[3] = operands[4];
  return m68k_output_branch_integer (code);
}
}

static const char *
output_6 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 511 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_qi (operands[1], operands[2], code);
  return m68k_output_branch_integer (code);
}
}

static const char *
output_7 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 511 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_hi (operands[1], operands[2], code);
  return m68k_output_branch_integer (code);
}
}

static const char *
output_8 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 511 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_si (operands[1], operands[2], code);
  return m68k_output_branch_integer (code);
}
}

static const char *
output_9 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 526 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_qi (operands[1], operands[2], code);
  return m68k_output_branch_integer_rev (code);
}
}

static const char *
output_10 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 526 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_hi (operands[1], operands[2], code);
  return m68k_output_branch_integer_rev (code);
}
}

static const char *
output_11 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 526 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_si (operands[1], operands[2], code);
  return m68k_output_branch_integer_rev (code);
}
}

static const char *
output_12 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 541 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_qi (operands[1], operands[2], code);
  return m68k_output_branch_integer (code);
}
}

static const char *
output_13 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 541 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_hi (operands[1], operands[2], code);
  return m68k_output_branch_integer (code);
}
}

static const char *
output_14 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 541 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_si (operands[1], operands[2], code);
  return m68k_output_branch_integer (code);
}
}

static const char *
output_15 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 556 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_qi (operands[1], operands[2], code);
  return m68k_output_branch_integer_rev (code);
}
}

static const char *
output_16 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 556 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_hi (operands[1], operands[2], code);
  return m68k_output_branch_integer_rev (code);
}
}

static const char *
output_17 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 556 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_si (operands[1], operands[2], code);
  return m68k_output_branch_integer_rev (code);
}
}

static const char *
output_18 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 569 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_qi (operands[2], operands[3], code);
  return m68k_output_scc (code);
}
}

static const char *
output_19 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 569 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_hi (operands[2], operands[3], code);
  return m68k_output_scc (code);
}
}

static const char *
output_20 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 569 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_si (operands[2], operands[3], code);
  return m68k_output_scc (code);
}
}

static const char *
output_21 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 582 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_qi (operands[2], operands[3], code);
  return m68k_output_scc (code);
}
}

static const char *
output_22 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 582 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_hi (operands[2], operands[3], code);
  return m68k_output_scc (code);
}
}

static const char *
output_23 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 582 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_si (operands[2], operands[3], code);
  return m68k_output_scc (code);
}
}

static const char *
output_24 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 604 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_btst (operands[2], operands[1], code, 7);
  return m68k_output_branch_integer (code);
}
}

static const char *
output_25 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 622 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_btst (operands[2], operands[1], code, 31);
  return m68k_output_branch_integer (code);
}
}

static const char *
output_26 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 640 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  operands[2] = GEN_INT (7 - INTVAL (operands[2]));
  code = m68k_output_btst (operands[2], operands[1], code, 7);
  return m68k_output_branch_integer (code);
}
}

static const char *
output_27 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 657 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  if (GET_CODE (operands[1]) == MEM)
    {
      operands[1] = adjust_address (operands[1], QImode,
				    INTVAL (operands[2]) / 8);
      operands[2] = GEN_INT (7 - INTVAL (operands[2]) % 8);
      code = m68k_output_btst (operands[2], operands[1], code, 7);
    }
  else
    {
      operands[2] = GEN_INT (31 - INTVAL (operands[2]));
      code = m68k_output_btst (operands[2], operands[1], code, 31);
    }
  return m68k_output_branch_integer (code);
}
}

static const char *
output_28 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 693 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_bftst (operands[1], operands[2], operands[3], code);
  operands[3] = operands[4];
  return m68k_output_branch_integer (code);
}
}

static const char *
output_29 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 693 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_bftst (operands[1], operands[2], operands[3], code);
  operands[3] = operands[4];
  return m68k_output_branch_integer (code);
}
}

static const char *
output_30 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 710 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_bftst (operands[2], operands[3], operands[4], code);
  return m68k_output_scc (code);
}
}

static const char *
output_31 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 710 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_bftst (operands[2], operands[3], operands[4], code);
  return m68k_output_scc (code);
}
}

static const char *
output_32 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 749 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float (code);
}
}

static const char *
output_33 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 749 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float (code);
}
}

static const char *
output_34 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 749 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float (code);
}
}

static const char *
output_35 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 767 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float (code);
}
}

static const char *
output_36 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 767 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float (code);
}
}

static const char *
output_37 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 767 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float (code);
}
}

static const char *
output_38 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 785 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float_rev (code);
}
}

static const char *
output_39 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 785 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float_rev (code);
}
}

static const char *
output_40 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 785 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float_rev (code);
}
}

static const char *
output_41 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 803 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float_rev (code);
}
}

static const char *
output_42 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 803 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float_rev (code);
}
}

static const char *
output_43 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 803 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_fp (operands[1], operands[2], code);
  return m68k_output_branch_float_rev (code);
}
}

static const char *
output_44 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 819 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_fp (operands[2], operands[3], code);
  return m68k_output_scc_float (code);
}
}

static const char *
output_45 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 819 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_fp (operands[2], operands[3], code);
  return m68k_output_scc_float (code);
}
}

static const char *
output_46 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 819 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_fp (operands[2], operands[3], code);
  return m68k_output_scc_float (code);
}
}

static const char *
output_47 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 833 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_fp (operands[2], operands[3], code);
  return m68k_output_scc_float (code);
}
}

static const char *
output_48 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 833 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  code = m68k_output_compare_fp (operands[2], operands[3], code);
  return m68k_output_scc_float (code);
}
}

static const char * const output_49[] = {
  "clr%.l %0",
  "mov3q%.l %1,%-",
  "pea %a1",
};

static const char * const output_50[] = {
  "moveq #0,%0",
  "sub%.l %0,%0",
  "clr%.l %0",
};

static const char *
output_51 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 881 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (which_alternative == 0)
    return MOTOROLA ? "lea 0.w,%0" : "lea 0:w,%0";
  else if (which_alternative == 1)
    return "clr%.l %0";
  else
    {
      gcc_unreachable ();
      return "";
    }
}
}

static const char * const output_52[] = {
  "sub%.l %0,%0",
  "clr%.l %0",
};

static const char *
output_53 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 996 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_move_simode (operands);
}
}

static const char *
output_54 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1009 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_move_simode (operands);
}
}

static const char *
output_55 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1019 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  switch (which_alternative)
    {
    case 0:
      return "mov3q%.l %1,%0";

    case 1:
      return "moveq %1,%0";

    case 2:
      {
	unsigned u = INTVAL (operands[1]);

	operands[1] = GEN_INT ((u << 16) | (u >> 16));  /*|*/
	return "moveq %1,%0\n\tswap %0";
      }

    case 3:
      return "mvz%.w %1,%0";

    case 4:
      return "mvs%.w %1,%0";

    case 5:
      return "move%.l %1,%0";

    case 6:
      return "move%.w %1,%0";

    case 7:
      return "pea %a1";

    case 8:
      return "lea %a1,%0";

    case 9:
    case 10:
    case 11:
      return "move%.l %1,%0";

    default:
      gcc_unreachable ();
      return "";
    }
}
}

static const char *
output_56 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1072 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (push_operand (operands[0], SImode))
    return "pea %a1";
  return "lea %a1,%0";
}
}

static const char *
output_57 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1088 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_move_himode (operands);
}

static const char *
output_58 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1098 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_move_himode (operands);
}

static const char *
output_59 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1114 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_move_stricthi (operands);
}

static const char *
output_60 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1121 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_move_stricthi (operands);
}

static const char *
output_61 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1134 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_move_qimode (operands);
}

static const char *
output_62 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1141 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_move_qimode (operands);
}

static const char *
output_63 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1154 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_move_strictqi (operands);
}

static const char * const output_64[] = {
  "clr%.b %0",
  "clr%.b %0",
  "move%.b %1,%0",
  "move%.b %1,%0",
};

static const char *
output_65 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1213 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "f%$move%.x %1,%0";
      else if (ADDRESS_REG_P (operands[1]))
	return "move%.l %1,%-\n\tf%$move%.s %+,%0";
      else if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_single (operands);
      return "f%$move%.s %f1,%0";
    }
  if (FP_REG_P (operands[1]))
    {
      if (ADDRESS_REG_P (operands[0]))
	return "fmove%.s %1,%-\n\tmove%.l %+,%0";
      return "fmove%.s %f1,%0";
    }
  if (operands[1] == CONST0_RTX (SFmode)
      /* clr insns on 68000 read before writing.  */
      && ((TARGET_68010 || TARGET_COLDFIRE)
	  || !(GET_CODE (operands[0]) == MEM && MEM_VOLATILE_P (operands[0]))))
    {
      if (ADDRESS_REG_P (operands[0]))
	{
	  /* On the '040, 'subl an,an' takes 2 clocks while lea takes only 1 */
	  if (TUNE_68040_60)
	    return MOTOROLA ? "lea 0.w,%0" : "lea 0:w,%0";
	  else
	    return "sub%.l %0,%0";
	}
      /* moveq is faster on the 68000.  */
      if (DATA_REG_P (operands[0]) && TUNE_68000_10)
	return "moveq #0,%0";
      return "clr%.l %0";
    }
  return "move%.l %1,%0";
}
}

static const char *
output_67 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1267 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (which_alternative == 4 || which_alternative == 5) {
    rtx xoperands[2];
    long l;
    REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (operands[1]), l);
    xoperands[0] = operands[0];
    xoperands[1] = GEN_INT (l);
    if (which_alternative == 5) {
      if (l == 0) {
        if (ADDRESS_REG_P (xoperands[0]))
          output_asm_insn ("sub%.l %0,%0", xoperands);
        else
          output_asm_insn ("clr%.l %0", xoperands);
      } else
        if (GET_CODE (operands[0]) == MEM
            && symbolic_operand (XEXP (operands[0], 0), SImode))
          output_asm_insn ("move%.l %1,%-;move%.l %+,%0", xoperands);
        else
          output_asm_insn ("move%.l %1,%0", xoperands);
      return "";
    }
    if (l != 0)
      output_asm_insn ("move%.l %1,%-;fsmove%.s %+,%0", xoperands);
    else
      output_asm_insn ("clr%.l %-;fsmove%.s %+,%0", xoperands);
    return "";
  }
  if (FP_REG_P (operands[0]))
    {
      if (ADDRESS_REG_P (operands[1]))
        return "move%.l %1,%-;fsmove%.s %+,%0";
      if (FP_REG_P (operands[1]))
        return "fsmove%.d %1,%0";
      return "fsmove%.s %f1,%0";
    }
  if (FP_REG_P (operands[1]))
    {
      if (ADDRESS_REG_P (operands[0]))
        return "fmove%.s %1,%-;move%.l %+,%0";
      return "fmove%.s %f1,%0";
    }
  if (operands[1] == CONST0_RTX (SFmode))
    {
      if (ADDRESS_REG_P (operands[0]))
	return "sub%.l %0,%0";
      return "clr%.l %0";
    }
  return "move%.l %1,%0";
}
}

static const char *
output_68 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1361 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "f%&move%.x %1,%0";
      if (REG_P (operands[1]))
	{
	  rtx xoperands[2];
	  xoperands[1] = gen_rtx_REG (SImode, REGNO (operands[1]) + 1);
	  output_asm_insn ("move%.l %1,%-", xoperands);
	  output_asm_insn ("move%.l %1,%-", operands);
	  return "f%&move%.d %+,%0";
	}
      if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_double (operands);
      return "f%&move%.d %f1,%0";
    }
  else if (FP_REG_P (operands[1]))
    {
      if (REG_P (operands[0]))
	{
	  output_asm_insn ("fmove%.d %f1,%-\n\tmove%.l %+,%0", operands);
	  operands[0] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
	  return "move%.l %+,%0";
	}
      else
        return "fmove%.d %f1,%0";
    }
  return output_move_double (operands);
}
}

static const char *
output_70 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1409 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx xoperands[3];
  long l[2];

  switch (which_alternative)
    {
    default:
      return "fdmove%.d %1,%0";
    case 1:
      return "fmove%.d %1,%0";
    case 2:
      return "fmove%.d %1,%-;move%.l %+,%0;move%.l %+,%R0";
    case 3:
      return "move%.l %R1,%-;move%.l %1,%-;fdmove%.d %+,%0";
    case 4: case 5: case 6:
      return output_move_double (operands);
    case 7:
      REAL_VALUE_TO_TARGET_DOUBLE (*CONST_DOUBLE_REAL_VALUE (operands[1]), l);
      xoperands[0] = operands[0];
      xoperands[1] = GEN_INT (l[0]);
      xoperands[2] = GEN_INT (l[1]);
      if (operands[1] == CONST0_RTX (DFmode))
	output_asm_insn ("clr%.l %-;clr%.l %-;fdmove%.d %+,%0",
			xoperands);
      else
	if (l[1] == 0)
	  output_asm_insn ("clr%.l %-;move%.l %1,%-;fdmove%.d %+,%0",
			  xoperands);
	else
	  output_asm_insn ("move%.l %2,%-;move%.l %1,%-;fdmove%.d %+,%0",
			  xoperands);
      return "";
    }
}
}

static const char *
output_71 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1479 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "fmove%.x %1,%0";
      if (REG_P (operands[1]))
	{
	  rtx xoperands[2];
	  xoperands[1] = gen_rtx_REG (SImode, REGNO (operands[1]) + 2);
	  output_asm_insn ("move%.l %1,%-", xoperands);
	  xoperands[1] = gen_rtx_REG (SImode, REGNO (operands[1]) + 1);
	  output_asm_insn ("move%.l %1,%-", xoperands);
	  output_asm_insn ("move%.l %1,%-", operands);
	  return "fmove%.x %+,%0";
	}
      if (GET_CODE (operands[1]) == CONST_DOUBLE)
        return "fmove%.x %1,%0";
      return "fmove%.x %f1,%0";
    }
  if (FP_REG_P (operands[1]))
    {
      if (REG_P (operands[0]))
	{
	  output_asm_insn ("fmove%.x %f1,%-\n\tmove%.l %+,%0", operands);
	  operands[0] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
	  output_asm_insn ("move%.l %+,%0", operands);
	  operands[0] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
	  return "move%.l %+,%0";
	}
      /* Must be memory destination.  */
      return "fmove%.x %f1,%0";
    }
  return output_move_double (operands);
}
}

static const char *
output_72 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1519 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "fmove%.x %1,%0";
      if (REG_P (operands[1]))
	{
	  rtx xoperands[2];
	  xoperands[1] = gen_rtx_REG (SImode, REGNO (operands[1]) + 2);
	  output_asm_insn ("move%.l %1,%-", xoperands);
	  xoperands[1] = gen_rtx_REG (SImode, REGNO (operands[1]) + 1);
	  output_asm_insn ("move%.l %1,%-", xoperands);
	  output_asm_insn ("move%.l %1,%-", operands);
	  return "fmove%.x %+,%0";
	}
      if (GET_CODE (operands[1]) == CONST_DOUBLE)
        return "fmove%.x %1,%0";
      return "fmove%.x %f1,%0";
    }
  if (FP_REG_P (operands[1]))
    {
      if (REG_P (operands[0]))
        {
          output_asm_insn ("fmove%.x %f1,%-\n\tmove%.l %+,%0", operands);
          operands[0] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
          output_asm_insn ("move%.l %+,%0", operands);
          operands[0] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
          return "move%.l %+,%0";
        }
      else
        return "fmove%.x %f1,%0";
    }
  return output_move_double (operands);
}
}

static const char *
output_73 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1558 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_move_double (operands);
}

static const char *
output_74 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1577 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "fmove%.x %1,%0";
      if (REG_P (operands[1]))
	{
	  rtx xoperands[2];
	  xoperands[1] = gen_rtx_REG (SImode, REGNO (operands[1]) + 1);
	  output_asm_insn ("move%.l %1,%-", xoperands);
	  output_asm_insn ("move%.l %1,%-", operands);
	  return "fmove%.d %+,%0";
	}
      if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_double (operands);
      return "fmove%.d %f1,%0";
    }
  else if (FP_REG_P (operands[1]))
    {
      if (REG_P (operands[0]))
	{
	  output_asm_insn ("fmove%.d %f1,%-\n\tmove%.l %+,%0", operands);
	  operands[0] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
	  return "move%.l %+,%0";
	}
      else
        return "fmove%.d %f1,%0";
    }
  return output_move_double (operands);
}
}

static const char *
output_75 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1612 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_move_double (operands);
}

static const char *
output_77 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1632 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[0]) == REG)
    return "move%.l %1,%0";

  if (GET_CODE (operands[1]) == MEM)
    operands[1] = adjust_address (operands[1], QImode, 3);
  return "move%.b %1,%0";
}
}

static const char *
output_78 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1650 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[0]) == REG
      && (GET_CODE (operands[1]) == MEM
	  || GET_CODE (operands[1]) == CONST_INT))
    return "move%.w %1,%0";

  if (GET_CODE (operands[0]) == REG)
    return "move%.l %1,%0";

  if (GET_CODE (operands[1]) == MEM)
    operands[1] = adjust_address (operands[1], QImode, 1);
  return "move%.b %1,%0";
}
}

static const char *
output_79 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1673 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[0]) == REG)
    return "move%.l %1,%0";

  if (GET_CODE (operands[1]) == MEM)
    operands[1] = adjust_address (operands[1], QImode, 2);
  return "move%.w %1,%0";
}
}

static const char *
output_90 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1851 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  if (ISA_HAS_MVS_MVZ)
    return "mvs%.b %1,%2\n\tsmi %0\n\textb%.l %0";
  if (TARGET_68020 || TARGET_COLDFIRE)
    {
      if (ADDRESS_REG_P (operands[1]))
	return "move%.w %1,%2\n\textb%.l %2\n\tsmi %0\n\textb%.l %0";
      else
	return "move%.b %1,%2\n\textb%.l %2\n\tsmi %0\n\textb%.l %0";
    }
  else
    {
      if (ADDRESS_REG_P (operands[1]))
	return "move%.w %1,%2\n\text%.w %2\n\text%.l %2\n\tmove%.l %2,%0\n\tsmi %0";
      else
	return "move%.b %1,%2\n\text%.w %2\n\text%.l %2\n\tmove%.l %2,%0\n\tsmi %0";
    }
}
}

static const char *
output_91 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1876 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  if (ISA_HAS_MVS_MVZ)
    return "mvs%.w %1,%2\n\tsmi %0\n\textb%.l %0";
  if (TARGET_68020 || TARGET_COLDFIRE)
    return "move%.w %1,%2\n\text%.l %2\n\tsmi %0\n\textb%.l %0";
  else
    return "move%.w %1,%2\n\text%.l %2\n\tsmi %0\n\text%.w %0\n\text%.l %0";
}
}

static const char *
output_92 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1892 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (which_alternative == 0)
    /* Handle alternative 0.  */
    {
      if (TARGET_68020 || TARGET_COLDFIRE)
        return "move%.l %1,%R0\n\tsmi %0\n\textb%.l %0";
      else
        return "move%.l %1,%R0\n\tsmi %0\n\text%.w %0\n\text%.l %0";
    }

  /* Handle alternatives 1, 2 and 3.  We don't need to adjust address by 4
     in alternative 3 because autodecrement will do that for us.  */
  operands[3] = adjust_address (operands[0], SImode,
				which_alternative == 3 ? 0 : 4);
  operands[0] = adjust_address (operands[0], SImode, 0);

  if (TARGET_68020 || TARGET_COLDFIRE)
    return "move%.l %1,%3\n\tsmi %2\n\textb%.l %2\n\tmove%.l %2,%0";
  else
    return "move%.l %1,%3\n\tsmi %2\n\text%.w %2\n\text%.l %2\n\tmove%.l %2,%0";
}
}

static const char *
output_93 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 1926 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[3] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  if (GET_CODE (operands[1]) == CONST_INT
  && (unsigned) INTVAL (operands[1]) > 8)
    {
      rtx tmp = operands[1];

      operands[1] = operands[2];
      operands[2] = tmp;
    }
  if (GET_CODE (operands[1]) == REG
      && REGNO (operands[1]) == REGNO (operands[3]))
    output_asm_insn ("add%.l %2,%3", operands);
  else
    output_asm_insn ("move%.l %2,%3\n\tadd%.l %1,%3", operands);
  if (TARGET_68020 || TARGET_COLDFIRE)
    return "smi %0\n\textb%.l %0";
  else
    return "smi %0\n\text%.w %0\n\text%.l %0";
}
}

static const char * const output_95[] = {
  "ext%.l %0",
  "move%.w %1,%0",
};

static const char *
output_99 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2013 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[0]) && FP_REG_P (operands[1]))
    {
      if (REGNO (operands[0]) == REGNO (operands[1]))
	{
	  /* Extending float to double in an fp-reg is a no-op.  */
	  return "";
	}
      return "f%&move%.x %1,%0";
    }
  if (FP_REG_P (operands[0]))
    return "f%&move%.s %f1,%0";
  if (DATA_REG_P (operands[0]) && FP_REG_P (operands[1]))
    {
      output_asm_insn ("fmove%.d %f1,%-\n\tmove%.l %+,%0", operands);
      operands[0] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
      return "move%.l %+,%0";
    }
  return "fmove%.d %f1,%0";
}
}

static const char *
output_100 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2039 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[0]) && FP_REG_P (operands[1]))
    {
      if (REGNO (operands[0]) == REGNO (operands[1]))
	{
	  /* Extending float to double in an fp-reg is a no-op.  */
	  return "";
	}
      return "fdmove%.d %1,%0";
    }
  return "fdmove%.s %f1,%0";
}
}

static const char *
output_101 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2067 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "f%$move%.x %1,%0";
  return "f%$move%.d %f1,%0";
}
}

static const char * const output_102[] = {
  "fsmove%.d %1,%0",
  "fmove%.s %1,%0",
};

static const char *
output_119 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2173 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "fmovem%.l %!,%2\n\tmoveq #16,%3\n\tor%.l %2,%3\n\tand%.w #-33,%3\n\tfmovem%.l %3,%!\n\tfmove%.l %1,%0\n\tfmovem%.l %2,%!";
}
}

static const char *
output_120 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2183 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "fmovem%.l %!,%2\n\tmoveq #16,%3\n\tor%.l %2,%3\n\tand%.w #-33,%3\n\tfmovem%.l %3,%!\n\tfmove%.w %1,%0\n\tfmovem%.l %2,%!";
}
}

static const char *
output_121 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2193 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "fmovem%.l %!,%2\n\tmoveq #16,%3\n\tor%.l %2,%3\n\tand%.w #-33,%3\n\tfmovem%.l %3,%!\n\tfmove%.b %1,%0\n\tfmovem%.l %2,%!";
}
}

static const char *
output_122 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2210 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fintrz%.x %f1,%0";
  return "fintrz%.s %f1,%0";
}
}

static const char *
output_123 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2210 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fintrz%.x %f1,%0";
  return "fintrz%.d %f1,%0";
}
}

static const char *
output_124 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2210 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fintrz%.x %f1,%0";
  return "fintrz%.x %f1,%0";
}
}

static const char *
output_125 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2221 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fintrz%.d %f1,%0";
  return "fintrz%.s %f1,%0";
}
}

static const char *
output_126 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2221 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fintrz%.d %f1,%0";
  return "fintrz%.d %f1,%0";
}
}

static const char *
output_142 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2300 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[3] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  if (REG_P (operands[1]) && REGNO (operands[1]) == REGNO (operands[0]))
    return
    "move%.l %1,%2\n\tadd%.l %2,%2\n\tsubx%.l %2,%2\n\tsub%.l %2,%3\n\tsubx%.l %2,%0";
  if (GET_CODE (operands[1]) == REG)
    operands[4] = gen_rtx_REG (SImode, REGNO (operands[1]) + 1);
  else if (GET_CODE (XEXP (operands[1], 0)) == POST_INC
        || GET_CODE (XEXP (operands[1], 0)) == PRE_DEC)
    operands[4] = operands[1];
  else
    operands[4] = adjust_address (operands[1], SImode, 4);
  if (GET_CODE (operands[1]) == MEM
   && GET_CODE (XEXP (operands[1], 0)) == PRE_DEC)
    output_asm_insn ("move%.l %4,%3", operands);
  output_asm_insn ("move%.l %1,%0\n\tsmi %2", operands);
  if (TARGET_68020 || TARGET_COLDFIRE)
    output_asm_insn ("extb%.l %2", operands);
  else
    output_asm_insn ("ext%.w %2\n\text%.l %2", operands);
  if (GET_CODE (operands[1]) != MEM
   || GET_CODE (XEXP (operands[1], 0)) != PRE_DEC)
    output_asm_insn ("move%.l %4,%3", operands);
  return "sub%.l %2,%3\n\tsubx%.l %2,%0";
}
}

static const char *
output_143 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2334 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (ADDRESS_REG_P (operands[0]))
    return "add%.w %1,%0";
  else if (ADDRESS_REG_P (operands[3]))
    return "move%.w %1,%3\n\tadd%.l %3,%0";
  else
    return "move%.w %1,%3\n\text%.l %3\n\tadd%.l %3,%0";
}
}

static const char *
output_144 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2349 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[0]) == REG)
    operands[2] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  else
    operands[2] = adjust_address (operands[0], SImode, 4);
  return "add%.l %1,%2\n\tnegx%.l %0\n\tneg%.l %0";
}
}

static const char *
output_145 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2363 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "add%.l %1,%R0\n\tnegx%.l %0\n\tneg%.l %0";
}
}

static const char *
output_146 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2376 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[1]) == REG)
    operands[1] = gen_rtx_REG (SImode, REGNO (operands[1]) + 1);
  else
    operands[1] = adjust_address (operands[1], SImode, 4);
  return "add%.l %1,%0";
}
}

static const char *
output_147 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2391 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      if (DATA_REG_P (operands[2]))
	return "add%.l %R2,%R0\n\taddx%.l %2,%0";
      else if (GET_CODE (operands[2]) == MEM
	  && GET_CODE (XEXP (operands[2], 0)) == POST_INC)
	return "move%.l %2,%3\n\tadd%.l %2,%R0\n\taddx%.l %3,%0";
      else
	{
	  rtx high, low;
	  rtx xoperands[2];

	  if (GET_CODE (operands[2]) == REG)
	    {
	      low = gen_rtx_REG (SImode, REGNO (operands[2]) + 1);
	      high = operands[2];
	    }
	  else if (CONSTANT_P (operands[2]))
	    split_double (operands[2], &high, &low);
	  else
	    {
	      low = adjust_address (operands[2], SImode, 4);
	      high = operands[2];
	    }

	  operands[1] = low, operands[2] = high;
	  xoperands[0] = operands[3];
	  if (GET_CODE (operands[1]) == CONST_INT
	      && INTVAL (operands[1]) >= -8 && INTVAL (operands[1]) < 0)
	    xoperands[1] = GEN_INT (-INTVAL (operands[2]) - 1);
	  else
	    xoperands[1] = operands[2];

	  output_asm_insn (output_move_simode (xoperands), xoperands);
	  if (GET_CODE (operands[1]) == CONST_INT)
	    {
	      if (INTVAL (operands[1]) > 0 && INTVAL (operands[1]) <= 8)
		return "addq%.l %1,%R0\n\taddx%.l %3,%0";
	      else if (INTVAL (operands[1]) >= -8 && INTVAL (operands[1]) < 0)
		{
		  operands[1] = GEN_INT (-INTVAL (operands[1]));
		  return "subq%.l %1,%R0\n\tsubx%.l %3,%0";
		}
	    }
	  return "add%.l %1,%R0\n\taddx%.l %3,%0";
	}
    }
  else
    {
      gcc_assert (GET_CODE (operands[0]) == MEM);
      if (GET_CODE (XEXP (operands[0], 0)) == POST_INC)
	{
	  operands[1] = gen_rtx_MEM (SImode,
				     plus_constant (Pmode,
						    XEXP(operands[0], 0), -8));
	  return "move%.l %0,%3\n\tadd%.l %R2,%0\n\taddx%.l %2,%3\n\tmove%.l %3,%1";
	}
      else if (GET_CODE (XEXP (operands[0], 0)) == PRE_DEC)
	{
	  operands[1] = XEXP(operands[0], 0);
	  return "add%.l %R2,%0\n\tmove%.l %0,%3\n\taddx%.l %2,%3\n\tmove%.l %3,%1";
	}
      else
	{
	  operands[1] = adjust_address (operands[0], SImode, 4);
	  return "add%.l %R2,%1\n\tmove%.l %0,%3\n\taddx%.l %2,%3\n\tmove%.l %3,%0";
	}
    }
}
}

static const char *
output_148 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2472 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = operands[0];
  operands[3] = gen_label_rtx();
  if (GET_CODE (operands[0]) == MEM)
    {
      if (GET_CODE (XEXP (operands[0], 0)) == POST_INC)
        operands[0] = gen_rtx_MEM (SImode, XEXP (XEXP (operands[0], 0), 0));
      else if (GET_CODE (XEXP (operands[0], 0)) == PRE_DEC)
        operands[2] = gen_rtx_MEM (SImode, XEXP (XEXP (operands[0], 0), 0));
    }
  output_asm_insn ("move%.l %1,%0", operands);
  output_asm_insn ("jpl %l3", operands);
  output_asm_insn ("addq%.l #1,%2", operands);
  (*targetm.asm_out.internal_label) (asm_out_file, "L",
				CODE_LABEL_NUMBER (operands[3]));
  return "";
}
}

static const char *
output_149 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2509 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
 return output_addsi3 (operands);
}

static const char *
output_150 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2517 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  switch (which_alternative)
    {
    case 0:
      return "addq%.l %2,%0";

    case 1:
      operands[2] = GEN_INT (- INTVAL (operands[2]));
      return "subq%.l %2,%0";

    case 3:
    case 4:
      return "add%.l %2,%0";

    case 5:
      /* move%.l %2,%0\n\tadd%.l %1,%0 */
      return "#";

    case 6:
      return MOTOROLA ? "lea (%1,%2.l),%0" : "lea %1@(0,%2:l),%0";

    case 7:
      return MOTOROLA ? "lea (%2,%1.l),%0" : "lea %2@(0,%1:l),%0";

    case 2:
    case 8:
      return MOTOROLA ? "lea (%c2,%1),%0" : "lea %1@(%c2),%0";

    default:
      gcc_unreachable ();
      return "";
    }
}
}

static const char *
output_152 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2574 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      /* If the constant would be a negative number when interpreted as
	 HImode, make it negative.  This is usually, but not always, done
	 elsewhere in the compiler.  First check for constants out of range,
	 which could confuse us.  */

      if (INTVAL (operands[2]) >= 32768)
	operands[2] = GEN_INT (INTVAL (operands[2]) - 65536);

      if (INTVAL (operands[2]) > 0
	  && INTVAL (operands[2]) <= 8)
	return "addq%.w %2,%0";
      if (INTVAL (operands[2]) < 0
	  && INTVAL (operands[2]) >= -8)
	{
	  operands[2] = GEN_INT (- INTVAL (operands[2]));
	  return "subq%.w %2,%0";
	}
      /* On the CPU32 it is faster to use two addqw instructions to
	 add a small integer (8 < N <= 16) to a register.
	 Likewise for subqw.  */
      if (TUNE_CPU32 && REG_P (operands[0]))
	{
	  if (INTVAL (operands[2]) > 8
	      && INTVAL (operands[2]) <= 16)
	    {
	      operands[2] = GEN_INT (INTVAL (operands[2]) - 8);
	      return "addq%.w #8,%0\n\taddq%.w %2,%0";
	    }
	  if (INTVAL (operands[2]) < -8
	      && INTVAL (operands[2]) >= -16)
	    {
	      operands[2] = GEN_INT (- INTVAL (operands[2]) - 8);
	      return "subq%.w #8,%0\n\tsubq%.w %2,%0";
	    }
	}
      if (ADDRESS_REG_P (operands[0]) && !TUNE_68040)
	return MOTOROLA ? "lea (%c2,%0),%0" : "lea %0@(%c2),%0";
    }
  return "add%.w %2,%0";
}
}

static const char *
output_153 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2633 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  gcc_assert (!ADDRESS_REG_P (operands[0]));
  if (GET_CODE (operands[1]) == CONST_INT)
    {
      /* If the constant would be a negative number when interpreted as
	 HImode, make it negative.  This is usually, but not always, done
	 elsewhere in the compiler.  First check for constants out of range,
	 which could confuse us.  */

      if (INTVAL (operands[1]) >= 32768)
	operands[1] = GEN_INT (INTVAL (operands[1]) - 65536);

      if (INTVAL (operands[1]) > 0
	  && INTVAL (operands[1]) <= 8)
	return "addq%.w %1,%0";
      if (INTVAL (operands[1]) < 0
	  && INTVAL (operands[1]) >= -8)
	{
	  operands[1] = GEN_INT (- INTVAL (operands[1]));
	  return "subq%.w %1,%0";
	}
      /* On the CPU32 it is faster to use two addqw instructions to
	 add a small integer (8 < N <= 16) to a register. 
	 Likewise for subqw.  */
      if (TUNE_CPU32 && REG_P (operands[0]))
	{
	  if (INTVAL (operands[1]) > 8
	      && INTVAL (operands[1]) <= 16)
	    {
	      operands[1] = GEN_INT (INTVAL (operands[1]) - 8);
	      return "addq%.w #8,%0\n\taddq%.w %1,%0";
	    }
	  if (INTVAL (operands[1]) < -8
	      && INTVAL (operands[1]) >= -16)
	    {
	      operands[1] = GEN_INT (- INTVAL (operands[1]) - 8);
	      return "subq%.w #8,%0\n\tsubq%.w %1,%0";
	    }
	}
    }
  return "add%.w %1,%0";
}
}

static const char *
output_154 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2682 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  gcc_assert (!ADDRESS_REG_P (operands[0]));
  if (GET_CODE (operands[1]) == CONST_INT)
    {
      /* If the constant would be a negative number when interpreted as
	 HImode, make it negative.  This is usually, but not always, done
	 elsewhere in the compiler.  First check for constants out of range,
	 which could confuse us.  */

      if (INTVAL (operands[1]) >= 32768)
	operands[1] = GEN_INT (INTVAL (operands[1]) - 65536);

      if (INTVAL (operands[1]) > 0
	  && INTVAL (operands[1]) <= 8)
	return "addq%.w %1,%0";
      if (INTVAL (operands[1]) < 0
	  && INTVAL (operands[1]) >= -8)
	{
	  operands[1] = GEN_INT (- INTVAL (operands[1]));
	  return "subq%.w %1,%0";
	}
      /* On the CPU32 it is faster to use two addqw instructions to
	 add a small integer (8 < N <= 16) to a register.
	 Likewise for subqw.  */
      if (TUNE_CPU32 && REG_P (operands[0]))
	{
	  if (INTVAL (operands[1]) > 8
	      && INTVAL (operands[1]) <= 16)
	    {
	      operands[1] = GEN_INT (INTVAL (operands[1]) - 8);
	      return "addq%.w #8,%0\n\taddq%.w %1,%0";
	    }
	  if (INTVAL (operands[1]) < -8
	      && INTVAL (operands[1]) >= -16)
	    {
	      operands[1] = GEN_INT (- INTVAL (operands[1]) - 8);
	      return "subq%.w #8,%0\n\tsubq%.w %1,%0";
	    }
	}
    }
  return "add%.w %1,%0";
}
}

static const char *
output_155 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2731 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      if (INTVAL (operands[2]) >= 128)
	operands[2] = GEN_INT (INTVAL (operands[2]) - 256);

      if (INTVAL (operands[2]) > 0
	  && INTVAL (operands[2]) <= 8)
	return "addq%.b %2,%0";
      if (INTVAL (operands[2]) < 0 && INTVAL (operands[2]) >= -8)
       {
	 operands[2] = GEN_INT (- INTVAL (operands[2]));
	 return "subq%.b %2,%0";
       }
    }
  return "add%.b %2,%0";
}
}

static const char *
output_156 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2755 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[1]) == CONST_INT)
    {
      if (INTVAL (operands[1]) >= 128)
	operands[1] = GEN_INT (INTVAL (operands[1]) - 256);

      if (INTVAL (operands[1]) > 0
	  && INTVAL (operands[1]) <= 8)
	return "addq%.b %1,%0";
      if (INTVAL (operands[1]) < 0 && INTVAL (operands[1]) >= -8)
       {
	 operands[1] = GEN_INT (- INTVAL (operands[1]));
	 return "subq%.b %1,%0";
       }
    }
  return "add%.b %1,%0";
}
}

static const char *
output_157 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2778 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[1]) == CONST_INT)
    {
      if (INTVAL (operands[1]) >= 128)
	operands[1] = GEN_INT (INTVAL (operands[1]) - 256);

      if (INTVAL (operands[1]) > 0
	  && INTVAL (operands[1]) <= 8)
	return "addq%.b %1,%0";
      if (INTVAL (operands[1]) < 0 && INTVAL (operands[1]) >= -8)
       {
	 operands[1] = GEN_INT (- INTVAL (operands[1]));
	 return "subq%.b %1,%0";
       }
    }
  return "add%.b %1,%0";
}
}

static const char *
output_167 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2835 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "f%$add%.x %2,%0";
  return "f%$add%.s %f2,%0";
}
}

static const char *
output_168 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2835 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "f%&add%.x %2,%0";
  return "f%&add%.d %f2,%0";
}
}

static const char *
output_169 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2835 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fadd%.x %2,%0";
  return "fadd%.x %f2,%0";
}
}

static const char *
output_170 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2848 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fsadd%.d %2,%0";
  return "fsadd%.s %2,%0";
}
}

static const char *
output_171 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2848 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fdadd%.d %2,%0";
  return "fdadd%.d %2,%0";
}
}

static const char *
output_172 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2865 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (ADDRESS_REG_P (operands[0]))
    return "sub%.w %2,%0";
  else if (ADDRESS_REG_P (operands[3]))
    return "move%.w %2,%3\n\tsub%.l %3,%0";
  else
    return "move%.w %2,%3\n\text%.l %3\n\tsub%.l %3,%0";
}
}

static const char *
output_173 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2880 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[1]) == REG)
    operands[1] = gen_rtx_REG (SImode, REGNO (operands[1]) + 1);
  else
    operands[1] = adjust_address (operands[1], SImode, 4);
  return "sub%.l %1,%0";
}
}

static const char *
output_174 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 2895 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      if (DATA_REG_P (operands[2]))
	return "sub%.l %R2,%R0\n\tsubx%.l %2,%0";
      else if (GET_CODE (operands[2]) == MEM
	  && GET_CODE (XEXP (operands[2], 0)) == POST_INC)
	{
	  return "move%.l %2,%3\n\tsub%.l %2,%R0\n\tsubx%.l %3,%0";
	}
      else
	{
	  rtx high, low;
	  rtx xoperands[2];

	  if (GET_CODE (operands[2]) == REG)
	    {
	      low = gen_rtx_REG (SImode, REGNO (operands[2]) + 1);
	      high = operands[2];
	    }
	  else if (CONSTANT_P (operands[2]))
	    split_double (operands[2], &high, &low);
	  else
	    {
	      low = adjust_address (operands[2], SImode, 4);
	      high = operands[2];
	    }

	  operands[1] = low, operands[2] = high;
	  xoperands[0] = operands[3];
	  if (GET_CODE (operands[1]) == CONST_INT
	      && INTVAL (operands[1]) >= -8 && INTVAL (operands[1]) < 0)
	    xoperands[1] = GEN_INT (-INTVAL (operands[2]) - 1);
	  else
	    xoperands[1] = operands[2];

	  output_asm_insn (output_move_simode (xoperands), xoperands);
	  if (GET_CODE (operands[1]) == CONST_INT)
	    {
	      if (INTVAL (operands[1]) > 0 && INTVAL (operands[1]) <= 8)
		return "subq%.l %1,%R0\n\tsubx%.l %3,%0";
	      else if (INTVAL (operands[1]) >= -8 && INTVAL (operands[1]) < 0)
		{
		  operands[1] = GEN_INT (-INTVAL (operands[1]));
		  return "addq%.l %1,%R0\n\taddx%.l %3,%0";
		}
	    }
	  return "sub%.l %1,%R0\n\tsubx%.l %3,%0";
	}
    }
  else
    {
      gcc_assert (GET_CODE (operands[0]) == MEM);
      if (GET_CODE (XEXP (operands[0], 0)) == POST_INC)
	{
	  operands[1]
	    = gen_rtx_MEM (SImode, plus_constant (Pmode,
						  XEXP (operands[0], 0), -8));
	  return "move%.l %0,%3\n\tsub%.l %R2,%0\n\tsubx%.l %2,%3\n\tmove%.l %3,%1";
	}
      else if (GET_CODE (XEXP (operands[0], 0)) == PRE_DEC)
	{
	  operands[1] = XEXP(operands[0], 0);
	  return "sub%.l %R2,%0\n\tmove%.l %0,%3\n\tsubx%.l %2,%3\n\tmove%.l %3,%1";
	}
      else
	{
	  operands[1] = adjust_address (operands[0], SImode, 4);
	  return "sub%.l %R2,%1\n\tmove%.l %0,%3\n\tsubx%.l %2,%3\n\tmove%.l %3,%0";
	}
    }
}
}

static const char * const output_175[] = {
  "subq%.l %2, %0",
  "subq%.l %2, %0",
  "sub%.l %2,%0",
  "sub%.l %2,%0",
  "sub%.l %2,%0",
};

static const char *
output_190 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3066 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "f%$sub%.x %2,%0";
  return "f%$sub%.s %f2,%0";
}
}

static const char *
output_191 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3066 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "f%&sub%.x %2,%0";
  return "f%&sub%.d %f2,%0";
}
}

static const char *
output_192 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3066 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fsub%.x %2,%0";
  return "fsub%.x %f2,%0";
}
}

static const char *
output_193 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3079 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fssub%.d %2,%0";
  return "fssub%.s %2,%0";
}
}

static const char *
output_194 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3079 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fdsub%.d %2,%0";
  return "fdsub%.d %2,%0";
}
}

static const char *
output_195 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3094 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return MOTOROLA ? "muls%.w %2,%0" : "muls %2,%0";
}
}

static const char *
output_196 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3107 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return MOTOROLA ? "muls%.w %2,%0" : "muls %2,%0";
}
}

static const char *
output_197 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3119 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return MOTOROLA ? "muls%.w %2,%0" : "muls %2,%0";
}
}

static const char *
output_200 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3158 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return MOTOROLA ? "mulu%.w %2,%0" : "mulu %2,%0";
}
}

static const char *
output_201 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3170 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return MOTOROLA ? "mulu%.w %2,%0" : "mulu %2,%0";
}
}

static const char *
output_210 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3354 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%$mul%.l %2,%0"
	 : "fsglmul%.l %2,%0";
}
}

static const char *
output_211 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3354 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%&mul%.l %2,%0"
	 : "f%&mul%.l %2,%0";
}
}

static const char *
output_212 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3354 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "fmul%.l %2,%0"
	 : "fmul%.l %2,%0";
}
}

static const char *
output_213 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3367 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%$mul%.w %2,%0"
	 : "fsglmul%.w %2,%0";
}
}

static const char *
output_214 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3367 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%&mul%.w %2,%0"
	 : "f%&mul%.w %2,%0";
}
}

static const char *
output_215 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3367 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "fmul%.w %2,%0"
	 : "fmul%.w %2,%0";
}
}

static const char *
output_216 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3380 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%$mul%.b %2,%0"
	 : "fsglmul%.b %2,%0";
}
}

static const char *
output_217 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3380 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%&mul%.b %2,%0"
	 : "f%&mul%.b %2,%0";
}
}

static const char *
output_218 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3380 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "fmul%.b %2,%0"
	 : "fmul%.b %2,%0";
}
}

static const char *
output_219 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3393 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[2]) == CONST_DOUBLE
      && floating_exact_log2 (operands[2]) && !TUNE_68040_60)
    {
      int i = floating_exact_log2 (operands[2]);
      operands[2] = GEN_INT (i);
      return "fscale%.l %2,%0";
    }
  if (REG_P (operands[2]))
    return "f%&mul%.x %2,%0";
  return "f%&mul%.d %f2,%0";
}
}

static const char *
output_220 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3411 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return (TARGET_68040
	    ? "fsmul%.x %2,%0"
	    : "fsglmul%.x %2,%0");
  return (TARGET_68040
	  ? "fsmul%.s %f2,%0"
	  : "fsglmul%.s %f2,%0");
}
}

static const char *
output_221 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3426 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "fmul%.x %f2,%0";
}
}

static const char *
output_222 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3435 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fsmul%.d %2,%0";
  return "fsmul%.s %2,%0";
}
}

static const char *
output_223 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3435 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fdmul%.d %2,%0";
  return "fdmul%.d %2,%0";
}
}

static const char *
output_224 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3457 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%$div%.l %2,%0"
	 : "fsgldiv%.l %2,%0";
}
}

static const char *
output_225 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3457 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%&div%.l %2,%0"
	 : "f%&div%.l %2,%0";
}
}

static const char *
output_226 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3457 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "fdiv%.l %2,%0"
	 : "fdiv%.l %2,%0";
}
}

static const char *
output_227 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3468 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%$div%.w %2,%0"
	 : "fsgldiv%.w %2,%0";
}
}

static const char *
output_228 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3468 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%&div%.w %2,%0"
	 : "f%&div%.w %2,%0";
}
}

static const char *
output_229 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3468 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "fdiv%.w %2,%0"
	 : "fdiv%.w %2,%0";
}
}

static const char *
output_230 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3479 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%$div%.b %2,%0"
	 : "fsgldiv%.b %2,%0";
}
}

static const char *
output_231 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3479 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "f%&div%.b %2,%0"
	 : "f%&div%.b %2,%0";
}
}

static const char *
output_232 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3479 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return TARGET_68040
	 ? "fdiv%.b %2,%0"
	 : "fdiv%.b %2,%0";
}
}

static const char *
output_233 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3490 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return (TARGET_68040
	    ? "f%$div%.x %2,%0"
	    : "fsgldiv%.x %2,%0");
  return (TARGET_68040
	  ? "f%$div%.s %f2,%0"
	  : "fsgldiv%.s %f2,%0");
}
}

static const char *
output_234 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3490 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return (TARGET_68040
	    ? "f%&div%.x %2,%0"
	    : "f%&div%.x %2,%0");
  return (TARGET_68040
	  ? "f%&div%.d %f2,%0"
	  : "f%&div%.d %f2,%0");
}
}

static const char *
output_235 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3490 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return (TARGET_68040
	    ? "fdiv%.x %2,%0"
	    : "fdiv%.x %2,%0");
  return (TARGET_68040
	  ? "fdiv%.x %f2,%0"
	  : "fdiv%.x %f2,%0");
}
}

static const char *
output_236 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3505 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fsdiv%.d %2,%0";
  return "fsdiv%.s %2,%0";
}
}

static const char *
output_237 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3505 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[2]))
    return "fddiv%.d %2,%0";
  return "fddiv%.d %2,%0";
}
}

static const char *
output_238 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3532 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (find_reg_note (insn, REG_UNUSED, operands[3]))
    return "divs%.l %2,%0";
  else if (find_reg_note (insn, REG_UNUSED, operands[0]))
    return "rems%.l %2,%3:%0";
  else
    return "rems%.l %2,%3:%0\n\tdivs%.l %2,%0";
}
}

static const char *
output_239 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3550 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (find_reg_note (insn, REG_UNUSED, operands[3]))
    return "divs%.l %2,%0";
  else
    return "divsl%.l %2,%3:%0";
}
}

static const char *
output_240 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3574 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (find_reg_note (insn, REG_UNUSED, operands[3]))
    return "divu%.l %2,%0";
  else if (find_reg_note (insn, REG_UNUSED, operands[0]))
    return "remu%.l %2,%3:%0";
  else
    return "remu%.l %2,%3:%0\n\tdivu%.l %2,%0";
}
}

static const char *
output_241 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3592 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (find_reg_note (insn, REG_UNUSED, operands[3]))
    return "divu%.l %2,%0";
  else
    return "divul%.l %2,%3:%0";
}
}

static const char *
output_242 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3606 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  output_asm_insn (MOTOROLA ?
    "ext%.l %0\n\tdivs%.w %2,%0" :
    "extl %0\n\tdivs %2,%0",
    operands);
  if (!find_reg_note(insn, REG_UNUSED, operands[3]))
    return "move%.l %0,%3\n\tswap %3";
  else
    return "";
}
}

static const char *
output_243 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3624 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (ISA_HAS_MVS_MVZ)
    output_asm_insn (MOTOROLA ?
      "mvz%.w %0,%0\n\tdivu%.w %2,%0" :
      "mvz%.w %0,%0\n\tdivu %2,%0",
      operands);
  else
    output_asm_insn (MOTOROLA ?
      "and%.l #0xFFFF,%0\n\tdivu%.w %2,%0" :
      "and%.l #0xFFFF,%0\n\tdivu %2,%0",
      operands);

  if (!find_reg_note(insn, REG_UNUSED, operands[3]))
    return "move%.l %0,%3\n\tswap %3";
  else
    return "";
}
}

static const char *
output_244 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3661 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_andsi3 (operands);
}
}

static const char *
output_245 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3670 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_andsi3 (operands);
}
}

static const char *
output_246 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3680 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (ISA_HAS_MVS_MVZ
      && DATA_REG_P (operands[0])
      && GET_CODE (operands[2]) == CONST_INT)
    {
      if (INTVAL (operands[2]) == 0x000000ff)
        return "mvz%.b %0,%0";
      else if (INTVAL (operands[2]) == 0x0000ffff)
        return "mvz%.w %0,%0";
    }
  return output_andsi3 (operands);
}
}

static const char *
output_253 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3747 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  int byte_mode;

  if (GET_CODE (operands[0]) == REG)
    operands[0] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  else
    operands[0] = adjust_address (operands[0], SImode, 4);
  if (GET_MODE (operands[1]) == SImode)
    return "or%.l %1,%0";
  byte_mode = (GET_MODE (operands[1]) == QImode);
  if (GET_CODE (operands[0]) == MEM)
    operands[0] = adjust_address (operands[0], byte_mode ? QImode : HImode,
				  byte_mode ? 3 : 2);
  if (byte_mode)
    return "or%.b %1,%0";
  else
    return "or%.w %1,%0";
}
}

static const char *
output_254 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3778 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_iorsi3 (operands);
}
}

static const char *
output_255 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3788 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_iorsi3 (operands);
}
}

static const char *
output_262 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3848 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[2]) != REG)
      operands[2] = adjust_address (operands[2], HImode, 2);
  if (GET_CODE (operands[2]) != REG
  || REGNO (operands[2]) != REGNO (operands[0]))
    output_asm_insn ("move%.w %2,%0", operands);
  return "swap %0\n\tmov%.w %1,%0";
}
}

static const char *
output_263 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3862 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  int byte_mode;

  byte_mode = (GET_MODE (operands[1]) == QImode);
  if (GET_CODE (operands[0]) == MEM)
    operands[0] = adjust_address (operands[0], byte_mode ? QImode : HImode,
				  byte_mode ? 3 : 2);
  if (byte_mode)
    return "or%.b %1,%0";
  else
    return "or%.w %1,%0";
}
}

static const char *
output_264 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3890 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_xorsi3 (operands);
}
}

static const char *
output_265 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3900 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_xorsi3 (operands);
}
}

static const char *
output_272 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3971 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (which_alternative == 0)
    return "neg%.l %0\n\tnegx%.l %0";
  if (GET_CODE (operands[0]) == REG)
    operands[1] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  else
    operands[1] = adjust_address (operands[0], SImode, 4);
  if (ADDRESS_REG_P (operands[0]))
    return "exg %/d0,%1\n\tneg%.l %/d0\n\texg %/d0,%1\n\texg %/d0,%0\n\tnegx%.l %/d0\n\texg %/d0,%0";
  else
    return "neg%.l %1\n\tnegx%.l %0";
}
}

static const char *
output_273 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 3988 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[1] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  return "neg%.l %1\n\tnegx%.l %0";
}
}

static const char *
output_280 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4146 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bchg %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "f%$neg%.x %1,%0";
  return "f%$neg%.s %f1,%0";
}
}

static const char *
output_281 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4146 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bchg %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "f%&neg%.x %1,%0";
  return "f%&neg%.d %f1,%0";
}
}

static const char *
output_282 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4146 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bchg %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "fneg%.x %1,%0";
  return "fneg%.x %f1,%0";
}
}

static const char *
output_283 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4161 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bchg %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "fsneg%.d %1,%0";
  return "fsneg%.s %1,%0";
}
}

static const char *
output_284 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4161 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bchg %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "fdneg%.d %1,%0";
  return "fdneg%.d %1,%0";
}
}

static const char *
output_285 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4184 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "f%$sqrt%.x %1,%0";
  return "f%$sqrt%.s %1,%0";
}
}

static const char *
output_286 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4184 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "f%&sqrt%.x %1,%0";
  return "f%&sqrt%.d %1,%0";
}
}

static const char *
output_287 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4184 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fsqrt%.x %1,%0";
  return "fsqrt%.x %1,%0";
}
}

static const char *
output_288 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4195 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fssqrt%.d %1,%0";
  return "fssqrt%.s %1,%0";
}
}

static const char *
output_289 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4195 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fdsqrt%.d %1,%0";
  return "fdsqrt%.d %1,%0";
}
}

static const char *
output_290 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4299 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bclr %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "f%$abs%.x %1,%0";
  return "f%$abs%.s %f1,%0";
}
}

static const char *
output_291 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4299 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bclr %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "f%&abs%.x %1,%0";
  return "f%&abs%.d %f1,%0";
}
}

static const char *
output_292 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4299 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bclr %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "fabs%.x %1,%0";
  return "fabs%.x %f1,%0";
}
}

static const char *
output_293 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4314 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bclr %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "fsabs%.d %1,%0";
  return "fsabs%.s %1,%0";
}
}

static const char *
output_294 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4314 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    {
      operands[1] = GEN_INT (31);
      return "bclr %1,%0";
    }
  if (FP_REG_P (operands[1]))
    return "fdabs%.d %1,%0";
  return "fdabs%.d %1,%0";
}
}

static const char *
output_304 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4428 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[0]) == MEM)
    {
    if (GET_CODE (XEXP (operands[0], 0)) == PRE_DEC)
      return "clr%.l %0\n\tmove%.w %1,%2\n\tmove%.l %2,%0";
    else if (GET_CODE (XEXP (operands[0], 0)) == POST_INC)
      return "move%.w %1,%2\n\tmove%.l %2,%0\n\tclr%.l %0";
    else
      {
	operands[3] = adjust_address (operands[0], SImode, 4);
	return "move%.w %1,%2\n\tmove%.l %2,%0\n\tclr%.l %3";
      }
    }
  else if (DATA_REG_P (operands[0]))
    return "move%.w %1,%0\n\text%.l %0\n\tclr%.l %R0";
  else
    return "move%.w %1,%0\n\tsub%.l %R0,%R0";
}
}

static const char *
output_309 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4640 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = GEN_INT (INTVAL (operands[2]) - 16);
  return "lsl%.w %2,%0\n\tswap %0\n\tclr%.w %0";
}
}

static const char *
output_310 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4650 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (operands[2] == const1_rtx)
    return "add%.l %0,%0";
  return "lsl%.l %2,%0";
}
}

static const char *
output_316 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4710 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = GEN_INT (INTVAL (operands[2]) - 16);
  return "swap %0\n\tasr%.w %2,%0\n\text%.l %0";
}
}

static const char *
output_317 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4720 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[1]) != REG)
    operands[1] = adjust_address (operands[1], HImode, 2);
  return "move%.w %1,%0";
}
}

static const char *
output_318 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4732 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "move%.l %1,%0";
}
}

static const char *
output_319 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4742 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[1] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  return "asr%.l #1,%0\n\troxr%.l #1,%1";
}
}

static const char *
output_320 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4808 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (TARGET_68020)
    return "move%.l %1,%R0\n\tsmi %0\n\textb%.l %0";
  else
    return "move%.l %1,%R0\n\tsmi %0\n\text%.w %0\n\text%.l %0";
}
}

static const char *
output_321 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4821 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[3] = adjust_address (operands[0], SImode,
				which_alternative == 0 ? 4 : 0);
  operands[0] = adjust_address (operands[0], SImode, 0);
  if (TARGET_68020 || TARGET_COLDFIRE)
    return "move%.l %1,%3\n\tsmi %2\n\textb%.l %2\n\tmove%.l %2,%0";
  else
    return "move%.l %1,%3\n\tsmi %2\n\text%.w %2\n\text%.l %2\n\tmove%.l %2,%0";
}
}

static const char *
output_322 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4852 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[1] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
  if (INTVAL (operands[2]) == 48)
    return "swap %0\n\text%.l %0\n\tmove%.l %0,%1\n\tsmi %0\n\text%.w %0";
  if (INTVAL (operands[2]) == 31)
    return "add%.l %1,%1\n\taddx%.l %0,%0\n\tmove%.l %0,%1\n\tsubx%.l %0,%0";
  if (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63)
    {
      operands[2] = GEN_INT (INTVAL (operands[2]) - 32);
      output_asm_insn (INTVAL (operands[2]) <= 8 ? "asr%.l %2,%0" :
			"moveq %2,%1\n\tasr%.l %1,%0", operands);
      output_asm_insn ("mov%.l %0,%1\n\tsmi %0", operands);
      return INTVAL (operands[2]) >= 15 ? "ext%.w %d0" :
	     TARGET_68020 ? "extb%.l %0" : "ext%.w %0\n\text%.l %0";
    }
  return "#";
}
}

static const char *
output_323 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4892 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "add%.l %0,%0\n\tsubx%.l %0,%0";
}
}

static const char *
output_330 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 4982 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "lsr%.l #1,%0\n\troxr%.l #1,%R0";
}
}

static const char *
output_334 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5168 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "add%.l %0,%0\n\tsubx%.l %0,%0\n\tneg%.l %0";
}
}

static const char *
output_335 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5179 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "clr%.w %0\n\tswap %0";
}
}

static const char *
output_336 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5192 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  /* I think lsr%.w sets the CC properly.  */
  operands[2] = GEN_INT (INTVAL (operands[2]) - 16);
  return "clr%.w %0\n\tswap %0\n\tlsr%.w %2,%0";
}
}

static const char *
output_343 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5256 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[2]) == CONST_INT && INTVAL (operands[2]) == 16)
    return "swap %0";
  else if (GET_CODE (operands[2]) == CONST_INT && INTVAL (operands[2]) >= 16)
    {
      operands[2] = GEN_INT (32 - INTVAL (operands[2]));
      return "ror%.l %2,%0";
    }
  else
    return "rol%.l %2,%0";
}
}

static const char *
output_344 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5274 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[2]) == CONST_INT && INTVAL (operands[2]) >= 8)
    {
      operands[2] = GEN_INT (16 - INTVAL (operands[2]));
      return "ror%.w %2,%0";
    }
  else
    return "rol%.w %2,%0";
}
}

static const char *
output_345 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5290 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[1]) == CONST_INT && INTVAL (operands[1]) >= 8)
    {
      operands[1] = GEN_INT (16 - INTVAL (operands[1]));
      return "ror%.w %1,%0";
    }
  else
    return "rol%.w %1,%0";
}
}

static const char *
output_346 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5306 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[2]) == CONST_INT && INTVAL (operands[2]) >= 4)
    {
      operands[2] = GEN_INT (8 - INTVAL (operands[2]));
      return "ror%.b %2,%0";
    }
  else
    return "rol%.b %2,%0";
}
}

static const char *
output_347 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5322 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[1]) == CONST_INT && INTVAL (operands[1]) >= 4)
    {
      operands[1] = GEN_INT (8 - INTVAL (operands[1]));
      return "ror%.b %1,%0";
    }
  else
    return "rol%.b %1,%0";
}
}

static const char *
output_360 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5482 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0]
    = adjust_address (operands[0], SImode, INTVAL (operands[1]) / 8);

  return "move%.l %2,%0";
}
}

static const char *
output_361 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5498 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (INTVAL (operands[1]) + INTVAL (operands[2]) != 32)
    return "bfins %3,%0{%b2:%b1}";

  if (INTVAL (operands[1]) == 8)
    return "move%.b %3,%0";
  return "move%.w %3,%0";
}
}

static const char *
output_362 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5524 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[1]
    = adjust_address (operands[1], SImode, INTVAL (operands[2]) / 8);

  return "move%.l %1,%0";
}
}

static const char *
output_363 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5540 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (INTVAL (operands[2]) + INTVAL (operands[3]) != 32)
    return "bfextu %1{%b3:%b2},%0";

  output_asm_insn ("clr%.l %0", operands);
  if (INTVAL (operands[2]) == 8)
    return "move%.b %1,%0";
  return "move%.w %1,%0";
}
}

static const char *
output_364 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5566 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[1]
    = adjust_address (operands[1], SImode, INTVAL (operands[2]) / 8);

  return "move%.l %1,%0";
}
}

static const char *
output_365 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5582 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (INTVAL (operands[2]) + INTVAL (operands[3]) != 32)
    return "bfexts %1{%b3:%b2},%0";

  if (INTVAL (operands[2]) == 8)
    return "move%.b %1,%0\n\textb%.l %0";
  return "move%.w %1,%0\n\text%.l %0";
}
}

static const char *
output_367 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5625 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "bfextu %1{%b3:%b2},%0";
}
}

static const char *
output_368 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5639 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "bfchg %0{%b2:%b1}";
}
}

static const char *
output_369 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5649 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "bfclr %0{%b2:%b1}";
}
}

static const char *
output_370 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5659 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "bfset %0{%b2:%b1}";
}
}

static const char *
output_373 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5707 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "bfextu %1{%b3:%b2},%0";
}
}

static const char *
output_374 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5717 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "bfclr %0{%b2:%b1}";
}
}

static const char *
output_375 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5727 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return "bfset %0{%b2:%b1}";
}
}

static const char *
output_376 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5737 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
#if 0
  /* These special cases are now recognized by a specific pattern.  */
  if (GET_CODE (operands[1]) == CONST_INT && GET_CODE (operands[2]) == CONST_INT
      && INTVAL (operands[1]) == 16 && INTVAL (operands[2]) == 16)
    return "move%.w %3,%0";
  if (GET_CODE (operands[1]) == CONST_INT && GET_CODE (operands[2]) == CONST_INT
      && INTVAL (operands[1]) == 24 && INTVAL (operands[2]) == 8)
    return "move%.b %3,%0";
#endif
  return "bfins %3,%0{%b2:%b1}";
}
}

static const char *
output_377 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5755 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_scc_di (operands[1], operands[2], const0_rtx, operands[0]);
}
}

static const char *
output_378 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5764 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_scc_di (operands[1], operands[2], const0_rtx, operands[0]);
}
}

static const char *
output_379 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5774 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_scc_di (operands[1], operands[2], operands[3], operands[0]);
}
}

static const char *
output_380 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5784 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_scc_di (operands[1], operands[2], operands[3], operands[0]);
}
}

static const char *
output_382 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5814 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return MOTOROLA ? "jmp (%0)" : "jmp %0@";
}
}

static const char *
output_383 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5826 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
#ifdef ASM_RETURN_CASE_JUMP
  ASM_RETURN_CASE_JUMP;
#else
  return MOTOROLA ? "jmp (2,pc,%0.l)" : "jmp pc@(2,%0:l)";
#endif
}
}

static const char *
output_384 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5840 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
#ifdef ASM_RETURN_CASE_JUMP
  ASM_RETURN_CASE_JUMP;
#else
  if (TARGET_COLDFIRE)
    {
      if (ADDRESS_REG_P (operands[0]))
	return MOTOROLA ? "jmp (2,pc,%0.l)" : "jmp pc@(2,%0:l)";
      else if (MOTOROLA)
	return "ext%.l %0\n\tjmp (2,pc,%0.l)";
      else
	return "extl %0\n\tjmp pc@(2,%0:l)";
    }
  else
    return MOTOROLA ? "jmp (2,pc,%0.w)" : "jmp pc@(2,%0:w)";
#endif
}
}

static const char *
output_385 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5870 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1";
  if (GET_CODE (operands[0]) == MEM)
    return "subq%.w #1,%0\n\tjcc %l1";
  return "subq%.w #1,%0\n\tcmp%.w #-1,%0\n\tjne %l1";
}
}

static const char *
output_386 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5889 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1\n\tclr%.w %0\n\tsubq%.l #1,%0\n\tjcc %l1";
  if (GET_CODE (operands[0]) == MEM)
    return "subq%.l #1,%0\n\tjcc %l1";
  return "subq%.l #1,%0\n\tcmp%.l #-1,%0\n\tjne %l1";
}
}

static const char *
output_387 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5911 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1";
  if (GET_CODE (operands[0]) == MEM)
    return "subq%.w #1,%0\n\tjcc %l1";
  return "subq%.w #1,%0\n\tcmp%.w #-1,%0\n\tjne %l1";
}
}

static const char *
output_388 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5945 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (DATA_REG_P (operands[0]))
    return "dbra %0,%l1\n\tclr%.w %0\n\tsubq%.l #1,%0\n\tjcc %l1";
  if (GET_CODE (operands[0]) == MEM)
    return "subq%.l #1,%0\n\tjcc %l1";
  return "subq%.l #1,%0\n\tcmp%.l #-1,%0\n\tjne %l1";
}
}

static const char *
output_389 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5965 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_sibcall (operands[0]);
}
}

static const char *
output_390 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 5983 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = operands[1];
  return output_sibcall (operands[0]);
}
}

static const char *
output_391 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6003 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return output_call (operands[0]);
}
}

static const char *
output_393 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6036 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = operands[1];
  return m68k_symbolic_call;
}
}

static const char *
output_394 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6051 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = operands[1];
  return m68k_symbolic_call;
}
}

static const char *
output_397 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6135 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  switch (m68k_get_function_kind (current_function_decl))
    {
    case m68k_fk_interrupt_handler:
      return "rte";

    case m68k_fk_interrupt_thread:
      return "sleep";

    default:
      if (crtl->args.pops_args)
	{
	  operands[0] = GEN_INT (crtl->args.pops_args);
	  return "rtd %0";
	}
      else
	return "rts";
    }
}
}

static const char *
output_398 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6159 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return m68k_output_movem (operands, operands[0], 0, true);
}
}

static const char *
output_399 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6169 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return m68k_output_movem (operands, operands[0], INTVAL (operands[3]), true);
}
}

static const char *
output_400 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6176 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return m68k_output_movem (operands, operands[0], 0, false);
}
}

static const char *
output_401 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6187 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  return m68k_output_movem (operands, operands[0],
			    INTVAL (operands[3]), false);
}
}

static const char *
output_402 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6216 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[1] = GEN_INT (INTVAL (operands[1]) + 4);
  if (!MOTOROLA)
    return "link %0,%1";
  else if (INTVAL (operands[1]) >= -0x8000)
    return "link.w %0,%1";
  else
    return "link.l %0,%1";
}
}

static const char *
output_404 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6253 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (TARGET_ID_SHARED_LIBRARY)
    {
      operands[1] = gen_rtx_REG (Pmode, PIC_REG);
      return MOTOROLA ? "move.l %?(%1),%0" : "movel %1@(%?), %0";
    }
  else if (MOTOROLA)
    {
      if (TARGET_COLDFIRE)
	/* Load the full 32-bit PC-relative offset of
	   _GLOBAL_OFFSET_TABLE_ into the PIC register, then use it to
	   calculate the absolute value.  The offset and "lea"
	   operation word together occupy 6 bytes.  */
	return ("move.l #_GLOBAL_OFFSET_TABLE_@GOTPC, %0\n\t"
		"lea (-6, %%pc, %0), %0");
      else
	return "lea (%%pc, _GLOBAL_OFFSET_TABLE_@GOTPC), %0";
    }
  else
    return ("movel #_GLOBAL_OFFSET_TABLE_, %0\n\t"
	    "lea %%pc@(0,%0:l),%0");
}
}

static const char *
output_407 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6524 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[0]) && FP_REG_P (operands[1]))
    {
      if (REGNO (operands[0]) == REGNO (operands[1]))
	{
	  /* Extending float to double in an fp-reg is a no-op.  */
	  return "";
	}
      return "f%$move%.x %1,%0";
    }
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "f%$move%.x %1,%0";
      else if (ADDRESS_REG_P (operands[1]))
	return "move%.l %1,%-\n\tf%$move%.s %+,%0";
      else if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_single (operands);
      return "f%$move%.s %f1,%0";
    }
  return "fmove%.x %f1,%0";
}
}

static const char *
output_408 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6553 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[0]) && FP_REG_P (operands[1]))
    {
      if (REGNO (operands[0]) == REGNO (operands[1]))
	{
	  /* Extending float to double in an fp-reg is a no-op.  */
	  return "";
	}
      return "fmove%.x %1,%0";
    }
  if (FP_REG_P (operands[0]))
    {
      if (REG_P (operands[1]))
	{
	  rtx xoperands[2];
	  xoperands[1] = gen_rtx_REG (SImode, REGNO (operands[1]) + 1);
	  output_asm_insn ("move%.l %1,%-", xoperands);
	  output_asm_insn ("move%.l %1,%-", operands);
	  return "f%&move%.d %+,%0";
	}
      if (GET_CODE (operands[1]) == CONST_DOUBLE)
	return output_move_const_double (operands);
      return "f%&move%.d %f1,%0";
    }
  return "fmove%.x %f1,%0";
}
}

static const char *
output_409 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6585 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (REG_P (operands[0]))
    {
      output_asm_insn ("fmove%.d %f1,%-\n\tmove%.l %+,%0", operands);
      operands[0] = gen_rtx_REG (SImode, REGNO (operands[0]) + 1);
      return "move%.l %+,%0";
    }
  return "fmove%.d %f1,%0";
}
}

static const char *
output_411 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6607 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fsin%.x %1,%0";
  else
    return "fsin%.s %1,%0";
}
}

static const char *
output_412 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6607 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fsin%.x %1,%0";
  else
    return "fsin%.d %1,%0";
}
}

static const char *
output_413 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6607 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fsin%.x %1,%0";
  else
    return "fsin%.x %1,%0";
}
}

static const char *
output_414 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6619 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fcos%.x %1,%0";
  else
    return "fcos%.s %1,%0";
}
}

static const char *
output_415 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6619 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fcos%.x %1,%0";
  else
    return "fcos%.d %1,%0";
}
}

static const char *
output_416 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6619 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (FP_REG_P (operands[1]))
    return "fcos%.x %1,%0";
  else
    return "fcos%.x %1,%0";
}
}

static const char *
output_418 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6643 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_qi (operands[1], operands[2], code);
  switch (code)
  {
  case EQ:  return "trapeq";
  case NE:  return "trapne";
  case GT:  return "trapgt";
  case GTU: return "traphi";
  case LT:  return "traplt";
  case LTU: return "trapcs";
  case GE:  return "trapge";
  case GEU: return "trapcc";
  case LE:  return "traple";
  case LEU: return "trapls";
  default: gcc_unreachable ();
  }
}
}

static const char *
output_419 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6643 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_hi (operands[1], operands[2], code);
  switch (code)
  {
  case EQ:  return "trapeq";
  case NE:  return "trapne";
  case GT:  return "trapgt";
  case GTU: return "traphi";
  case LT:  return "traplt";
  case LTU: return "trapcs";
  case GE:  return "trapge";
  case GEU: return "trapcc";
  case LE:  return "traple";
  case LEU: return "trapls";
  default: gcc_unreachable ();
  }
}
}

static const char *
output_420 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6643 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_si (operands[1], operands[2], code);
  switch (code)
  {
  case EQ:  return "trapeq";
  case NE:  return "trapne";
  case GT:  return "trapgt";
  case GTU: return "traphi";
  case LT:  return "traplt";
  case LTU: return "trapcs";
  case GE:  return "trapge";
  case GEU: return "trapcc";
  case LE:  return "traple";
  case LEU: return "trapls";
  default: gcc_unreachable ();
  }
}
}

static const char *
output_421 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6668 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_qi (operands[1], operands[2], code);
  switch (code)
  {
  case EQ:  return "trapeq";
  case NE:  return "trapne";
  case GT:  return "trapgt";
  case GTU: return "traphi";
  case LT:  return "traplt";
  case LTU: return "trapcs";
  case GE:  return "trapge";
  case GEU: return "trapcc";
  case LE:  return "traple";
  case LEU: return "trapls";
  default: gcc_unreachable ();
  }
}
}

static const char *
output_422 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6668 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_hi (operands[1], operands[2], code);
  switch (code)
  {
  case EQ:  return "trapeq";
  case NE:  return "trapne";
  case GT:  return "trapgt";
  case GTU: return "traphi";
  case LT:  return "traplt";
  case LTU: return "trapcs";
  case GE:  return "trapge";
  case GEU: return "trapcc";
  case LE:  return "traple";
  case LEU: return "trapls";
  default: gcc_unreachable ();
  }
}
}

static const char *
output_423 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6668 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  code = m68k_output_compare_si (operands[1], operands[2], code);
  switch (code)
  {
  case EQ:  return "trapeq";
  case NE:  return "trapne";
  case GT:  return "trapgt";
  case GTU: return "traphi";
  case LT:  return "traplt";
  case LTU: return "trapcs";
  case GE:  return "trapge";
  case GEU: return "trapcc";
  case LE:  return "traple";
  case LEU: return "trapls";
  default: gcc_unreachable ();
  }
}
}

static const char *
output_541 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6488 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_qi (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_542 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6488 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_qi (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_543 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6488 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_hi (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_544 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6488 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_hi (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_545 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6488 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_si (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_546 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6488 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_si (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_547 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6513 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_qi (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_548 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6513 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_qi (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_549 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6513 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_hi (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_550 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6513 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_hi (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_551 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6513 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_si (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}

static const char *
output_552 (rtx *operands ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
#line 6513 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[3]);
  code = m68k_output_compare_si (operands[4], operands[5], code);
  output_dbcc_and_branch (operands, code);
  return "";
}
}



static const struct insn_operand_data operand_data[] = 
{
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    0,
    0
  },
  {
    push_operand,
    "=m,m",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f,ro<>E",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    push_operand,
    "=m",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "ro<>Fi",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d*a,o,<>",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    ",,",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    scratch_operand,
    "=d,&d,d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    general_operand,
    "d,o,*a",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    ",,",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    scratch_operand,
    "=d,&d,X",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    scratch_operand,
    "=d,d,d,X",
    E_DImode,
    0,
    0,
    0,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "0,d,am,d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d,0,C0,C0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    scratch_operand,
    "=X,X,X,d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "dn,dm,>",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dm,nd,>",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "rnm,d,n,m,>",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d,rnm,m,n,>",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "r,r,r,mr,ma,>",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "mrC0,mr,ma,KTrC0,Ksr,>",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const0_operand,
    "C0",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "dm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    const0_operand,
    "C0",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "mr,r,rm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "r,mrKs,C0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d,d,d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "dn,dm,>",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dm,nd,>",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d,d,d,d,d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "rnm,d,n,m,>",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d,rnm,m,n,>",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d,d,d,d,d,d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "r,r,r,mr,ma,>",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "mrC0,mr,ma,KTrC0,Ksr,>",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const0_operand,
    "C0",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "dm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    const0_operand,
    "C0",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d,d,d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "mr,r,rm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "r,mrKs,C0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    equality_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    memory_src_operand,
    "oS,o",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "di,d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    equality_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "di",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    equality_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    memory_operand,
    "m",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    equality_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "do,dQ",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "n,n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    memory_operand,
    "o",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    memory_operand,
    "o",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f,f,dmF,fdm",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    fp_src_operand,
    "f,dmF,f,H",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f,f,mF,fm",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    fp_src_operand,
    "f,mF,f,H",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f,f,mF,fm",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    fp_src_operand,
    "f,mF,f,H",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f,f,d<Q>U,fd<Q>U",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    fp_src_operand,
    "f,d<Q>U,f,H",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f,f,<Q>U,f<Q>U",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    fp_src_operand,
    "f,<Q>U,f,H",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f,f,<Q>U,f<Q>U",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    fp_src_operand,
    "f,<Q>U,f,H",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d,d,d,d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    m68k_cstore_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f,f,dmF,fdm",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    fp_src_operand,
    "f,dmF,f,H",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d,d,d,d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    m68k_cstore_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f,f,mF,fm",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    fp_src_operand,
    "f,mF,f,H",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d,d,d,d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    m68k_cstore_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f,f,mF,fm",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    fp_src_operand,
    "f,mF,f,H",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    m68k_cstore_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "fd<Q>U",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    const0_operand,
    "H",
    E_SFmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    m68k_cstore_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    fp_src_operand,
    "f<Q>U",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    const0_operand,
    "H",
    E_DFmode,
    0,
    0,
    1,
    0
  },
  {
    push_operand,
    "=m,m,m",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "C0,R,J",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    movsi_const0_operand,
    "=d,a,g",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    movsi_const0_operand,
    "=a,g",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=g,d,a<",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "damSnT,n,i",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=g,d,a<",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "damSKT,n,i",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=g,d,d,d,d,d,a,Ap,a,r<Q>,g,U",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "R,CQ,CW,CZ,CS,Ci,J,JCs,Cs,g,Rr<Q>,U",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=a<",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    pcrel_address,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=g",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "gS",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=r<Q>,g,U",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "g,r<Q>,U",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "+dm",
    E_HImode,
    1,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "rmSn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "+d,m",
    E_HImode,
    1,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "rmn,r",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d,*a,m",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dmSi*a,di*a,dmSi",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d<Q>,dm,U,d*a",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dmi,d<Q>,U,di*a",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "+dm",
    E_QImode,
    1,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dmSn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "+d,Ac,d,m",
    E_QImode,
    1,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "C0,C0,dmn,d",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=rmf",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rmfF",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=r<Q>,g,U",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "g,r<Q>,U",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=r<Q>U,f,f,mr,f,r<Q>,f,m",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f,r<Q>U,f,rm,F,F,m,f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=rm,rf,rf,&rof<>",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "*rf,m,0,*rofE<>",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=r,g",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "g,r",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f,<Q>U,r,f,r,r,m,f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f<Q>U,f,f,r,r,m,r,E",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f,m,f,!r,!f,!r,m,!r",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "m,f,f,f,r,!r,!r,m",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=rm,rf,&rof<>",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "rf,m,rof<>",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=r,g",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "g,r",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=rm,r,&ro<>",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rF,m,roi<>F",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=r,g",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "g,r",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    push_operand,
    "=m",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    address_operand,
    "p",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm,d",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "doJS,i",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm,d",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "doJS,i",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm,d",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "roJS,i",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    post_inc_operand,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    pre_dec_operand,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_src_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_src_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_src_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_src_operand,
    "rmS",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_src_operand,
    "dmS",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_src_operand,
    "dmS",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "rmS",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "rmS",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d,o,o,<",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_src_operand,
    "rm,rm,r<Q>,rm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    scratch_operand,
    "=X,d,d,d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    register_operand,
    "=d",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "%rn",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rmn",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_src_operand,
    "rmS",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=*d,a",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_src_operand,
    "0,rmS",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "0",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "rms",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "0",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=*fdm,f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f,dmF",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f,f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f,<Q>U",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fmG",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f,d<Q>U",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "<Q>U,f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmi",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmi",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmi",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d<Q>U",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d<Q>U",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d<Q>U",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d<Q>U",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d<Q>U",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d<Q>U",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    0
  },
  {
    scratch_operand,
    "=d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    scratch_operand,
    "=d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    nonimmediate_operand,
    "=dm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    0
  },
  {
    scratch_operand,
    "=d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    scratch_operand,
    "=d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    nonimmediate_operand,
    "=dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    0
  },
  {
    scratch_operand,
    "=d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    scratch_operand,
    "=d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fdm",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fm",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fm",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fd<Q>U",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f<Q>U",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d<Q>U",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d<Q>U",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d<Q>U",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d<Q>U",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d<Q>U",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d<Q>U",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rm",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    scratch_operand,
    "=d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    nonimmediate_operand,
    "=o,a,*d,*d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rm,rm,rm,rm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0,0,0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    scratch_operand,
    "=&d,X,a,?d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    nonimmediate_operand,
    "=d,o",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "ro,d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "0",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=r,o",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "ro,d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=o<>,d,d,d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0,0,0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d,no>,d,a",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    scratch_operand,
    "=&d,&d,X,&d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    nonimmediate_operand,
    "=dm,dm,d<Q>",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rm,r<Q>,rm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=m,?a,?a,d,a",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,a,rJK,0,0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dIKLT,rJK,a,mSrIKLT,mSrIKLs",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=mr,mr,a,m,r,?a,?a,?a,?a",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0,0,0,0,a,a,r,a",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "I,L,JCu,d,mrKi,Cj,r,a,JCu",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=a",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_src_operand,
    "rmS",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=m,r",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dn,rmSn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "+m,d",
    E_HImode,
    1,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dn,rmSn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=m,d",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dn,dmSn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "+m,d",
    E_QImode,
    1,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dn,dmSn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmi",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmi",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmi",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dmn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fdmF",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fmG",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fm",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fd<Q>U",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f<Q>U",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=o,a,*d,*d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0,0,0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rm,rm,rm,rm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    scratch_operand,
    "=&d,X,a,?d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    nonimmediate_operand,
    "+ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=o<>,d,d,d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0,0,0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d,no>,d,a",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    scratch_operand,
    "=&d,&d,X,&d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    nonimmediate_operand,
    "=md,ma,m,d,a",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0,0,0,0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "I,I,dT,mSrT,mSrs",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=m,r",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dn,rmSn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=m,d",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dn,dmSn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fdmF",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fmG",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fm",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fd<Q>U",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f<Q>U",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dmSn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "%0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_src_operand,
    "dmS",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "%0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dmSTK",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d<Q>",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "%0",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "dm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "%0",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "%1",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "dm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "1",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_uint32_operand,
    "n",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "1",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_sint32_operand,
    "n",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "%0",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "fm",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f<Q>Ud",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f<Q>Ud",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "d<Q>U",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=&d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dmSTK",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dmSKT",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "i",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    not_sp_operand,
    "=m,d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dKT,dmSM",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    not_sp_operand,
    "=m,d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "d,dmsK",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=m,d",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dn,dmSn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "+m,d",
    E_HImode,
    1,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dn,dmSn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=o,d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dn,dmn",
    E_VOIDmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=m,d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "dKT,dmSMT",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=m,d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "d,dmsK",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=&d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rmn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "or",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=o,d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dn,dmn",
    E_VOIDmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d,o,m",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0,0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "di,dK,dKT",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm,d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0,0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d,Ks",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "+dm",
    E_HImode,
    1,
    0,
    1,
    1
  },
  {
    general_operand,
    "dn",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "%0",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "+dm",
    E_QImode,
    1,
    0,
    1,
    1
  },
  {
    general_operand,
    "dn",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=<,do,!*a",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0,0,0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "0",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f,d",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fdmF,0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f,d",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fmG,0",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f,d",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fm,0",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f,d",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "fd<Q>U,0",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f,d",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f<Q>U,0",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "do",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    extend_operator,
    "",
    E_DImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "=m,a*d",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "rm,rm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    scratch_operand,
    "=a,X",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    register_operand,
    "=d",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=ro<>",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "dI",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "dI",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "+d",
    E_HImode,
    1,
    0,
    1,
    0
  },
  {
    general_operand,
    "dI",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "dI",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "+d",
    E_QImode,
    1,
    0,
    1,
    0
  },
  {
    general_operand,
    "dI",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=rm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=rm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_src_operand,
    "ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    memory_operand,
    "=o,<",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_src_operand,
    "ro,ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    scratch_operand,
    "=d,d",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    nonimmediate_operand,
    "=ro<>",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "dINO",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=d",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "dIP",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "+d",
    E_HImode,
    1,
    0,
    1,
    0
  },
  {
    general_operand,
    "dIP",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    memory_operand,
    "+m",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    memory_operand,
    "+m",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d",
    E_VOIDmode,
    0,
    0,
    1,
    1
  },
  {
    extend_operator,
    "",
    E_SImode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "0",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "+o",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_src_operand,
    "rmSi",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "+d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=rm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    memory_src_operand,
    "oS",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=&d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "o",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonmemory_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonmemory_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "+o",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonmemory_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonmemory_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "n",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "+o",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    memory_operand,
    "+o",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonmemory_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonmemory_operand,
    "dn",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm,dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "ro,r",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "r,ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=d,d",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "ro,r",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "r,ro",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "a",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "r",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "r",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "+d*g",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "+d*g",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "+d*am",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "+d*am",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    sibcall_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "=rf,rf",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    sibcall_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    call_operand,
    "a,W",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "g,g",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "=rf,rf",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    non_symbolic_call_operand,
    "a,W",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "g,g",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "=rf,rf",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    symbolic_operand,
    "a,W",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "g,g",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "=a",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "1",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "+r",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "=a",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    address_operand,
    "p",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=fm,f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f,rmF",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=fm,f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f,rmE",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=m,!r",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f,f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=dm",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "f",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "dn,dm,>",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "dm,nd,>",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const1_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "rnm,d,n,m,>",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "d,rnm,m,n,>",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    const1_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "r,r,r,mr,ma,>",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "mrC0,mr,ma,KTrC0,Ksr,>",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    const1_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "dm",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "C0",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const1_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "dm",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "C0",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    const1_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "mr,r,rm",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "r,mrKs,C0",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    const1_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "r",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "r",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "+m",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "1",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "+m",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "1",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "d",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "+m",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "1",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "d",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    scratch_operand,
    "",
    E_DImode,
    0,
    0,
    0,
    0
  },
  {
    scratch_operand,
    "",
    E_SImode,
    0,
    0,
    0,
    0
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    1
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    m68k_comparison_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    m68k_comparison_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    m68k_comparison_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    m68k_comparison_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    m68k_comparison_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    m68k_comparison_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    0
  },
  {
    fp_src_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    0
  },
  {
    fp_src_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    0
  },
  {
    fp_src_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    m68k_cstore_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    0
  },
  {
    fp_src_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    m68k_cstore_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    0
  },
  {
    fp_src_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    m68k_cstore_comparison_operator,
    "",
    E_QImode,
    0,
    1,
    0,
    0
  },
  {
    register_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    0
  },
  {
    fp_src_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_HImode,
    1,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_QImode,
    1,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "mf",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=&a",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "f",
    E_SFmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=&a",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "=f",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "mf",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "=&a",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "f",
    E_DFmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "=&a",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_src_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_src_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_DFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    not_sp_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_src_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    nonimmediate_operand,
    "",
    E_XFmode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_DImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    reg_or_pow2_m1_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    nonimmediate_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    0,
    "",
    E_VOIDmode,
    0,
    0,
    1,
    0
  },
  {
    ordered_comparison_operator,
    "",
    E_VOIDmode,
    0,
    1,
    0,
    0
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    general_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_HImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    1
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
  {
    register_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    0
  },
  {
    memory_operand,
    "",
    E_QImode,
    0,
    0,
    1,
    1
  },
  {
    const_int_operand,
    "",
    E_SImode,
    0,
    0,
    1,
    0
  },
};


#if GCC_VERSION >= 2007
__extension__
#endif

const struct insn_data_d insn_data[] = 
{
  /* <internal>:0 */
  {
    "*placeholder_for_nothing",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { 0 },
    &operand_data[0],
    0,
    0,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:291 */
  {
    "*movdf_internal",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .multi = output_1 },
#else
    { 0, output_1, 0 },
#endif
    { 0 },
    &operand_data[1],
    2,
    2,
    0,
    2,
    2
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:306 */
  {
    "pushdi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_pushdi },
    &operand_data[3],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:320 */
  {
    "beq0_di",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_3 },
#else
    { 0, 0, output_3 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_beq0_di },
    &operand_data[5],
    2,
    3,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:359 */
  {
    "bne0_di",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_4 },
#else
    { 0, 0, output_4 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_bne0_di },
    &operand_data[8],
    2,
    3,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:392 */
  {
    "cbranchdi4_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_5 },
#else
    { 0, 0, output_5 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchdi4_insn },
    &operand_data[11],
    5,
    6,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:503 */
  {
    "cbranchqi4_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_6 },
#else
    { 0, 0, output_6 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchqi4_insn },
    &operand_data[17],
    4,
    4,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:503 */
  {
    "cbranchhi4_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_7 },
#else
    { 0, 0, output_7 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchhi4_insn },
    &operand_data[21],
    4,
    4,
    0,
    5,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:503 */
  {
    "cbranchsi4_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_8 },
#else
    { 0, 0, output_8 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsi4_insn },
    &operand_data[25],
    4,
    4,
    0,
    6,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:518 */
  {
    "cbranchqi4_insn_rev",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_9 },
#else
    { 0, 0, output_9 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchqi4_insn_rev },
    &operand_data[17],
    4,
    4,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:518 */
  {
    "cbranchhi4_insn_rev",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_10 },
#else
    { 0, 0, output_10 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchhi4_insn_rev },
    &operand_data[21],
    4,
    4,
    0,
    5,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:518 */
  {
    "cbranchsi4_insn_rev",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_11 },
#else
    { 0, 0, output_11 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsi4_insn_rev },
    &operand_data[25],
    4,
    4,
    0,
    6,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:533 */
  {
    "cbranchqi4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_12 },
#else
    { 0, 0, output_12 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchqi4_insn_cf },
    &operand_data[29],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:533 */
  {
    "cbranchhi4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_13 },
#else
    { 0, 0, output_13 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchhi4_insn_cf },
    &operand_data[33],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:533 */
  {
    "cbranchsi4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_14 },
#else
    { 0, 0, output_14 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsi4_insn_cf },
    &operand_data[37],
    4,
    4,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:548 */
  {
    "cbranchqi4_insn_cf_rev",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_15 },
#else
    { 0, 0, output_15 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchqi4_insn_cf_rev },
    &operand_data[29],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:548 */
  {
    "cbranchhi4_insn_cf_rev",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_16 },
#else
    { 0, 0, output_16 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchhi4_insn_cf_rev },
    &operand_data[33],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:548 */
  {
    "cbranchsi4_insn_cf_rev",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_17 },
#else
    { 0, 0, output_17 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsi4_insn_cf_rev },
    &operand_data[37],
    4,
    4,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:563 */
  {
    "cstoreqi4_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_18 },
#else
    { 0, 0, output_18 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoreqi4_insn },
    &operand_data[41],
    4,
    4,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:563 */
  {
    "cstorehi4_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_19 },
#else
    { 0, 0, output_19 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstorehi4_insn },
    &operand_data[45],
    4,
    4,
    0,
    5,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:563 */
  {
    "cstoresi4_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_20 },
#else
    { 0, 0, output_20 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoresi4_insn },
    &operand_data[49],
    4,
    4,
    0,
    6,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:576 */
  {
    "cstoreqi4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_21 },
#else
    { 0, 0, output_21 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoreqi4_insn_cf },
    &operand_data[53],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:576 */
  {
    "cstorehi4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_22 },
#else
    { 0, 0, output_22 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstorehi4_insn_cf },
    &operand_data[57],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:576 */
  {
    "cstoresi4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_23 },
#else
    { 0, 0, output_23 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoresi4_insn_cf },
    &operand_data[61],
    4,
    4,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:593 */
  {
    "cbranchsi4_btst_mem_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_24 },
#else
    { 0, 0, output_24 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsi4_btst_mem_insn },
    &operand_data[65],
    4,
    4,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:611 */
  {
    "cbranchsi4_btst_reg_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_25 },
#else
    { 0, 0, output_25 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsi4_btst_reg_insn },
    &operand_data[69],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:630 */
  {
    "cbranchsi4_btst_mem_insn_1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_26 },
#else
    { 0, 0, output_26 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsi4_btst_mem_insn_1 },
    &operand_data[73],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:647 */
  {
    "cbranchsi4_btst_reg_insn_1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_27 },
#else
    { 0, 0, output_27 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsi4_btst_reg_insn_1 },
    &operand_data[77],
    4,
    4,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:681 */
  {
    "cbranch_bftstqi_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_28 },
#else
    { 0, 0, output_28 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranch_bftstqi_insn },
    &operand_data[81],
    5,
    5,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:681 */
  {
    "cbranch_bftstsi_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_29 },
#else
    { 0, 0, output_29 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranch_bftstsi_insn },
    &operand_data[86],
    5,
    5,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:700 */
  {
    "cstore_bftstqi_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_30 },
#else
    { 0, 0, output_30 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstore_bftstqi_insn },
    &operand_data[91],
    5,
    5,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:700 */
  {
    "cstore_bftstsi_insn",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_31 },
#else
    { 0, 0, output_31 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstore_bftstsi_insn },
    &operand_data[96],
    5,
    5,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:738 */
  {
    "cbranchsf4_insn_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_32 },
#else
    { 0, 0, output_32 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsf4_insn_68881 },
    &operand_data[101],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:738 */
  {
    "cbranchdf4_insn_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_33 },
#else
    { 0, 0, output_33 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchdf4_insn_68881 },
    &operand_data[105],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:738 */
  {
    "cbranchxf4_insn_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_34 },
#else
    { 0, 0, output_34 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchxf4_insn_68881 },
    &operand_data[109],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:756 */
  {
    "cbranchsf4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_35 },
#else
    { 0, 0, output_35 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsf4_insn_cf },
    &operand_data[113],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:756 */
  {
    "cbranchdf4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_36 },
#else
    { 0, 0, output_36 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchdf4_insn_cf },
    &operand_data[117],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:756 */
  {
    "cbranchxf4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_37 },
#else
    { 0, 0, output_37 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchxf4_insn_cf },
    &operand_data[121],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:774 */
  {
    "cbranchsf4_insn_rev_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_38 },
#else
    { 0, 0, output_38 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsf4_insn_rev_68881 },
    &operand_data[101],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:774 */
  {
    "cbranchdf4_insn_rev_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_39 },
#else
    { 0, 0, output_39 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchdf4_insn_rev_68881 },
    &operand_data[105],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:774 */
  {
    "cbranchxf4_insn_rev_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_40 },
#else
    { 0, 0, output_40 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchxf4_insn_rev_68881 },
    &operand_data[109],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:792 */
  {
    "cbranchsf4_insn_rev_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_41 },
#else
    { 0, 0, output_41 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsf4_insn_rev_cf },
    &operand_data[113],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:792 */
  {
    "cbranchdf4_insn_rev_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_42 },
#else
    { 0, 0, output_42 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchdf4_insn_rev_cf },
    &operand_data[117],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:792 */
  {
    "cbranchxf4_insn_rev_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_43 },
#else
    { 0, 0, output_43 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchxf4_insn_rev_cf },
    &operand_data[121],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:810 */
  {
    "cstoresf4_insn_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_44 },
#else
    { 0, 0, output_44 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoresf4_insn_68881 },
    &operand_data[125],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:810 */
  {
    "cstoredf4_insn_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_45 },
#else
    { 0, 0, output_45 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoredf4_insn_68881 },
    &operand_data[129],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:810 */
  {
    "cstorexf4_insn_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_46 },
#else
    { 0, 0, output_46 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstorexf4_insn_68881 },
    &operand_data[133],
    4,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:827 */
  {
    "cstoresf4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_47 },
#else
    { 0, 0, output_47 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoresf4_insn_cf },
    &operand_data[137],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:827 */
  {
    "cstoredf4_insn_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_48 },
#else
    { 0, 0, output_48 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoredf4_insn_cf },
    &operand_data[141],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:844 */
  {
    "pushexthisi_const",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .multi = output_49 },
#else
    { 0, output_49, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_pushexthisi_const },
    &operand_data[145],
    2,
    2,
    0,
    3,
    2
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:864 */
  {
    "*movsi_const0_68000_10",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .multi = output_50 },
#else
    { 0, output_50, 0 },
#endif
    { 0 },
    &operand_data[147],
    1,
    1,
    0,
    3,
    2
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:877 */
  {
    "*movsi_const0_68040_60",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_51 },
#else
    { 0, 0, output_51 },
#endif
    { 0 },
    &operand_data[148],
    1,
    1,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:895 */
  {
    "*movsi_const0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .multi = output_52 },
#else
    { 0, output_52, 0 },
#endif
    { 0 },
    &operand_data[148],
    1,
    1,
    0,
    2,
    2
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:990 */
  {
    "*movsi_m68k",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_53 },
#else
    { 0, 0, output_53 },
#endif
    { 0 },
    &operand_data[149],
    2,
    2,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1004 */
  {
    "*movsi_m68k2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_54 },
#else
    { 0, 0, output_54 },
#endif
    { 0 },
    &operand_data[151],
    2,
    2,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1015 */
  {
    "*movsi_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_55 },
#else
    { 0, 0, output_55 },
#endif
    { 0 },
    &operand_data[153],
    2,
    2,
    0,
    12,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1068 */
  {
    "*m68k.md:1068",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_56 },
#else
    { 0, 0, output_56 },
#endif
    { 0 },
    &operand_data[155],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1084 */
  {
    "*m68k.md:1084",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_57 },
#else
    { 0, 0, output_57 },
#endif
    { 0 },
    &operand_data[157],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1094 */
  {
    "*m68k.md:1094",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_58 },
#else
    { 0, 0, output_58 },
#endif
    { 0 },
    &operand_data[159],
    2,
    2,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1110 */
  {
    "*m68k.md:1110",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_59 },
#else
    { 0, 0, output_59 },
#endif
    { 0 },
    &operand_data[161],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1117 */
  {
    "*m68k.md:1117",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_60 },
#else
    { 0, 0, output_60 },
#endif
    { 0 },
    &operand_data[163],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1130 */
  {
    "*m68k.md:1130",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_61 },
#else
    { 0, 0, output_61 },
#endif
    { 0 },
    &operand_data[165],
    2,
    2,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1137 */
  {
    "*m68k.md:1137",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_62 },
#else
    { 0, 0, output_62 },
#endif
    { 0 },
    &operand_data[167],
    2,
    2,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1150 */
  {
    "*m68k.md:1150",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_63 },
#else
    { 0, 0, output_63 },
#endif
    { 0 },
    &operand_data[169],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1157 */
  {
    "*movstrictqi_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .multi = output_64 },
#else
    { 0, output_64, 0 },
#endif
    { 0 },
    &operand_data[171],
    2,
    2,
    0,
    4,
    2
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1209 */
  {
    "*m68k.md:1209",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_65 },
#else
    { 0, 0, output_65 },
#endif
    { 0 },
    &operand_data[173],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1252 */
  {
    "movsf_cf_soft",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "move%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movsf_cf_soft },
    &operand_data[175],
    2,
    2,
    0,
    3,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1261 */
  {
    "movsf_cf_hard",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_67 },
#else
    { 0, 0, output_67 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movsf_cf_hard },
    &operand_data[177],
    2,
    2,
    0,
    8,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1355 */
  {
    "*m68k.md:1355",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_68 },
#else
    { 0, 0, output_68 },
#endif
    { 0 },
    &operand_data[179],
    2,
    2,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1393 */
  {
    "movdf_cf_soft",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movdf_cf_soft },
    &operand_data[181],
    2,
    2,
    0,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1405 */
  {
    "movdf_cf_hard",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_70 },
#else
    { 0, 0, output_70 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movdf_cf_hard },
    &operand_data[183],
    2,
    2,
    0,
    8,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1475 */
  {
    "*m68k.md:1475",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_71 },
#else
    { 0, 0, output_71 },
#endif
    { 0 },
    &operand_data[185],
    2,
    2,
    0,
    8,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1515 */
  {
    "*m68k.md:1515",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_72 },
#else
    { 0, 0, output_72 },
#endif
    { 0 },
    &operand_data[187],
    2,
    2,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1554 */
  {
    "*m68k.md:1554",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_73 },
#else
    { 0, 0, output_73 },
#endif
    { 0 },
    &operand_data[189],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1568 */
  {
    "*m68k.md:1568",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_74 },
#else
    { 0, 0, output_74 },
#endif
    { 0 },
    &operand_data[191],
    2,
    2,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1608 */
  {
    "*m68k.md:1608",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_75 },
#else
    { 0, 0, output_75 },
#endif
    { 0 },
    &operand_data[193],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1619 */
  {
    "pushasi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "pea %a1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_pushasi },
    &operand_data[195],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1627 */
  {
    "truncsiqi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_77 },
#else
    { 0, 0, output_77 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_truncsiqi2 },
    &operand_data[197],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1645 */
  {
    "trunchiqi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_78 },
#else
    { 0, 0, output_78 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_trunchiqi2 },
    &operand_data[199],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1668 */
  {
    "truncsihi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_79 },
#else
    { 0, 0, output_79 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_truncsihi2 },
    &operand_data[201],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1689 */
  {
    "*zero_extend_inc",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[203],
    2,
    2,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1705 */
  {
    "*zero_extend_dec",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[205],
    2,
    2,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1722 */
  {
    "zero_extendqidi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_zero_extendqidi2 },
    &operand_data[207],
    2,
    2,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1737 */
  {
    "zero_extendhidi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_zero_extendhidi2 },
    &operand_data[209],
    2,
    2,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1762 */
  {
    "*zero_extendsidi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[211],
    2,
    2,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1777 */
  {
    "*zero_extendhisi2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "mvz%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[213],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1784 */
  {
    "zero_extendhisi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_zero_extendhisi2 },
    &operand_data[213],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1796 */
  {
    "*zero_extendqihi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[215],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1802 */
  {
    "*zero_extendqisi2_cfv4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "mvz%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[217],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1809 */
  {
    "zero_extendqisi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_zero_extendqisi2 },
    &operand_data[217],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1847 */
  {
    "extendqidi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_90 },
#else
    { 0, 0, output_90 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendqidi2 },
    &operand_data[219],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1871 */
  {
    "extendhidi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_91 },
#else
    { 0, 0, output_91 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendhidi2 },
    &operand_data[221],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1886 */
  {
    "extendsidi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_92 },
#else
    { 0, 0, output_92 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendsidi2 },
    &operand_data[223],
    2,
    3,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1921 */
  {
    "extendplussidi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_93 },
#else
    { 0, 0, output_93 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendplussidi },
    &operand_data[226],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1954 */
  {
    "*cfv4_extendhisi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "mvs%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[229],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1962 */
  {
    "*68k_extendhisi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .multi = output_95 },
#else
    { 0, output_95, 0 },
#endif
    { 0 },
    &operand_data[231],
    2,
    2,
    0,
    2,
    2
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1972 */
  {
    "extendqihi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "ext%.w %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendqihi2 },
    &operand_data[233],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1985 */
  {
    "*cfv4_extendqisi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "mvs%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[235],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1992 */
  {
    "*68k_extendqisi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "extb%.l %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[237],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2008 */
  {
    "*m68k.md:2008",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_99 },
#else
    { 0, 0, output_99 },
#endif
    { 0 },
    &operand_data[239],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2034 */
  {
    "extendsfdf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_100 },
#else
    { 0, 0, output_100 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendsfdf2_cf },
    &operand_data[241],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2062 */
  {
    "*m68k.md:2062",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_101 },
#else
    { 0, 0, output_101 },
#endif
    { 0 },
    &operand_data[243],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2073 */
  {
    "truncdfsf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .multi = output_102 },
#else
    { 0, output_102, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_truncdfsf2_cf },
    &operand_data[245],
    2,
    2,
    0,
    2,
    2
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2083 */
  {
    "*truncdfsf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.s %f1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[247],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2105 */
  {
    "floatsisf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%$move%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatsisf2_68881 },
    &operand_data[249],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2105 */
  {
    "floatsidf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%&move%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatsidf2_68881 },
    &operand_data[251],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2105 */
  {
    "floatsixf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatsixf2_68881 },
    &operand_data[253],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2112 */
  {
    "floatsisf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fsmove%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatsisf2_cf },
    &operand_data[255],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2112 */
  {
    "floatsidf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fdmove%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatsidf2_cf },
    &operand_data[257],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2126 */
  {
    "floathisf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floathisf2_68881 },
    &operand_data[259],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2126 */
  {
    "floathidf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floathidf2_68881 },
    &operand_data[261],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2126 */
  {
    "floathixf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floathixf2_68881 },
    &operand_data[263],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2133 */
  {
    "floathisf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floathisf2_cf },
    &operand_data[265],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2133 */
  {
    "floathidf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floathidf2_cf },
    &operand_data[267],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2147 */
  {
    "floatqisf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatqisf2_68881 },
    &operand_data[269],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2147 */
  {
    "floatqidf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatqidf2_68881 },
    &operand_data[271],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2147 */
  {
    "floatqixf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatqixf2_68881 },
    &operand_data[273],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2154 */
  {
    "floatqisf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatqisf2_cf },
    &operand_data[275],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2154 */
  {
    "floatqidf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatqidf2_cf },
    &operand_data[277],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2167 */
  {
    "fix_truncdfsi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_119 },
#else
    { 0, 0, output_119 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fix_truncdfsi2 },
    &operand_data[279],
    2,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2177 */
  {
    "fix_truncdfhi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_120 },
#else
    { 0, 0, output_120 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fix_truncdfhi2 },
    &operand_data[283],
    2,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2187 */
  {
    "fix_truncdfqi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_121 },
#else
    { 0, 0, output_121 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fix_truncdfqi2 },
    &operand_data[287],
    2,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2206 */
  {
    "ftruncsf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_122 },
#else
    { 0, 0, output_122 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ftruncsf2_68881 },
    &operand_data[291],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2206 */
  {
    "ftruncdf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_123 },
#else
    { 0, 0, output_123 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ftruncdf2_68881 },
    &operand_data[293],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2206 */
  {
    "ftruncxf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_124 },
#else
    { 0, 0, output_124 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ftruncxf2_68881 },
    &operand_data[295],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2217 */
  {
    "ftruncsf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_125 },
#else
    { 0, 0, output_125 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ftruncsf2_cf },
    &operand_data[297],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2217 */
  {
    "ftruncdf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_126 },
#else
    { 0, 0, output_126 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ftruncdf2_cf },
    &operand_data[299],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2236 */
  {
    "fixsfqi2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixsfqi2_68881 },
    &operand_data[301],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2236 */
  {
    "fixdfqi2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixdfqi2_68881 },
    &operand_data[303],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2236 */
  {
    "fixxfqi2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixxfqi2_68881 },
    &operand_data[305],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2243 */
  {
    "fixsfqi2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixsfqi2_cf },
    &operand_data[307],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2243 */
  {
    "fixdfqi2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixdfqi2_cf },
    &operand_data[309],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2256 */
  {
    "fixsfhi2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixsfhi2_68881 },
    &operand_data[311],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2256 */
  {
    "fixdfhi2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixdfhi2_68881 },
    &operand_data[313],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2256 */
  {
    "fixxfhi2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixxfhi2_68881 },
    &operand_data[315],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2263 */
  {
    "fixsfhi2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixsfhi2_cf },
    &operand_data[317],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2263 */
  {
    "fixdfhi2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixdfhi2_cf },
    &operand_data[319],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2276 */
  {
    "fixsfsi2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixsfsi2_68881 },
    &operand_data[321],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2276 */
  {
    "fixdfsi2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixdfsi2_68881 },
    &operand_data[323],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2276 */
  {
    "fixxfsi2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixxfsi2_68881 },
    &operand_data[325],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2283 */
  {
    "fixsfsi2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixsfsi2_cf },
    &operand_data[327],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2283 */
  {
    "fixdfsi2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixdfsi2_cf },
    &operand_data[329],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2293 */
  {
    "adddi_lshrdi_63",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_142 },
#else
    { 0, 0, output_142 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddi_lshrdi_63 },
    &operand_data[331],
    2,
    3,
    1,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2326 */
  {
    "adddi_sexthishl32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_143 },
#else
    { 0, 0, output_143 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddi_sexthishl32 },
    &operand_data[334],
    3,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2343 */
  {
    "*adddi_dilshr32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_144 },
#else
    { 0, 0, output_144 },
#endif
    { 0 },
    &operand_data[338],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2357 */
  {
    "*adddi_dilshr32_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_145 },
#else
    { 0, 0, output_145 },
#endif
    { 0 },
    &operand_data[341],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2367 */
  {
    "adddi_dishl32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_146 },
#else
    { 0, 0, output_146 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddi_dishl32 },
    &operand_data[344],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2385 */
  {
    "adddi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_147 },
#else
    { 0, 0, output_147 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddi3 },
    &operand_data[347],
    3,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2466 */
  {
    "addsi_lshrsi_31",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_148 },
#else
    { 0, 0, output_148 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addsi_lshrsi_31 },
    &operand_data[351],
    2,
    2,
    1,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2502 */
  {
    "*addsi3_internal",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_149 },
#else
    { 0, 0, output_149 },
#endif
    { 0 },
    &operand_data[353],
    3,
    3,
    0,
    5,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2512 */
  {
    "*addsi3_5200",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_150 },
#else
    { 0, 0, output_150 },
#endif
    { 0 },
    &operand_data[356],
    3,
    3,
    0,
    9,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2561 */
  {
    "*m68k.md:2561",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "add%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[359],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2569 */
  {
    "addhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_152 },
#else
    { 0, 0, output_152 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addhi3 },
    &operand_data[362],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2628 */
  {
    "*m68k.md:2628",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_153 },
#else
    { 0, 0, output_153 },
#endif
    { 0 },
    &operand_data[365],
    2,
    2,
    1,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2677 */
  {
    "*m68k.md:2677",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_154 },
#else
    { 0, 0, output_154 },
#endif
    { 0 },
    &operand_data[365],
    2,
    2,
    1,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2726 */
  {
    "addqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_155 },
#else
    { 0, 0, output_155 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addqi3 },
    &operand_data[367],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2750 */
  {
    "*m68k.md:2750",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_156 },
#else
    { 0, 0, output_156 },
#endif
    { 0 },
    &operand_data[370],
    2,
    2,
    1,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2773 */
  {
    "*m68k.md:2773",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_157 },
#else
    { 0, 0, output_157 },
#endif
    { 0 },
    &operand_data[370],
    2,
    2,
    1,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2803 */
  {
    "addsf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%$add%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addsf3_floatsi_68881 },
    &operand_data[372],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2803 */
  {
    "adddf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%&add%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddf3_floatsi_68881 },
    &operand_data[375],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2803 */
  {
    "addxf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fadd%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addxf3_floatsi_68881 },
    &operand_data[378],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2812 */
  {
    "addsf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%$add%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addsf3_floathi_68881 },
    &operand_data[381],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2812 */
  {
    "adddf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%&add%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddf3_floathi_68881 },
    &operand_data[384],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2812 */
  {
    "addxf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fadd%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addxf3_floathi_68881 },
    &operand_data[387],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2821 */
  {
    "addsf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%$add%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addsf3_floatqi_68881 },
    &operand_data[390],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2821 */
  {
    "adddf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%&add%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddf3_floatqi_68881 },
    &operand_data[393],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2821 */
  {
    "addxf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fadd%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addxf3_floatqi_68881 },
    &operand_data[396],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2830 */
  {
    "addsf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_167 },
#else
    { 0, 0, output_167 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addsf3_68881 },
    &operand_data[399],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2830 */
  {
    "adddf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_168 },
#else
    { 0, 0, output_168 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddf3_68881 },
    &operand_data[402],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2830 */
  {
    "addxf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_169 },
#else
    { 0, 0, output_169 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addxf3_68881 },
    &operand_data[405],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2843 */
  {
    "addsf3_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_170 },
#else
    { 0, 0, output_170 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addsf3_cf },
    &operand_data[408],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2843 */
  {
    "adddf3_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_171 },
#else
    { 0, 0, output_171 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddf3_cf },
    &operand_data[411],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2858 */
  {
    "subdi_sexthishl32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_172 },
#else
    { 0, 0, output_172 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subdi_sexthishl32 },
    &operand_data[414],
    3,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2874 */
  {
    "subdi_dishl32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_173 },
#else
    { 0, 0, output_173 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subdi_dishl32 },
    &operand_data[418],
    2,
    2,
    1,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2889 */
  {
    "subdi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_174 },
#else
    { 0, 0, output_174 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subdi3 },
    &operand_data[420],
    3,
    4,
    0,
    4,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2972 */
  {
    "subsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .multi = output_175 },
#else
    { 0, output_175, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subsi3 },
    &operand_data[424],
    3,
    3,
    0,
    5,
    2
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2987 */
  {
    "*m68k.md:2987",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "sub%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[359],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2995 */
  {
    "subhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "sub%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subhi3 },
    &operand_data[427],
    3,
    3,
    0,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3003 */
  {
    "*m68k.md:3003",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "sub%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[365],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3011 */
  {
    "subqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "sub%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subqi3 },
    &operand_data[430],
    3,
    3,
    0,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3019 */
  {
    "*m68k.md:3019",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "sub%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[370],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3034 */
  {
    "subsf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%$sub%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subsf3_floatsi_68881 },
    &operand_data[372],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3034 */
  {
    "subdf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%&sub%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subdf3_floatsi_68881 },
    &operand_data[375],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3034 */
  {
    "subxf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fsub%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subxf3_floatsi_68881 },
    &operand_data[378],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3043 */
  {
    "subsf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%$sub%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subsf3_floathi_68881 },
    &operand_data[381],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3043 */
  {
    "subdf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%&sub%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subdf3_floathi_68881 },
    &operand_data[384],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3043 */
  {
    "subxf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fsub%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subxf3_floathi_68881 },
    &operand_data[387],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3052 */
  {
    "subsf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%$sub%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subsf3_floatqi_68881 },
    &operand_data[390],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3052 */
  {
    "subdf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "f%&sub%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subdf3_floatqi_68881 },
    &operand_data[393],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3052 */
  {
    "subxf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fsub%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subxf3_floatqi_68881 },
    &operand_data[396],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3061 */
  {
    "subsf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_190 },
#else
    { 0, 0, output_190 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subsf3_68881 },
    &operand_data[433],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3061 */
  {
    "subdf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_191 },
#else
    { 0, 0, output_191 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subdf3_68881 },
    &operand_data[436],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3061 */
  {
    "subxf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_192 },
#else
    { 0, 0, output_192 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subxf3_68881 },
    &operand_data[439],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3074 */
  {
    "subsf3_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_193 },
#else
    { 0, 0, output_193 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subsf3_cf },
    &operand_data[442],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3074 */
  {
    "subdf3_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_194 },
#else
    { 0, 0, output_194 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subdf3_cf },
    &operand_data[445],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3089 */
  {
    "mulhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_195 },
#else
    { 0, 0, output_195 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulhi3 },
    &operand_data[448],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3100 */
  {
    "mulhisi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_196 },
#else
    { 0, 0, output_196 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulhisi3 },
    &operand_data[451],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3113 */
  {
    "*mulhisisi3_s",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_197 },
#else
    { 0, 0, output_197 },
#endif
    { 0 },
    &operand_data[454],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3132 */
  {
    "*mulsi3_68020",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "muls%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[457],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3142 */
  {
    "*mulsi3_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "muls%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[460],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3151 */
  {
    "umulhisi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_200 },
#else
    { 0, 0, output_200 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_umulhisi3 },
    &operand_data[451],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3164 */
  {
    "*mulhisisi3_z",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_201 },
#else
    { 0, 0, output_201 },
#endif
    { 0 },
    &operand_data[454],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3191 */
  {
    "*m68k.md:3191",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "mulu%.l %2,%3:%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[463],
    4,
    4,
    2,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3206 */
  {
    "*m68k.md:3206",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "mulu%.l %2,%3:%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[466],
    4,
    4,
    2,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3230 */
  {
    "*m68k.md:3230",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "muls%.l %2,%3:%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[463],
    4,
    4,
    2,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3241 */
  {
    "*m68k.md:3241",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "muls%.l %2,%3:%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[466],
    4,
    4,
    2,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3277 */
  {
    "*m68k.md:3277",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "mulu%.l %3,%0:%1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[469],
    4,
    4,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3288 */
  {
    "const_umulsi3_highpart",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "mulu%.l %3,%0:%1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_const_umulsi3_highpart },
    &operand_data[473],
    4,
    4,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3320 */
  {
    "*m68k.md:3320",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "muls%.l %3,%0:%1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[469],
    4,
    4,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3331 */
  {
    "const_smulsi3_highpart",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "muls%.l %3,%0:%1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_const_smulsi3_highpart },
    &operand_data[477],
    4,
    4,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3349 */
  {
    "mulsf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_210 },
#else
    { 0, 0, output_210 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulsf3_floatsi_68881 },
    &operand_data[372],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3349 */
  {
    "muldf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_211 },
#else
    { 0, 0, output_211 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_muldf3_floatsi_68881 },
    &operand_data[375],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3349 */
  {
    "mulxf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_212 },
#else
    { 0, 0, output_212 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulxf3_floatsi_68881 },
    &operand_data[378],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3362 */
  {
    "mulsf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_213 },
#else
    { 0, 0, output_213 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulsf3_floathi_68881 },
    &operand_data[381],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3362 */
  {
    "muldf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_214 },
#else
    { 0, 0, output_214 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_muldf3_floathi_68881 },
    &operand_data[384],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3362 */
  {
    "mulxf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_215 },
#else
    { 0, 0, output_215 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulxf3_floathi_68881 },
    &operand_data[387],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3375 */
  {
    "mulsf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_216 },
#else
    { 0, 0, output_216 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulsf3_floatqi_68881 },
    &operand_data[390],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3375 */
  {
    "muldf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_217 },
#else
    { 0, 0, output_217 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_muldf3_floatqi_68881 },
    &operand_data[393],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3375 */
  {
    "mulxf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_218 },
#else
    { 0, 0, output_218 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulxf3_floatqi_68881 },
    &operand_data[396],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3388 */
  {
    "muldf_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_219 },
#else
    { 0, 0, output_219 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_muldf_68881 },
    &operand_data[402],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3406 */
  {
    "mulsf_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_220 },
#else
    { 0, 0, output_220 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulsf_68881 },
    &operand_data[399],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3421 */
  {
    "mulxf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_221 },
#else
    { 0, 0, output_221 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulxf3_68881 },
    &operand_data[481],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3430 */
  {
    "fmulsf3_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_222 },
#else
    { 0, 0, output_222 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fmulsf3_cf },
    &operand_data[484],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3430 */
  {
    "fmuldf3_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_223 },
#else
    { 0, 0, output_223 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fmuldf3_cf },
    &operand_data[411],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3452 */
  {
    "divsf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_224 },
#else
    { 0, 0, output_224 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divsf3_floatsi_68881 },
    &operand_data[372],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3452 */
  {
    "divdf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_225 },
#else
    { 0, 0, output_225 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divdf3_floatsi_68881 },
    &operand_data[375],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3452 */
  {
    "divxf3_floatsi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_226 },
#else
    { 0, 0, output_226 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divxf3_floatsi_68881 },
    &operand_data[378],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3463 */
  {
    "divsf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_227 },
#else
    { 0, 0, output_227 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divsf3_floathi_68881 },
    &operand_data[381],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3463 */
  {
    "divdf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_228 },
#else
    { 0, 0, output_228 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divdf3_floathi_68881 },
    &operand_data[384],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3463 */
  {
    "divxf3_floathi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_229 },
#else
    { 0, 0, output_229 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divxf3_floathi_68881 },
    &operand_data[387],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3474 */
  {
    "divsf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_230 },
#else
    { 0, 0, output_230 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divsf3_floatqi_68881 },
    &operand_data[390],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3474 */
  {
    "divdf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_231 },
#else
    { 0, 0, output_231 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divdf3_floatqi_68881 },
    &operand_data[393],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3474 */
  {
    "divxf3_floatqi_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_232 },
#else
    { 0, 0, output_232 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divxf3_floatqi_68881 },
    &operand_data[396],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3485 */
  {
    "divsf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_233 },
#else
    { 0, 0, output_233 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divsf3_68881 },
    &operand_data[433],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3485 */
  {
    "divdf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_234 },
#else
    { 0, 0, output_234 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divdf3_68881 },
    &operand_data[436],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3485 */
  {
    "divxf3_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_235 },
#else
    { 0, 0, output_235 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divxf3_68881 },
    &operand_data[439],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3500 */
  {
    "divsf3_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_236 },
#else
    { 0, 0, output_236 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divsf3_cf },
    &operand_data[487],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3500 */
  {
    "divdf3_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_237 },
#else
    { 0, 0, output_237 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divdf3_cf },
    &operand_data[445],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3525 */
  {
    "*m68k.md:3525",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_238 },
#else
    { 0, 0, output_238 },
#endif
    { 0 },
    &operand_data[490],
    4,
    4,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3543 */
  {
    "*m68k.md:3543",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_239 },
#else
    { 0, 0, output_239 },
#endif
    { 0 },
    &operand_data[494],
    4,
    4,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3567 */
  {
    "*m68k.md:3567",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_240 },
#else
    { 0, 0, output_240 },
#endif
    { 0 },
    &operand_data[490],
    4,
    4,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3585 */
  {
    "*m68k.md:3585",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_241 },
#else
    { 0, 0, output_241 },
#endif
    { 0 },
    &operand_data[494],
    4,
    4,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3599 */
  {
    "divmodhi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_242 },
#else
    { 0, 0, output_242 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divmodhi4 },
    &operand_data[498],
    4,
    4,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3617 */
  {
    "udivmodhi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_243 },
#else
    { 0, 0, output_243 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_udivmodhi4 },
    &operand_data[498],
    4,
    4,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3656 */
  {
    "*andsi3_split",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_244 },
#else
    { 0, 0, output_244 },
#endif
    { 0 },
    &operand_data[502],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3665 */
  {
    "andsi3_internal",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_245 },
#else
    { 0, 0, output_245 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_andsi3_internal },
    &operand_data[505],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3675 */
  {
    "andsi3_5200",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_246 },
#else
    { 0, 0, output_246 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_andsi3_5200 },
    &operand_data[508],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3693 */
  {
    "andhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "and%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_andhi3 },
    &operand_data[511],
    3,
    3,
    0,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3701 */
  {
    "*m68k.md:3701",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "and%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[514],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3709 */
  {
    "*m68k.md:3709",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "and%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[514],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3717 */
  {
    "andqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "and%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_andqi3 },
    &operand_data[367],
    3,
    3,
    0,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3725 */
  {
    "*m68k.md:3725",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "and%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[370],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3733 */
  {
    "*m68k.md:3733",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "and%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[370],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3742 */
  {
    "iordi_zext",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_253 },
#else
    { 0, 0, output_253 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_iordi_zext },
    &operand_data[516],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3773 */
  {
    "iorsi3_internal",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_254 },
#else
    { 0, 0, output_254 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_iorsi3_internal },
    &operand_data[519],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3783 */
  {
    "iorsi3_5200",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_255 },
#else
    { 0, 0, output_255 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_iorsi3_5200 },
    &operand_data[522],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3793 */
  {
    "iorhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "or%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_iorhi3 },
    &operand_data[511],
    3,
    3,
    0,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3800 */
  {
    "*m68k.md:3800",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "or%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[514],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3807 */
  {
    "*m68k.md:3807",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "or%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[514],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3815 */
  {
    "iorqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "or%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_iorqi3 },
    &operand_data[367],
    3,
    3,
    0,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3823 */
  {
    "*m68k.md:3823",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "or%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[370],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3831 */
  {
    "*m68k.md:3831",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "or%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[370],
    2,
    2,
    1,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3842 */
  {
    "iorsi_zexthi_ashl16",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_262 },
#else
    { 0, 0, output_262 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_iorsi_zexthi_ashl16 },
    &operand_data[525],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3857 */
  {
    "iorsi_zext",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_263 },
#else
    { 0, 0, output_263 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_iorsi_zext },
    &operand_data[528],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3884 */
  {
    "xorsi3_internal",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_264 },
#else
    { 0, 0, output_264 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_xorsi3_internal },
    &operand_data[531],
    3,
    3,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3895 */
  {
    "xorsi3_5200",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_265 },
#else
    { 0, 0, output_265 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_xorsi3_5200 },
    &operand_data[534],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3905 */
  {
    "xorhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "eor%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_xorhi3 },
    &operand_data[537],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3913 */
  {
    "*m68k.md:3913",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "eor%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[540],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3921 */
  {
    "*m68k.md:3921",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "eor%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[540],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3929 */
  {
    "xorqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "eor%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_xorqi3 },
    &operand_data[542],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3937 */
  {
    "*m68k.md:3937",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "eor%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[545],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3945 */
  {
    "*m68k.md:3945",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "eor%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[545],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3967 */
  {
    "negdi2_internal",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_272 },
#else
    { 0, 0, output_272 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negdi2_internal },
    &operand_data[547],
    2,
    2,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3984 */
  {
    "negdi2_5200",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_273 },
#else
    { 0, 0, output_273 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negdi2_5200 },
    &operand_data[549],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4005 */
  {
    "negsi2_internal",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "neg%.l %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negsi2_internal },
    &operand_data[551],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4013 */
  {
    "negsi2_5200",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "neg%.l %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negsi2_5200 },
    &operand_data[553],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4021 */
  {
    "neghi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "neg%.w %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_neghi2 },
    &operand_data[555],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4028 */
  {
    "*m68k.md:4028",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "neg%.w %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[161],
    1,
    1,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4035 */
  {
    "negqi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "neg%.b %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negqi2 },
    &operand_data[557],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4042 */
  {
    "*m68k.md:4042",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "neg%.b %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[169],
    1,
    1,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4142 */
  {
    "negsf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_280 },
#else
    { 0, 0, output_280 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negsf2_68881 },
    &operand_data[559],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4142 */
  {
    "negdf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_281 },
#else
    { 0, 0, output_281 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negdf2_68881 },
    &operand_data[561],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4142 */
  {
    "negxf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_282 },
#else
    { 0, 0, output_282 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negxf2_68881 },
    &operand_data[563],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4157 */
  {
    "negsf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_283 },
#else
    { 0, 0, output_283 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negsf2_cf },
    &operand_data[565],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4157 */
  {
    "negdf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_284 },
#else
    { 0, 0, output_284 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negdf2_cf },
    &operand_data[567],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4180 */
  {
    "sqrtsf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_285 },
#else
    { 0, 0, output_285 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sqrtsf2_68881 },
    &operand_data[291],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4180 */
  {
    "sqrtdf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_286 },
#else
    { 0, 0, output_286 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sqrtdf2_68881 },
    &operand_data[293],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4180 */
  {
    "sqrtxf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_287 },
#else
    { 0, 0, output_287 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sqrtxf2_68881 },
    &operand_data[295],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4191 */
  {
    "sqrtsf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_288 },
#else
    { 0, 0, output_288 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sqrtsf2_cf },
    &operand_data[297],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4191 */
  {
    "sqrtdf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_289 },
#else
    { 0, 0, output_289 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sqrtdf2_cf },
    &operand_data[299],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4295 */
  {
    "abssf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_290 },
#else
    { 0, 0, output_290 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_abssf2_68881 },
    &operand_data[559],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4295 */
  {
    "absdf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_291 },
#else
    { 0, 0, output_291 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_absdf2_68881 },
    &operand_data[561],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4295 */
  {
    "absxf2_68881",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_292 },
#else
    { 0, 0, output_292 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_absxf2_68881 },
    &operand_data[563],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4310 */
  {
    "abssf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_293 },
#else
    { 0, 0, output_293 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_abssf2_cf },
    &operand_data[565],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4310 */
  {
    "absdf2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_294 },
#else
    { 0, 0, output_294 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_absdf2_cf },
    &operand_data[567],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4337 */
  {
    "*clzsi2_68k",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bfffo %1{#0:#0},%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[569],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4344 */
  {
    "*clzsi2_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "ff1 %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[502],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4365 */
  {
    "one_cmplsi2_internal",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "not%.l %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_one_cmplsi2_internal },
    &operand_data[551],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4372 */
  {
    "one_cmplsi2_5200",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "not%.l %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_one_cmplsi2_5200 },
    &operand_data[553],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4379 */
  {
    "one_cmplhi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "not%.w %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_one_cmplhi2 },
    &operand_data[555],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4386 */
  {
    "*m68k.md:4386",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "not%.w %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[161],
    1,
    1,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4393 */
  {
    "one_cmplqi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "not%.b %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_one_cmplqi2 },
    &operand_data[557],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4400 */
  {
    "*m68k.md:4400",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "not%.b %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[169],
    1,
    1,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4409 */
  {
    "ashldi_extsi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashldi_extsi },
    &operand_data[571],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4422 */
  {
    "ashldi_sexthi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_304 },
#else
    { 0, 0, output_304 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashldi_sexthi },
    &operand_data[574],
    2,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4447 */
  {
    "*ashldi3_const1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "add%.l %R0,%R0\n\taddx%.l %0,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[577],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4536 */
  {
    "*ashldi3_const32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[579],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4594 */
  {
    "*ashldi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[581],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4621 */
  {
    "ashlsi_16",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "swap %0\n\tclr%.w %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashlsi_16 },
    &operand_data[502],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4633 */
  {
    "ashlsi_17_24",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_309 },
#else
    { 0, 0, output_309 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashlsi_17_24 },
    &operand_data[584],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4645 */
  {
    "ashlsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_310 },
#else
    { 0, 0, output_310 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashlsi3 },
    &operand_data[587],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4660 */
  {
    "ashlhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lsl%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashlhi3 },
    &operand_data[590],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4668 */
  {
    "*m68k.md:4668",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lsl%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[593],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4676 */
  {
    "ashlqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lsl%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashlqi3 },
    &operand_data[595],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4684 */
  {
    "*m68k.md:4684",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lsl%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[598],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4694 */
  {
    "ashrsi_16",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "swap %0\n\text%.l %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashrsi_16 },
    &operand_data[502],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4703 */
  {
    "*m68k.md:4703",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_316 },
#else
    { 0, 0, output_316 },
#endif
    { 0 },
    &operand_data[584],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4715 */
  {
    "subreghi1ashrdi_const32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_317 },
#else
    { 0, 0, output_317 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subreghi1ashrdi_const32 },
    &operand_data[600],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4727 */
  {
    "subregsi1ashrdi_const32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_318 },
#else
    { 0, 0, output_318 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subregsi1ashrdi_const32 },
    &operand_data[602],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4737 */
  {
    "*ashrdi3_const1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_319 },
#else
    { 0, 0, output_319 },
#endif
    { 0 },
    &operand_data[577],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4803 */
  {
    "*ashrdi_const32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_320 },
#else
    { 0, 0, output_320 },
#endif
    { 0 },
    &operand_data[604],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4815 */
  {
    "*ashrdi_const32_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_321 },
#else
    { 0, 0, output_321 },
#endif
    { 0 },
    &operand_data[606],
    2,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4843 */
  {
    "ashrdi_const",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_322 },
#else
    { 0, 0, output_322 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashrdi_const },
    &operand_data[581],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4887 */
  {
    "ashrsi_31",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_323 },
#else
    { 0, 0, output_323 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashrsi_31 },
    &operand_data[502],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4896 */
  {
    "ashrsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "asr%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashrsi3 },
    &operand_data[587],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4906 */
  {
    "ashrhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "asr%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashrhi3 },
    &operand_data[590],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4914 */
  {
    "*m68k.md:4914",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "asr%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[593],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4922 */
  {
    "ashrqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "asr%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashrqi3 },
    &operand_data[595],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4930 */
  {
    "*m68k.md:4930",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "asr%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[598],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4969 */
  {
    "subreg1lshrdi_const32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "move%.l %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subreg1lshrdi_const32 },
    &operand_data[602],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4977 */
  {
    "*lshrdi3_const1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_330 },
#else
    { 0, 0, output_330 },
#endif
    { 0 },
    &operand_data[577],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5075 */
  {
    "*lshrdi_const32",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[609],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5129 */
  {
    "*lshrdi_const63",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "add%.l %0,%0\n\tclr%.l %0\n\tclr%.l %R1\n\taddx%.l %R1,%R1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[577],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5136 */
  {
    "*lshrdi3_const",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[581],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5163 */
  {
    "lshrsi_31",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_334 },
#else
    { 0, 0, output_334 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_lshrsi_31 },
    &operand_data[502],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5174 */
  {
    "lshrsi_16",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_335 },
#else
    { 0, 0, output_335 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_lshrsi_16 },
    &operand_data[502],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5185 */
  {
    "lshrsi_17_24",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_336 },
#else
    { 0, 0, output_336 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_lshrsi_17_24 },
    &operand_data[584],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5198 */
  {
    "lshrsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lsr%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_lshrsi3 },
    &operand_data[587],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5208 */
  {
    "lshrhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lsr%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_lshrhi3 },
    &operand_data[590],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5216 */
  {
    "*m68k.md:5216",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lsr%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[593],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5224 */
  {
    "lshrqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lsr%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_lshrqi3 },
    &operand_data[595],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5232 */
  {
    "*m68k.md:5232",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lsr%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[598],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5242 */
  {
    "rotlsi_16",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "swap %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_rotlsi_16 },
    &operand_data[502],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5251 */
  {
    "rotlsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_343 },
#else
    { 0, 0, output_343 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_rotlsi3 },
    &operand_data[611],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5269 */
  {
    "rotlhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_344 },
#else
    { 0, 0, output_344 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_rotlhi3 },
    &operand_data[614],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5285 */
  {
    "*rotlhi3_lowpart",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_345 },
#else
    { 0, 0, output_345 },
#endif
    { 0 },
    &operand_data[617],
    2,
    2,
    1,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5301 */
  {
    "rotlqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_346 },
#else
    { 0, 0, output_346 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_rotlqi3 },
    &operand_data[595],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5317 */
  {
    "*rotlqi3_lowpart",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_347 },
#else
    { 0, 0, output_347 },
#endif
    { 0 },
    &operand_data[598],
    2,
    2,
    1,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5333 */
  {
    "rotrsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "ror%.l %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_rotrsi3 },
    &operand_data[587],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5341 */
  {
    "rotrhi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "ror%.w %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_rotrhi3 },
    &operand_data[590],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5348 */
  {
    "rotrhi_lowpart",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "ror%.w %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_rotrhi_lowpart },
    &operand_data[593],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5355 */
  {
    "rotrqi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "ror%.b %2,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_rotrqi3 },
    &operand_data[595],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5363 */
  {
    "*m68k.md:5363",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "ror%.b %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[598],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5388 */
  {
    "bsetmemqi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bset %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_bsetmemqi },
    &operand_data[619],
    2,
    2,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5398 */
  {
    "*bsetmemqi_ext",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bset %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[621],
    3,
    3,
    1,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5408 */
  {
    "*bsetdreg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bset %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[624],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5418 */
  {
    "*bchgdreg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bchg %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[624],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5428 */
  {
    "*bclrdreg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bclr %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[624],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5439 */
  {
    "bclrmemqi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bclr %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_bclrmemqi },
    &operand_data[619],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5450 */
  {
    "*bclrmemqi_ext",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bclr %1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[621],
    3,
    3,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5473 */
  {
    "*insv_32_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_360 },
#else
    { 0, 0, output_360 },
#endif
    { 0 },
    &operand_data[627],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5489 */
  {
    "*insv_8_16_reg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_361 },
#else
    { 0, 0, output_361 },
#endif
    { 0 },
    &operand_data[630],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5515 */
  {
    "*extzv_32_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_362 },
#else
    { 0, 0, output_362 },
#endif
    { 0 },
    &operand_data[634],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5531 */
  {
    "*extzv_8_16_reg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_363 },
#else
    { 0, 0, output_363 },
#endif
    { 0 },
    &operand_data[637],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5557 */
  {
    "*extv_32_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_364 },
#else
    { 0, 0, output_364 },
#endif
    { 0 },
    &operand_data[634],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5573 */
  {
    "*extv_8_16_reg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_365 },
#else
    { 0, 0, output_365 },
#endif
    { 0 },
    &operand_data[641],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5603 */
  {
    "*extv_bfexts_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bfexts %1{%b3:%b2},%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[645],
    4,
    4,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5619 */
  {
    "*extzv_bfextu_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_367 },
#else
    { 0, 0, output_367 },
#endif
    { 0 },
    &operand_data[645],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5629 */
  {
    "*insv_bfchg_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_368 },
#else
    { 0, 0, output_368 },
#endif
    { 0 },
    &operand_data[649],
    4,
    4,
    3,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5643 */
  {
    "*insv_bfclr_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_369 },
#else
    { 0, 0, output_369 },
#endif
    { 0 },
    &operand_data[649],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5653 */
  {
    "*insv_bfset_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_370 },
#else
    { 0, 0, output_370 },
#endif
    { 0 },
    &operand_data[653],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5682 */
  {
    "*insv_bfins_mem",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bfins %3,%0{%b2:%b1}",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[656],
    4,
    4,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5693 */
  {
    "*extv_bfexts_reg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "bfexts %1{%b3:%b2},%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[641],
    4,
    4,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5701 */
  {
    "*extv_bfextu_reg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_373 },
#else
    { 0, 0, output_373 },
#endif
    { 0 },
    &operand_data[641],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5711 */
  {
    "*insv_bfclr_reg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_374 },
#else
    { 0, 0, output_374 },
#endif
    { 0 },
    &operand_data[630],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5721 */
  {
    "*insv_bfset_reg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_375 },
#else
    { 0, 0, output_375 },
#endif
    { 0 },
    &operand_data[630],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5731 */
  {
    "*insv_bfins_reg",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_376 },
#else
    { 0, 0, output_376 },
#endif
    { 0 },
    &operand_data[630],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5750 */
  {
    "scc0_di",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_377 },
#else
    { 0, 0, output_377 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_scc0_di },
    &operand_data[660],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5759 */
  {
    "scc0_di_5200",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_378 },
#else
    { 0, 0, output_378 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_scc0_di_5200 },
    &operand_data[663],
    3,
    3,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5768 */
  {
    "scc_di",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_379 },
#else
    { 0, 0, output_379 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_scc_di },
    &operand_data[666],
    4,
    4,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5778 */
  {
    "scc_di_5200",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_380 },
#else
    { 0, 0, output_380 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_scc_di_5200 },
    &operand_data[670],
    4,
    4,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5789 */
  {
    "jump",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "jra %l0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_jump },
    &operand_data[15],
    1,
    1,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5810 */
  {
    "*tablejump_internal",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_382 },
#else
    { 0, 0, output_382 },
#endif
    { 0 },
    &operand_data[674],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5820 */
  {
    "*tablejump_pcrel_si",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_383 },
#else
    { 0, 0, output_383 },
#endif
    { 0 },
    &operand_data[676],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5834 */
  {
    "*tablejump_pcrel_hi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_384 },
#else
    { 0, 0, output_384 },
#endif
    { 0 },
    &operand_data[678],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5859 */
  {
    "*dbne_hi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_385 },
#else
    { 0, 0, output_385 },
#endif
    { 0 },
    &operand_data[680],
    2,
    2,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5878 */
  {
    "*dbne_si",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_386 },
#else
    { 0, 0, output_386 },
#endif
    { 0 },
    &operand_data[682],
    2,
    2,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5899 */
  {
    "*dbge_hi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_387 },
#else
    { 0, 0, output_387 },
#endif
    { 0 },
    &operand_data[684],
    2,
    2,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5933 */
  {
    "*dbge_si",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_388 },
#else
    { 0, 0, output_388 },
#endif
    { 0 },
    &operand_data[686],
    2,
    2,
    2,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5961 */
  {
    "*sibcall",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_389 },
#else
    { 0, 0, output_389 },
#endif
    { 0 },
    &operand_data[688],
    2,
    2,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5978 */
  {
    "*sibcall_value",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_390 },
#else
    { 0, 0, output_390 },
#endif
    { 0 },
    &operand_data[690],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5998 */
  {
    "*call",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_391 },
#else
    { 0, 0, output_391 },
#endif
    { 0 },
    &operand_data[693],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6020 */
  {
    "*non_symbolic_call_value",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "jsr %a1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[695],
    3,
    3,
    0,
    2,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6030 */
  {
    "*symbolic_call_value_jsr",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_393 },
#else
    { 0, 0, output_393 },
#endif
    { 0 },
    &operand_data[698],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6043 */
  {
    "*symbolic_call_value_bsr",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_394 },
#else
    { 0, 0, output_394 },
#endif
    { 0 },
    &operand_data[698],
    3,
    3,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6089 */
  {
    "blockage",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_blockage },
    &operand_data[0],
    0,
    0,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6095 */
  {
    "nop",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "nop",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_nop },
    &operand_data[0],
    0,
    0,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6132 */
  {
    "*return",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_397 },
#else
    { 0, 0, output_397 },
#endif
    { 0 },
    &operand_data[0],
    0,
    0,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6156 */
  {
    "*m68k_store_multiple",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_398 },
#else
    { 0, 0, output_398 },
#endif
    { 0 },
    &operand_data[701],
    2,
    2,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6163 */
  {
    "*m68k_store_multiple_automod",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_399 },
#else
    { 0, 0, output_399 },
#endif
    { 0 },
    &operand_data[703],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6173 */
  {
    "*m68k_load_multiple",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_400 },
#else
    { 0, 0, output_400 },
#endif
    { 0 },
    &operand_data[701],
    2,
    2,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6180 */
  {
    "*m68k_load_multiple_automod",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_401 },
#else
    { 0, 0, output_401 },
#endif
    { 0 },
    &operand_data[703],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6207 */
  {
    "*link",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_402 },
#else
    { 0, 0, output_402 },
#endif
    { 0 },
    &operand_data[707],
    2,
    2,
    1,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6239 */
  {
    "*unlink",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "unlk %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[707],
    1,
    1,
    2,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6249 */
  {
    "load_got",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_404 },
#else
    { 0, 0, output_404 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_load_got },
    &operand_data[704],
    1,
    1,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6276 */
  {
    "indirect_jump",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "jmp %a0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_indirect_jump },
    &operand_data[196],
    1,
    1,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6284 */
  {
    "*lea",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "lea %a1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { 0 },
    &operand_data[709],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6520 */
  {
    "extendsfxf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_407 },
#else
    { 0, 0, output_407 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendsfxf2 },
    &operand_data[711],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6548 */
  {
    "extenddfxf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_408 },
#else
    { 0, 0, output_408 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extenddfxf2 },
    &operand_data[713],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6580 */
  {
    "truncxfdf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_409 },
#else
    { 0, 0, output_409 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_truncxfdf2 },
    &operand_data[715],
    2,
    2,
    0,
    2,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6595 */
  {
    "truncxfsf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "fmove%.s %f1,%0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_truncxfsf2 },
    &operand_data[717],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6602 */
  {
    "sinsf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_411 },
#else
    { 0, 0, output_411 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sinsf2 },
    &operand_data[291],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6602 */
  {
    "sindf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_412 },
#else
    { 0, 0, output_412 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sindf2 },
    &operand_data[293],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6602 */
  {
    "sinxf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_413 },
#else
    { 0, 0, output_413 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sinxf2 },
    &operand_data[295],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6614 */
  {
    "cossf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_414 },
#else
    { 0, 0, output_414 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cossf2 },
    &operand_data[291],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6614 */
  {
    "cosdf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_415 },
#else
    { 0, 0, output_415 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cosdf2 },
    &operand_data[293],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6614 */
  {
    "cosxf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_416 },
#else
    { 0, 0, output_416 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cosxf2 },
    &operand_data[295],
    2,
    2,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6627 */
  {
    "trap",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "trap #7",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_trap },
    &operand_data[0],
    0,
    0,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6637 */
  {
    "ctrapqi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_418 },
#else
    { 0, 0, output_418 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ctrapqi4 },
    &operand_data[719],
    4,
    4,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6637 */
  {
    "ctraphi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_419 },
#else
    { 0, 0, output_419 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ctraphi4 },
    &operand_data[723],
    4,
    4,
    0,
    5,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6637 */
  {
    "ctrapsi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_420 },
#else
    { 0, 0, output_420 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ctrapsi4 },
    &operand_data[727],
    4,
    4,
    0,
    6,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6662 */
  {
    "ctrapqi4_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_421 },
#else
    { 0, 0, output_421 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ctrapqi4_cf },
    &operand_data[731],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6662 */
  {
    "ctraphi4_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_422 },
#else
    { 0, 0, output_422 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ctraphi4_cf },
    &operand_data[735],
    4,
    4,
    0,
    1,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6662 */
  {
    "ctrapsi4_cf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_423 },
#else
    { 0, 0, output_423 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ctrapsi4_cf },
    &operand_data[739],
    4,
    4,
    0,
    3,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6689 */
  {
    "stack_tie",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_stack_tie },
    &operand_data[743],
    2,
    2,
    0,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6701 */
  {
    "ib",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "#",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ib },
    &operand_data[0],
    0,
    0,
    0,
    0,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:39 */
  {
    "atomic_compare_and_swapqi_1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "cas%.b %1,%4,%2\n\tseq %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_atomic_compare_and_swapqi_1 },
    &operand_data[745],
    5,
    5,
    7,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:39 */
  {
    "atomic_compare_and_swaphi_1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "cas%.w %1,%4,%2\n\tseq %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_atomic_compare_and_swaphi_1 },
    &operand_data[750],
    5,
    5,
    7,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:39 */
  {
    "atomic_compare_and_swapsi_1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "cas%.l %1,%4,%2\n\tseq %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_atomic_compare_and_swapsi_1 },
    &operand_data[755],
    5,
    5,
    7,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:72 */
  {
    "atomic_test_and_set_1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .single =
#else
    {
#endif
    "tas %1\n\tsne %0",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    },
#else
    0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_atomic_test_and_set_1 },
    &operand_data[746],
    2,
    2,
    2,
    1,
    1
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:409 */
  {
    "cbranchdi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchdi4 },
    &operand_data[760],
    4,
    6,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:443 */
  {
    "cstoredi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoredi4 },
    &operand_data[766],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:464 */
  {
    "cbranchqi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchqi4 },
    &operand_data[770],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:464 */
  {
    "cbranchhi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchhi4 },
    &operand_data[774],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:464 */
  {
    "cbranchsi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsi4 },
    &operand_data[778],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:474 */
  {
    "cstoreqi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoreqi4 },
    &operand_data[782],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:474 */
  {
    "cstorehi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstorehi4 },
    &operand_data[786],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:474 */
  {
    "cstoresi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoresi4 },
    &operand_data[790],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:717 */
  {
    "cbranchsf4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchsf4 },
    &operand_data[794],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:717 */
  {
    "cbranchdf4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchdf4 },
    &operand_data[798],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:717 */
  {
    "cbranchxf4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cbranchxf4 },
    &operand_data[802],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:729 */
  {
    "cstoresf4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoresf4 },
    &operand_data[806],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:729 */
  {
    "cstoredf4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstoredf4 },
    &operand_data[810],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:729 */
  {
    "cstorexf4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_cstorexf4 },
    &operand_data[814],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:914 */
  {
    "movsi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movsi },
    &operand_data[818],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1078 */
  {
    "movhi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movhi },
    &operand_data[820],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1104 */
  {
    "movstricthi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movstricthi },
    &operand_data[822],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1124 */
  {
    "movqi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movqi },
    &operand_data[824],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1144 */
  {
    "movstrictqi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movstrictqi },
    &operand_data[826],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1168 */
  {
    "pushqi1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_pushqi1 },
    &operand_data[828],
    1,
    1,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1175 */
  {
    "reload_insf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_reload_insf },
    &operand_data[829],
    3,
    3,
    0,
    1,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1189 */
  {
    "reload_outsf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_reload_outsf },
    &operand_data[832],
    3,
    3,
    0,
    1,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1203 */
  {
    "movsf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movsf },
    &operand_data[835],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1317 */
  {
    "reload_indf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_reload_indf },
    &operand_data[837],
    3,
    3,
    0,
    1,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1331 */
  {
    "reload_outdf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_reload_outdf },
    &operand_data[840],
    3,
    3,
    0,
    1,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1345 */
  {
    "movdf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movdf },
    &operand_data[843],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1449 */
  {
    "movxf",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movxf },
    &operand_data[845],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1560 */
  {
    "movdi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_movdi },
    &operand_data[761],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1752 */
  {
    "zero_extendsidi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_zero_extendsidi2 },
    &operand_data[211],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1790 */
  {
    "zero_extendqihi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_zero_extendqihi2 },
    &operand_data[847],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1947 */
  {
    "extendhisi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendhisi2 },
    &operand_data[849],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1979 */
  {
    "extendqisi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendqisi2 },
    &operand_data[851],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2001 */
  {
    "extendsfdf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extendsfdf2 },
    &operand_data[853],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2054 */
  {
    "truncdfsf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_truncdfsf2 },
    &operand_data[855],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2099 */
  {
    "floatsisf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatsisf2 },
    &operand_data[857],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2099 */
  {
    "floatsidf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatsidf2 },
    &operand_data[859],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2099 */
  {
    "floatsixf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatsixf2 },
    &operand_data[861],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2120 */
  {
    "floathisf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floathisf2 },
    &operand_data[863],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2120 */
  {
    "floathidf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floathidf2 },
    &operand_data[865],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2120 */
  {
    "floathixf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floathixf2 },
    &operand_data[867],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2141 */
  {
    "floatqisf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatqisf2 },
    &operand_data[869],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2141 */
  {
    "floatqidf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatqidf2 },
    &operand_data[871],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2141 */
  {
    "floatqixf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_floatqixf2 },
    &operand_data[873],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2200 */
  {
    "ftruncsf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ftruncsf2 },
    &operand_data[835],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2200 */
  {
    "ftruncdf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ftruncdf2 },
    &operand_data[843],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2200 */
  {
    "ftruncxf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ftruncxf2 },
    &operand_data[845],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2230 */
  {
    "fixsfqi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixsfqi2 },
    &operand_data[875],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2230 */
  {
    "fixdfqi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixdfqi2 },
    &operand_data[877],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2230 */
  {
    "fixxfqi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixxfqi2 },
    &operand_data[879],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2250 */
  {
    "fixsfhi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixsfhi2 },
    &operand_data[881],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2250 */
  {
    "fixdfhi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixdfhi2 },
    &operand_data[883],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2250 */
  {
    "fixxfhi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixxfhi2 },
    &operand_data[885],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2270 */
  {
    "fixsfsi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixsfsi2 },
    &operand_data[887],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2270 */
  {
    "fixdfsi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixdfsi2 },
    &operand_data[889],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2270 */
  {
    "fixxfsi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_fixxfsi2 },
    &operand_data[891],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2491 */
  {
    "addsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addsi3 },
    &operand_data[893],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2796 */
  {
    "addsf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addsf3 },
    &operand_data[896],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2796 */
  {
    "adddf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_adddf3 },
    &operand_data[899],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2796 */
  {
    "addxf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_addxf3 },
    &operand_data[902],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3027 */
  {
    "subsf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subsf3 },
    &operand_data[896],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3027 */
  {
    "subdf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subdf3 },
    &operand_data[899],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3027 */
  {
    "subxf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_subxf3 },
    &operand_data[902],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3125 */
  {
    "mulsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulsi3 },
    &operand_data[905],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3179 */
  {
    "umulsidi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_umulsidi3 },
    &operand_data[908],
    3,
    3,
    3,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3218 */
  {
    "mulsidi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulsidi3 },
    &operand_data[908],
    3,
    3,
    3,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3252 */
  {
    "umulsi3_highpart",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_umulsi3_highpart },
    &operand_data[909],
    3,
    3,
    1,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3299 */
  {
    "smulsi3_highpart",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_smulsi3_highpart },
    &operand_data[909],
    3,
    3,
    1,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3342 */
  {
    "mulsf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulsf3 },
    &operand_data[896],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3342 */
  {
    "muldf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_muldf3 },
    &operand_data[899],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3342 */
  {
    "mulxf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_mulxf3 },
    &operand_data[902],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3445 */
  {
    "divsf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divsf3 },
    &operand_data[896],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3445 */
  {
    "divdf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divdf3 },
    &operand_data[899],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3445 */
  {
    "divxf3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divxf3 },
    &operand_data[902],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3515 */
  {
    "divmodsi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_divmodsi4 },
    &operand_data[910],
    4,
    4,
    2,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3557 */
  {
    "udivmodsi4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_udivmodsi4 },
    &operand_data[494],
    4,
    4,
    2,
    1,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3648 */
  {
    "andsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_andsi3 },
    &operand_data[914],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3766 */
  {
    "iorsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_iorsi3 },
    &operand_data[893],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3877 */
  {
    "xorsi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_xorsi3 },
    &operand_data[905],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3955 */
  {
    "negdi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negdi2 },
    &operand_data[761],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3993 */
  {
    "negsi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negsi2 },
    &operand_data[893],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4051 */
  {
    "negsf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negsf2 },
    &operand_data[835],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4076 */
  {
    "negdf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negdf2 },
    &operand_data[843],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4108 */
  {
    "negxf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_negxf2 },
    &operand_data[917],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4174 */
  {
    "sqrtsf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sqrtsf2 },
    &operand_data[835],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4174 */
  {
    "sqrtdf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sqrtdf2 },
    &operand_data[843],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4174 */
  {
    "sqrtxf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sqrtxf2 },
    &operand_data[845],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4204 */
  {
    "abssf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_abssf2 },
    &operand_data[835],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4229 */
  {
    "absdf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_absdf2 },
    &operand_data[843],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4261 */
  {
    "absxf2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_absxf2 },
    &operand_data[917],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4328 */
  {
    "clzsi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_clzsi2 },
    &operand_data[910],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4353 */
  {
    "one_cmplsi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_one_cmplsi2 },
    &operand_data[893],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4604 */
  {
    "ashldi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashldi3 },
    &operand_data[919],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4870 */
  {
    "ashrdi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_ashrdi3 },
    &operand_data[919],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5146 */
  {
    "lshrdi3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_lshrdi3 },
    &operand_data[919],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5371 */
  {
    "bswapsi2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_bswapsi2 },
    &operand_data[909],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5595 */
  {
    "extv",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extv },
    &operand_data[922],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5611 */
  {
    "extzv",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_extzv },
    &operand_data[922],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5663 */
  {
    "insv",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_insv },
    &operand_data[926],
    4,
    4,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5796 */
  {
    "tablejump",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_tablejump },
    &operand_data[930],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5919 */
  {
    "decrement_and_branch_until_zero",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_decrement_and_branch_until_zero },
    &operand_data[932],
    2,
    2,
    2,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5953 */
  {
    "sibcall",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sibcall },
    &operand_data[934],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5969 */
  {
    "sibcall_value",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sibcall_value },
    &operand_data[933],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5989 */
  {
    "call",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_call },
    &operand_data[934],
    2,
    2,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6010 */
  {
    "call_value",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_call_value },
    &operand_data[933],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6060 */
  {
    "untyped_call",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_untyped_call },
    &operand_data[936],
    3,
    3,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6102 */
  {
    "prologue",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_prologue },
    &operand_data[0],
    0,
    0,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6110 */
  {
    "epilogue",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_epilogue },
    &operand_data[0],
    0,
    0,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6118 */
  {
    "sibcall_epilogue",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_sibcall_epilogue },
    &operand_data[0],
    0,
    0,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6127 */
  {
    "return",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_return },
    &operand_data[0],
    0,
    0,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6192 */
  {
    "link",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_link },
    &operand_data[939],
    2,
    2,
    2,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6227 */
  {
    "unlink",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_unlink },
    &operand_data[909],
    1,
    1,
    2,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6471 */
  {
    "unlink+1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_541 },
#else
    { 0, 0, output_541 },
#endif
    { 0 },
    &operand_data[941],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6471 */
  {
    "unlink+2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_542 },
#else
    { 0, 0, output_542 },
#endif
    { 0 },
    &operand_data[947],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6471 */
  {
    "unlink+3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_543 },
#else
    { 0, 0, output_543 },
#endif
    { 0 },
    &operand_data[953],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6471 */
  {
    "unlink+4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_544 },
#else
    { 0, 0, output_544 },
#endif
    { 0 },
    &operand_data[959],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6471 */
  {
    "unlink+5",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_545 },
#else
    { 0, 0, output_545 },
#endif
    { 0 },
    &operand_data[965],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6471 */
  {
    "unlink+6",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_546 },
#else
    { 0, 0, output_546 },
#endif
    { 0 },
    &operand_data[971],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6495 */
  {
    "atomic_compare_and_swapqi-6",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_547 },
#else
    { 0, 0, output_547 },
#endif
    { 0 },
    &operand_data[941],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6495 */
  {
    "atomic_compare_and_swapqi-5",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_548 },
#else
    { 0, 0, output_548 },
#endif
    { 0 },
    &operand_data[947],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6495 */
  {
    "atomic_compare_and_swapqi-4",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_549 },
#else
    { 0, 0, output_549 },
#endif
    { 0 },
    &operand_data[953],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6495 */
  {
    "atomic_compare_and_swapqi-3",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_550 },
#else
    { 0, 0, output_550 },
#endif
    { 0 },
    &operand_data[959],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6495 */
  {
    "atomic_compare_and_swapqi-2",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_551 },
#else
    { 0, 0, output_551 },
#endif
    { 0 },
    &operand_data[965],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6495 */
  {
    "atomic_compare_and_swapqi-1",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { .function = output_552 },
#else
    { 0, 0, output_552 },
#endif
    { 0 },
    &operand_data[971],
    0,
    6,
    0,
    0,
    3
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:21 */
  {
    "atomic_compare_and_swapqi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_atomic_compare_and_swapqi },
    &operand_data[977],
    8,
    8,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:21 */
  {
    "atomic_compare_and_swaphi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_atomic_compare_and_swaphi },
    &operand_data[985],
    8,
    8,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:21 */
  {
    "atomic_compare_and_swapsi",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_atomic_compare_and_swapsi },
    &operand_data[993],
    8,
    8,
    0,
    0,
    0
  },
  /* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:58 */
  {
    "atomic_test_and_set",
#if HAVE_DESIGNATED_UNION_INITIALIZERS
    { 0 },
#else
    { 0, 0, 0 },
#endif
    { (insn_gen_fn::stored_funcptr) gen_atomic_test_and_set },
    &operand_data[1001],
    3,
    3,
    0,
    0,
    0
  },
};


const char *
get_insn_name (int code)
{
  if (code == NOOP_MOVE_INSN_CODE)
    return "NOOP_MOVE";
  else
    return insn_data[code].name;
}
