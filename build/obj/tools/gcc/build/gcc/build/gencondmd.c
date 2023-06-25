/* Generated automatically by the program `genconditions' from the target
   machine description file.  */

#define IN_TARGET_CODE 1
#include "bconfig.h"
#define INCLUDE_STRING
#include "system.h"

/* It is necessary, but not entirely safe, to include the headers below
   in a generator program.  As a defensive measure, don't do so when the
   table isn't going to have anything in it.  */
#if GCC_VERSION >= 3001

/* Do not allow checking to confuse the issue.  */
#undef CHECKING_P
#define CHECKING_P 0
#undef ENABLE_TREE_CHECKING
#undef ENABLE_RTL_CHECKING
#undef ENABLE_RTL_FLAG_CHECKING
#undef ENABLE_GC_CHECKING
#undef ENABLE_GC_ALWAYS_COLLECT
#define USE_ENUM_MODES

#include "coretypes.h"
#include "tm.h"
#include "insn-constants.h"
#include "rtl.h"
#include "memmodel.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "function.h"
#include "emit-rtl.h"

/* Fake - insn-config.h doesn't exist yet.  */
#define MAX_RECOG_OPERANDS 10
#define MAX_DUP_OPERANDS 10
#define MAX_INSNS_PER_SPLIT 5

#include "regs.h"
#include "recog.h"
#include "output.h"
#include "flags.h"
#include "hard-reg-set.h"
#include "predict.h"
#include "basic-block.h"
#include "bitmap.h"
#include "df.h"
#include "resource.h"
#include "diagnostic-core.h"
#include "reload.h"
#include "tm-constrs.h"

#include "except.h"

/* Dummy external declarations.  */
extern rtx_insn *insn;
extern rtx ins1;
extern rtx operands[];

#endif /* gcc >= 3.0.1 */

/* Structure definition duplicated from gensupport.h rather than
   drag in that file and its dependencies.  */
struct c_test
{
  const char *expr;
  int value;
};

/* This table lists each condition found in the machine description.
   Each condition is mapped to its truth value (0 or 1), or -1 if that
   cannot be calculated at compile time.
   If we don't have __builtin_constant_p, or it's not acceptable in array
   initializers, fall back to assuming that all conditions potentially
   vary at run time.  It works in 3.0.1 and later; 3.0 only when not
   optimizing.  */

#if GCC_VERSION >= 3001
static const struct c_test insn_conditions[] = {

#line 5839 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TARGET_LONG_JUMP_TABLE_OFFSETS",
    __builtin_constant_p 
#line 5839 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_LONG_JUMP_TABLE_OFFSETS)
    ? (int) 
#line 5839 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_LONG_JUMP_TABLE_OFFSETS)
    : -1 },
#line 995 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TARGET_COLDFIRE && reload_completed",
    __builtin_constant_p 
#line 995 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && reload_completed)
    ? (int) 
#line 995 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && reload_completed)
    : -1 },
  { "(TARGET_68881\n\
   && (register_operand (operands[1], XFmode)\n\
       || register_operand (operands[2], XFmode)\n\
       || const0_operand (operands[2], XFmode))) && (TARGET_68881)",
    __builtin_constant_p (
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], XFmode)
       || register_operand (operands[2], XFmode)
       || const0_operand (operands[2], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], XFmode)
       || register_operand (operands[2], XFmode)
       || const0_operand (operands[2], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)\n\
   && (register_operand (operands[2], SFmode)\n\
       || register_operand (operands[3], SFmode)\n\
       || const0_operand (operands[3], SFmode))",
    __builtin_constant_p 
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)
   && (register_operand (operands[2], SFmode)
       || register_operand (operands[3], SFmode)
       || const0_operand (operands[3], SFmode)))
    ? (int) 
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)
   && (register_operand (operands[2], SFmode)
       || register_operand (operands[3], SFmode)
       || const0_operand (operands[3], SFmode)))
    : -1 },
#line 6770 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "peep2_reg_dead_p (2, operands[0])\n\
   && peep2_reg_dead_p (2, operands[2])\n\
   && (operands[3] == pc_rtx || operands[4] == pc_rtx)\n\
   && DATA_REG_P (operands[2])\n\
   && !rtx_equal_p (operands[0], operands[2])",
    __builtin_constant_p 
#line 6770 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(peep2_reg_dead_p (2, operands[0])
   && peep2_reg_dead_p (2, operands[2])
   && (operands[3] == pc_rtx || operands[4] == pc_rtx)
   && DATA_REG_P (operands[2])
   && !rtx_equal_p (operands[0], operands[2]))
    ? (int) 
#line 6770 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(peep2_reg_dead_p (2, operands[0])
   && peep2_reg_dead_p (2, operands[2])
   && (operands[3] == pc_rtx || operands[4] == pc_rtx)
   && DATA_REG_P (operands[2])
   && !rtx_equal_p (operands[0], operands[2]))
    : -1 },
  { "(TARGET_COLDFIRE) && ( reload_completed && (extract_constrain_insn_cached (insn), which_alternative == 5) && !operands_match_p (operands[0], operands[1]))",
    __builtin_constant_p (
#line 2516 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE) && 
#line 2550 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed && (extract_constrain_insn_cached (insn), which_alternative == 5) && !operands_match_p (operands[0], operands[1])))
    ? (int) (
#line 2516 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE) && 
#line 2550 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed && (extract_constrain_insn_cached (insn), which_alternative == 5) && !operands_match_p (operands[0], operands[1])))
    : -1 },
#line 4847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TARGET_COLDFIRE\n\
    && ((INTVAL (operands[2]) >= 1 && INTVAL (operands[2]) <= 3)\n\
	|| INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16\n\
	|| INTVAL (operands[2]) == 31\n\
	|| (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63))",
    __builtin_constant_p 
#line 4847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE
    && ((INTVAL (operands[2]) >= 1 && INTVAL (operands[2]) <= 3)
	|| INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16
	|| INTVAL (operands[2]) == 31
	|| (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63)))
    ? (int) 
#line 4847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE
    && ((INTVAL (operands[2]) >= 1 && INTVAL (operands[2]) <= 3)
	|| INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16
	|| INTVAL (operands[2]) == 31
	|| (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63)))
    : -1 },
#line 5668 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD",
    __builtin_constant_p 
#line 5668 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD)
    ? (int) 
#line 5668 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD)
    : -1 },
#line 6667 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_COLDFIRE",
    __builtin_constant_p 
#line 6667 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_COLDFIRE)
    ? (int) 
#line 6667 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_COLDFIRE)
    : -1 },
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_COLDFIRE_FPU\n\
   && (register_operand (operands[1], SFmode)\n\
       || register_operand (operands[2], SFmode)\n\
       || const0_operand (operands[2], SFmode))",
    __builtin_constant_p 
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], SFmode)
       || register_operand (operands[2], SFmode)
       || const0_operand (operands[2], SFmode)))
    ? (int) 
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], SFmode)
       || register_operand (operands[2], SFmode)
       || const0_operand (operands[2], SFmode)))
    : -1 },
#line 1071 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_PCREL",
    __builtin_constant_p 
#line 1071 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_PCREL)
    ? (int) 
#line 1071 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_PCREL)
    : -1 },
#line 6065 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "NEEDS_UNTYPED_CALL",
    __builtin_constant_p 
#line 6065 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(NEEDS_UNTYPED_CALL)
    ? (int) 
#line 6065 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(NEEDS_UNTYPED_CALL)
    : -1 },
#line 6158 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "m68k_movem_pattern_p (operands[0], NULL, 0, true)",
    __builtin_constant_p 
#line 6158 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], NULL, 0, true))
    ? (int) 
#line 6158 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], NULL, 0, true))
    : -1 },
#line 3573 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_CF_HWDIV",
    __builtin_constant_p 
#line 3573 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_CF_HWDIV)
    ? (int) 
#line 3573 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_CF_HWDIV)
    : -1 },
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)\n\
   && (register_operand (operands[2], DFmode)\n\
       || register_operand (operands[3], DFmode)\n\
       || const0_operand (operands[3], DFmode))",
    __builtin_constant_p 
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)
   && (register_operand (operands[2], DFmode)
       || register_operand (operands[3], DFmode)
       || const0_operand (operands[3], DFmode)))
    ? (int) 
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)
   && (register_operand (operands[2], DFmode)
       || register_operand (operands[3], DFmode)
       || const0_operand (operands[3], DFmode)))
    : -1 },
#line 6129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "m68k_use_return_insn ()",
    __builtin_constant_p 
#line 6129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_use_return_insn ())
    ? (int) 
#line 6129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_use_return_insn ())
    : -1 },
#line 2203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_HARD_FLOAT && !TUNE_68040",
    __builtin_constant_p 
#line 2203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !TUNE_68040)
    ? (int) 
#line 2203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !TUNE_68040)
    : -1 },
  { "(TARGET_COLDFIRE_FPU) && (TARGET_68881)",
    __builtin_constant_p (
#line 4313 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 4313 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
#line 5706 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[3]), 0, 31)",
    __builtin_constant_p 
#line 5706 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[3]), 0, 31))
    ? (int) 
#line 5706 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[3]), 0, 31))
    : -1 },
#line 30 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
  { "TARGET_CAS",
    __builtin_constant_p 
#line 30 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
(TARGET_CAS)
    ? (int) 
#line 30 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
(TARGET_CAS)
    : -1 },
#line 5825 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_LONG_JUMP_TABLE_OFFSETS",
    __builtin_constant_p 
#line 5825 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_LONG_JUMP_TABLE_OFFSETS)
    ? (int) 
#line 5825 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_LONG_JUMP_TABLE_OFFSETS)
    : -1 },
#line 6035 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!SIBLING_CALL_P (insn) && m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_JSR",
    __builtin_constant_p 
#line 6035 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn) && m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_JSR)
    ? (int) 
#line 6035 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn) && m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_JSR)
    : -1 },
  { "(TARGET_68881 && flag_unsafe_math_optimizations) && (TARGET_68881)",
    __builtin_constant_p (
#line 6618 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 6618 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
#line 6025 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!SIBLING_CALL_P (insn)",
    __builtin_constant_p 
#line 6025 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn))
    ? (int) 
#line 6025 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn))
    : -1 },
#line 1335 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_COLDFIRE_FPU",
    __builtin_constant_p 
#line 1335 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU)
    ? (int) 
#line 1335 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU)
    : -1 },
#line 1820 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!ISA_HAS_MVS_MVZ\n\
   && reload_completed\n\
   && reg_mentioned_p (operands[0], operands[1])",
    __builtin_constant_p 
#line 1820 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!ISA_HAS_MVS_MVZ
   && reload_completed
   && reg_mentioned_p (operands[0], operands[1]))
    ? (int) 
#line 1820 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!ISA_HAS_MVS_MVZ
   && reload_completed
   && reg_mentioned_p (operands[0], operands[1]))
    : -1 },
#line 6349 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "GET_CODE (XEXP (operands[0], 0)) != PRE_DEC\n\
   && INTVAL (operands[1]) != 0\n\
   && IN_RANGE (INTVAL (operands[1]), -0x80, 0x7f)\n\
   && !valid_mov3q_const (INTVAL (operands[1]))",
    __builtin_constant_p 
#line 6349 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(GET_CODE (XEXP (operands[0], 0)) != PRE_DEC
   && INTVAL (operands[1]) != 0
   && IN_RANGE (INTVAL (operands[1]), -0x80, 0x7f)
   && !valid_mov3q_const (INTVAL (operands[1])))
    ? (int) 
#line 6349 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(GET_CODE (XEXP (operands[0], 0)) != PRE_DEC
   && INTVAL (operands[1]) != 0
   && IN_RANGE (INTVAL (operands[1]), -0x80, 0x7f)
   && !valid_mov3q_const (INTVAL (operands[1])))
    : -1 },
#line 6362 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "INTVAL (operands[1]) != 0\n\
   && IN_RANGE (INTVAL (operands[1]), -0x80, 0x7f)\n\
   && !valid_mov3q_const (INTVAL (operands[1]))",
    __builtin_constant_p 
#line 6362 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) != 0
   && IN_RANGE (INTVAL (operands[1]), -0x80, 0x7f)
   && !valid_mov3q_const (INTVAL (operands[1])))
    ? (int) 
#line 6362 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) != 0
   && IN_RANGE (INTVAL (operands[1]), -0x80, 0x7f)
   && !valid_mov3q_const (INTVAL (operands[1])))
    : -1 },
#line 6729 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "(optimize_size || !TUNE_68060)\n\
   && (operands[0] == operands[3] || operands[0] == operands[4])\n\
   && ADDRESS_REG_P (operands[1])\n\
   && ADDRESS_REG_P ((operands[0] == operands[3]) ? operands[4] : operands[3])\n\
   && peep2_reg_dead_p (2, operands[3])\n\
   && peep2_reg_dead_p (2, operands[4])",
    __builtin_constant_p 
#line 6729 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((optimize_size || !TUNE_68060)
   && (operands[0] == operands[3] || operands[0] == operands[4])
   && ADDRESS_REG_P (operands[1])
   && ADDRESS_REG_P ((operands[0] == operands[3]) ? operands[4] : operands[3])
   && peep2_reg_dead_p (2, operands[3])
   && peep2_reg_dead_p (2, operands[4]))
    ? (int) 
#line 6729 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((optimize_size || !TUNE_68060)
   && (operands[0] == operands[3] || operands[0] == operands[4])
   && ADDRESS_REG_P (operands[1])
   && ADDRESS_REG_P ((operands[0] == operands[3]) ? operands[4] : operands[3])
   && peep2_reg_dead_p (2, operands[3])
   && peep2_reg_dead_p (2, operands[4]))
    : -1 },
#line 880 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TUNE_68040_60",
    __builtin_constant_p 
#line 880 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68040_60)
    ? (int) 
#line 880 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68040_60)
    : -1 },
#line 6313 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!reg_mentioned_p (stack_pointer_rtx, operands[0])",
    __builtin_constant_p 
#line 6313 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[0]))
    ? (int) 
#line 6313 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[0]))
    : -1 },
#line 734 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)",
    __builtin_constant_p 
#line 734 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU))
    ? (int) 
#line 734 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU))
    : -1 },
#line 1692 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "GET_MODE_CLASS (GET_MODE (operands[0])) == MODE_INT &&\n\
   GET_MODE_CLASS (GET_MODE (operands[1])) == MODE_INT &&\n\
   GET_MODE_SIZE (GET_MODE (operands[0])) == GET_MODE_SIZE (GET_MODE (operands[1])) * 2",
    __builtin_constant_p 
#line 1692 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(GET_MODE_CLASS (GET_MODE (operands[0])) == MODE_INT &&
   GET_MODE_CLASS (GET_MODE (operands[1])) == MODE_INT &&
   GET_MODE_SIZE (GET_MODE (operands[0])) == GET_MODE_SIZE (GET_MODE (operands[1])) * 2)
    ? (int) 
#line 1692 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(GET_MODE_CLASS (GET_MODE (operands[0])) == MODE_INT &&
   GET_MODE_CLASS (GET_MODE (operands[1])) == MODE_INT &&
   GET_MODE_SIZE (GET_MODE (operands[0])) == GET_MODE_SIZE (GET_MODE (operands[1])) * 2)
    : -1 },
#line 4598 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TARGET_COLDFIRE\n\
    && ((INTVAL (operands[2]) >= 1 && INTVAL (operands[2]) <= 3)\n\
	|| INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16\n\
	|| (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63))",
    __builtin_constant_p 
#line 4598 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE
    && ((INTVAL (operands[2]) >= 1 && INTVAL (operands[2]) <= 3)
	|| INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16
	|| (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63)))
    ? (int) 
#line 4598 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE
    && ((INTVAL (operands[2]) >= 1 && INTVAL (operands[2]) <= 3)
	|| INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16
	|| (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63)))
    : -1 },
#line 6423 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!reg_mentioned_p (stack_pointer_rtx, operands[1])",
    __builtin_constant_p 
#line 6423 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[1]))
    ? (int) 
#line 6423 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[1]))
    : -1 },
#line 3214 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE\n\
   && (unsigned) INTVAL (operands[2]) <= 0x7fffffff",
    __builtin_constant_p 
#line 3214 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE
   && (unsigned) INTVAL (operands[2]) <= 0x7fffffff)
    ? (int) 
#line 3214 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE
   && (unsigned) INTVAL (operands[2]) <= 0x7fffffff)
    : -1 },
#line 3169 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "INTVAL (operands[2]) >= 0 && INTVAL (operands[2]) <= 0xffff",
    __builtin_constant_p 
#line 3169 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[2]) >= 0 && INTVAL (operands[2]) <= 0xffff)
    ? (int) 
#line 3169 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[2]) >= 0 && INTVAL (operands[2]) <= 0xffff)
    : -1 },
#line 6376 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "INTVAL (operands[0]) > 4\n\
   && !reg_mentioned_p (stack_pointer_rtx, operands[2])\n\
   && !(CONST_INT_P (operands[2]) && INTVAL (operands[2]) != 0\n\
	&& IN_RANGE (INTVAL (operands[2]), -0x8000, 0x7fff)\n\
	&& !valid_mov3q_const (INTVAL (operands[2])))",
    __builtin_constant_p 
#line 6376 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[0]) > 4
   && !reg_mentioned_p (stack_pointer_rtx, operands[2])
   && !(CONST_INT_P (operands[2]) && INTVAL (operands[2]) != 0
	&& IN_RANGE (INTVAL (operands[2]), -0x8000, 0x7fff)
	&& !valid_mov3q_const (INTVAL (operands[2]))))
    ? (int) 
#line 6376 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[0]) > 4
   && !reg_mentioned_p (stack_pointer_rtx, operands[2])
   && !(CONST_INT_P (operands[2]) && INTVAL (operands[2]) != 0
	&& IN_RANGE (INTVAL (operands[2]), -0x8000, 0x7fff)
	&& !valid_mov3q_const (INTVAL (operands[2]))))
    : -1 },
  { "(TARGET_HARD_FLOAT) && (TARGET_68881)",
    __builtin_constant_p (
#line 4177 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 4177 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
  { "(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)\n\
   && (register_operand (operands[2], XFmode)\n\
       || register_operand (operands[3], XFmode)\n\
       || const0_operand (operands[3], XFmode))) && (TARGET_68881)",
    __builtin_constant_p (
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)
   && (register_operand (operands[2], XFmode)
       || register_operand (operands[3], XFmode)
       || const0_operand (operands[3], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)
   && (register_operand (operands[2], XFmode)
       || register_operand (operands[3], XFmode)
       || const0_operand (operands[3], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
#line 3549 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020",
    __builtin_constant_p 
#line 3549 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020)
    ? (int) 
#line 3549 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020)
    : -1 },
#line 298 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { " reload_completed && (extract_constrain_insn_cached (insn), which_alternative == 1)",
    __builtin_constant_p 
#line 298 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed && (extract_constrain_insn_cached (insn), which_alternative == 1))
    ? (int) 
#line 298 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed && (extract_constrain_insn_cached (insn), which_alternative == 1))
    : -1 },
#line 5578 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD\n\
   && IN_RANGE (INTVAL (operands[3]), 0, 31)\n\
   && (INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16)\n\
   && INTVAL (operands[3]) % INTVAL (operands[2]) == 0",
    __builtin_constant_p 
#line 5578 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && IN_RANGE (INTVAL (operands[3]), 0, 31)
   && (INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16)
   && INTVAL (operands[3]) % INTVAL (operands[2]) == 0)
    ? (int) 
#line 5578 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && IN_RANGE (INTVAL (operands[3]), 0, 31)
   && (INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16)
   && INTVAL (operands[3]) % INTVAL (operands[2]) == 0)
    : -1 },
#line 5944 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TARGET_COLDFIRE && find_reg_note (insn, REG_NONNEG, 0)",
    __builtin_constant_p 
#line 5944 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && find_reg_note (insn, REG_NONNEG, 0))
    ? (int) 
#line 5944 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && find_reg_note (insn, REG_NONNEG, 0))
    : -1 },
#line 6299 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "FP_REG_P (operands[0]) && !FP_REG_P (operands[1])",
    __builtin_constant_p 
#line 6299 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(FP_REG_P (operands[0]) && !FP_REG_P (operands[1]))
    ? (int) 
#line 6299 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(FP_REG_P (operands[0]) && !FP_REG_P (operands[1]))
    : -1 },
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_COLDFIRE_FPU\n\
   && (register_operand (operands[1], DFmode)\n\
       || register_operand (operands[2], DFmode)\n\
       || const0_operand (operands[2], DFmode))",
    __builtin_constant_p 
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], DFmode)
       || register_operand (operands[2], DFmode)
       || const0_operand (operands[2], DFmode)))
    ? (int) 
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], DFmode)
       || register_operand (operands[2], DFmode)
       || const0_operand (operands[2], DFmode)))
    : -1 },
#line 5783 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_COLDFIRE",
    __builtin_constant_p 
#line 5783 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE)
    ? (int) 
#line 5783 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE)
    : -1 },
#line 898 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!(TUNE_68000_10 || TUNE_68040_60)",
    __builtin_constant_p 
#line 898 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!(TUNE_68000_10 || TUNE_68040_60))
    ? (int) 
#line 898 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!(TUNE_68000_10 || TUNE_68040_60))
    : -1 },
#line 5494 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD\n\
   && IN_RANGE (INTVAL (operands[2]), 0, 31)\n\
   && (INTVAL (operands[1]) == 8 || INTVAL (operands[1]) == 16)\n\
   && INTVAL (operands[2]) % INTVAL (operands[1]) == 0",
    __builtin_constant_p 
#line 5494 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && IN_RANGE (INTVAL (operands[2]), 0, 31)
   && (INTVAL (operands[1]) == 8 || INTVAL (operands[1]) == 16)
   && INTVAL (operands[2]) % INTVAL (operands[1]) == 0)
    ? (int) 
#line 5494 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && IN_RANGE (INTVAL (operands[2]), 0, 31)
   && (INTVAL (operands[1]) == 8 || INTVAL (operands[1]) == 16)
   && INTVAL (operands[2]) % INTVAL (operands[1]) == 0)
    : -1 },
#line 5140 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "(!TARGET_COLDFIRE\n\
    && ((INTVAL (operands[2]) >= 2 && INTVAL (operands[2]) <= 3)\n\
	 || INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16\n\
	 || (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63)))",
    __builtin_constant_p 
#line 5140 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((!TARGET_COLDFIRE
    && ((INTVAL (operands[2]) >= 2 && INTVAL (operands[2]) <= 3)
	 || INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16
	 || (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63))))
    ? (int) 
#line 5140 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((!TARGET_COLDFIRE
    && ((INTVAL (operands[2]) >= 2 && INTVAL (operands[2]) <= 3)
	 || INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16
	 || (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63))))
    : -1 },
#line 6618 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68881 && flag_unsafe_math_optimizations",
    __builtin_constant_p 
#line 6618 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations)
    ? (int) 
#line 6618 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations)
    : -1 },
#line 3308 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE",
    __builtin_constant_p 
#line 3308 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE)
    ? (int) 
#line 3308 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE)
    : -1 },
#line 6322 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "INTVAL (operands[0]) > 4\n\
   && !reg_mentioned_p (stack_pointer_rtx, operands[2])",
    __builtin_constant_p 
#line 6322 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[0]) > 4
   && !reg_mentioned_p (stack_pointer_rtx, operands[2]))
    ? (int) 
#line 6322 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[0]) > 4
   && !reg_mentioned_p (stack_pointer_rtx, operands[2]))
    : -1 },
#line 5101 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "reload_completed",
    __builtin_constant_p 
#line 5101 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed)
    ? (int) 
#line 5101 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed)
    : -1 },
#line 1557 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "! TARGET_68881 && TARGET_COLDFIRE",
    __builtin_constant_p 
#line 1557 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_68881 && TARGET_COLDFIRE)
    ? (int) 
#line 1557 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_68881 && TARGET_COLDFIRE)
    : -1 },
#line 6642 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && !TARGET_COLDFIRE",
    __builtin_constant_p 
#line 6642 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TARGET_COLDFIRE)
    ? (int) 
#line 6642 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TARGET_COLDFIRE)
    : -1 },
#line 6438 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "REGNO (operands[0]) == REGNO (operands[1])\n\
   && strict_low_part_peephole_ok (HImode, insn, operands[0])",
    __builtin_constant_p 
#line 6438 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(REGNO (operands[0]) == REGNO (operands[1])
   && strict_low_part_peephole_ok (HImode, insn, operands[0]))
    ? (int) 
#line 6438 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(REGNO (operands[0]) == REGNO (operands[1])
   && strict_low_part_peephole_ok (HImode, insn, operands[0]))
    : -1 },
#line 1966 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!ISA_HAS_MVS_MVZ",
    __builtin_constant_p 
#line 1966 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!ISA_HAS_MVS_MVZ)
    ? (int) 
#line 1966 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!ISA_HAS_MVS_MVZ)
    : -1 },
#line 3118 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "INTVAL (operands[2]) >= -0x8000 && INTVAL (operands[2]) <= 0x7fff",
    __builtin_constant_p 
#line 3118 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[2]) >= -0x8000 && INTVAL (operands[2]) <= 0x7fff)
    ? (int) 
#line 3118 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[2]) >= -0x8000 && INTVAL (operands[2]) <= 0x7fff)
    : -1 },
#line 4542 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { " reload_completed",
    __builtin_constant_p 
#line 4542 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed)
    ? (int) 
#line 4542 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed)
    : -1 },
#line 5478 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD\n\
   && (INTVAL (operands[1]) % 8) == 0\n\
   && ! mode_dependent_address_p (XEXP (operands[0], 0),\n\
                                  MEM_ADDR_SPACE (operands[0]))",
    __builtin_constant_p 
#line 5478 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[1]) % 8) == 0
   && ! mode_dependent_address_p (XEXP (operands[0], 0),
                                  MEM_ADDR_SPACE (operands[0])))
    ? (int) 
#line 5478 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[1]) % 8) == 0
   && ! mode_dependent_address_p (XEXP (operands[0], 0),
                                  MEM_ADDR_SPACE (operands[0])))
    : -1 },
#line 6048 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!SIBLING_CALL_P (insn)\n\
   && (m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_BSR_C\n\
       || m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_BSR_P)",
    __builtin_constant_p 
#line 6048 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn)
   && (m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_BSR_C
       || m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_BSR_P))
    ? (int) 
#line 6048 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn)
   && (m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_BSR_C
       || m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_BSR_P))
    : -1 },
#line 1836 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!ISA_HAS_MVS_MVZ && reload_completed",
    __builtin_constant_p 
#line 1836 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!ISA_HAS_MVS_MVZ && reload_completed)
    ? (int) 
#line 1836 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!ISA_HAS_MVS_MVZ && reload_completed)
    : -1 },
#line 5028 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "reload_completed && !TARGET_COLDFIRE",
    __builtin_constant_p 
#line 5028 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE)
    ? (int) 
#line 5028 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE)
    : -1 },
  { "(TARGET_COLDFIRE_FPU\n\
   && (register_operand (operands[1], XFmode)\n\
       || register_operand (operands[2], XFmode)\n\
       || const0_operand (operands[2], XFmode))) && (TARGET_68881)",
    __builtin_constant_p (
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], XFmode)
       || register_operand (operands[2], XFmode)
       || const0_operand (operands[2], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], XFmode)
       || register_operand (operands[2], XFmode)
       || const0_operand (operands[2], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
#line 62 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
  { "ISA_HAS_TAS",
    __builtin_constant_p 
#line 62 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
(ISA_HAS_TAS)
    ? (int) 
#line 62 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
(ISA_HAS_TAS)
    : -1 },
#line 5635 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD\n\
   && (INTVAL (operands[3]) == -1\n\
       || (GET_CODE (operands[1]) == CONST_INT\n\
           && (~ INTVAL (operands[3]) & ((1 << INTVAL (operands[1]))- 1)) == 0))",
    __builtin_constant_p 
#line 5635 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[3]) == -1
       || (GET_CODE (operands[1]) == CONST_INT
           && (~ INTVAL (operands[3]) & ((1 << INTVAL (operands[1]))- 1)) == 0)))
    ? (int) 
#line 5635 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[3]) == -1
       || (GET_CODE (operands[1]) == CONST_INT
           && (~ INTVAL (operands[3]) & ((1 << INTVAL (operands[1]))- 1)) == 0)))
    : -1 },
#line 4582 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "reload_completed && !TARGET_COLDFIRE\n\
   && INTVAL (operands[2]) > 40 && INTVAL (operands[2]) <= 63",
    __builtin_constant_p 
#line 4582 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 40 && INTVAL (operands[2]) <= 63)
    ? (int) 
#line 4582 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 40 && INTVAL (operands[2]) <= 63)
    : -1 },
#line 3564 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 || TARGET_CF_HWDIV",
    __builtin_constant_p 
#line 3564 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || TARGET_CF_HWDIV)
    ? (int) 
#line 3564 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || TARGET_CF_HWDIV)
    : -1 },
#line 6512 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TARGET_COLDFIRE && DATA_REG_P (operands[0])",
    __builtin_constant_p 
#line 6512 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && DATA_REG_P (operands[0]))
    ? (int) 
#line 6512 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && DATA_REG_P (operands[0]))
    : -1 },
#line 6846 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "INTVAL (operands[1]) <= 255\n\
   &&  peep2_reg_dead_p (1, operands[0])\n\
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)\n\
   && (optimize_size || TUNE_68040_60)\n\
   && DATA_REG_P (operands[0])",
    __builtin_constant_p 
#line 6846 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   &&  peep2_reg_dead_p (1, operands[0])
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[0]))
    ? (int) 
#line 6846 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   &&  peep2_reg_dead_p (1, operands[0])
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[0]))
    : -1 },
#line 1765 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "GET_CODE (operands[0]) != MEM || GET_CODE (operands[1]) != MEM",
    __builtin_constant_p 
#line 1765 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(GET_CODE (operands[0]) != MEM || GET_CODE (operands[1]) != MEM)
    ? (int) 
#line 1765 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(GET_CODE (operands[0]) != MEM || GET_CODE (operands[1]) != MEM)
    : -1 },
#line 6826 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "INTVAL (operands[1]) <= 255\n\
   && operands[0] == operands[3]\n\
   && peep2_reg_dead_p (2, operands[0])\n\
   && peep2_reg_dead_p (2, operands[2])\n\
   && (operands[4] == pc_rtx || operands[5] == pc_rtx)\n\
   && (optimize_size || TUNE_68040_60)\n\
   && DATA_REG_P (operands[2])",
    __builtin_constant_p 
#line 6826 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   && operands[0] == operands[3]
   && peep2_reg_dead_p (2, operands[0])
   && peep2_reg_dead_p (2, operands[2])
   && (operands[4] == pc_rtx || operands[5] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[2]))
    ? (int) 
#line 6826 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   && operands[0] == operands[3]
   && peep2_reg_dead_p (2, operands[0])
   && peep2_reg_dead_p (2, operands[2])
   && (operands[4] == pc_rtx || operands[5] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[2]))
    : -1 },
  { "(TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU) && (TARGET_68881)",
    __builtin_constant_p (
#line 832 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 832 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
#line 2066 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68881 && TARGET_68040",
    __builtin_constant_p 
#line 2066 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TARGET_68040)
    ? (int) 
#line 2066 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TARGET_68040)
    : -1 },
#line 5736 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[2]), 0, 31)",
    __builtin_constant_p 
#line 5736 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[2]), 0, 31))
    ? (int) 
#line 5736 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[2]), 0, 31))
    : -1 },
#line 4177 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_HARD_FLOAT",
    __builtin_constant_p 
#line 4177 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT)
    ? (int) 
#line 4177 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT)
    : -1 },
#line 1988 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "ISA_HAS_MVS_MVZ",
    __builtin_constant_p 
#line 1988 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_MVS_MVZ)
    ? (int) 
#line 1988 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_MVS_MVZ)
    : -1 },
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68881\n\
   && (register_operand (operands[1], SFmode)\n\
       || register_operand (operands[2], SFmode)\n\
       || const0_operand (operands[2], SFmode))",
    __builtin_constant_p 
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], SFmode)
       || register_operand (operands[2], SFmode)
       || const0_operand (operands[2], SFmode)))
    ? (int) 
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], SFmode)
       || register_operand (operands[2], SFmode)
       || const0_operand (operands[2], SFmode)))
    : -1 },
#line 6168 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "m68k_movem_pattern_p (operands[0], operands[1], INTVAL (operands[3]), true)",
    __builtin_constant_p 
#line 6168 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], operands[1], INTVAL (operands[3]), true))
    ? (int) 
#line 6168 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], operands[1], INTVAL (operands[3]), true))
    : -1 },
#line 6395 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!reg_mentioned_p (stack_pointer_rtx, operands[1])\n\
   && GET_CODE (XEXP (operands[0], 0)) == PLUS\n\
   && rtx_equal_p (XEXP (XEXP (operands[0], 0), 0), stack_pointer_rtx)\n\
   && CONST_INT_P (XEXP (XEXP (operands[0], 0), 1))\n\
   && INTVAL (XEXP (XEXP (operands[0], 0), 1)) == 3",
    __builtin_constant_p 
#line 6395 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[1])
   && GET_CODE (XEXP (operands[0], 0)) == PLUS
   && rtx_equal_p (XEXP (XEXP (operands[0], 0), 0), stack_pointer_rtx)
   && CONST_INT_P (XEXP (XEXP (operands[0], 0), 1))
   && INTVAL (XEXP (XEXP (operands[0], 0), 1)) == 3)
    ? (int) 
#line 6395 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[1])
   && GET_CODE (XEXP (operands[0], 0)) == PLUS
   && rtx_equal_p (XEXP (XEXP (operands[0], 0), 0), stack_pointer_rtx)
   && CONST_INT_P (XEXP (XEXP (operands[0], 0), 1))
   && INTVAL (XEXP (XEXP (operands[0], 0), 1)) == 3)
    : -1 },
  { "(TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU) && ( reload_completed)",
    __builtin_constant_p (
#line 1396 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU) && 
#line 1398 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed))
    ? (int) (
#line 1396 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU) && 
#line 1398 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed))
    : -1 },
#line 621 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!(CONST_INT_P (operands[1]) && !IN_RANGE (INTVAL (operands[1]), 0, 31))",
    __builtin_constant_p 
#line 621 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!(CONST_INT_P (operands[1]) && !IN_RANGE (INTVAL (operands[1]), 0, 31)))
    ? (int) 
#line 621 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!(CONST_INT_P (operands[1]) && !IN_RANGE (INTVAL (operands[1]), 0, 31)))
    : -1 },
#line 2209 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68881 && !TUNE_68040",
    __builtin_constant_p 
#line 2209 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && !TUNE_68040)
    ? (int) 
#line 2209 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && !TUNE_68040)
    : -1 },
#line 832 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU",
    __builtin_constant_p 
#line 832 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU)
    ? (int) 
#line 832 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU)
    : -1 },
  { "(TARGET_68881 && !TUNE_68040) && (TARGET_68881)",
    __builtin_constant_p (
#line 2209 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && !TUNE_68040) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 2209 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && !TUNE_68040) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
#line 5982 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "SIBLING_CALL_P (insn)",
    __builtin_constant_p 
#line 5982 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(SIBLING_CALL_P (insn))
    ? (int) 
#line 5982 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(SIBLING_CALL_P (insn))
    : -1 },
#line 4347 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "ISA_HAS_FF1",
    __builtin_constant_p 
#line 4347 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_FF1)
    ? (int) 
#line 4347 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_FF1)
    : -1 },
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68881\n\
   && (register_operand (operands[1], DFmode)\n\
       || register_operand (operands[2], DFmode)\n\
       || const0_operand (operands[2], DFmode))",
    __builtin_constant_p 
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], DFmode)
       || register_operand (operands[2], DFmode)
       || const0_operand (operands[2], DFmode)))
    ? (int) 
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], DFmode)
       || register_operand (operands[2], DFmode)
       || const0_operand (operands[2], DFmode)))
    : -1 },
#line 1396 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU",
    __builtin_constant_p 
#line 1396 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU)
    ? (int) 
#line 1396 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU)
    : -1 },
#line 5178 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TUNE_68060",
    __builtin_constant_p 
#line 5178 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TUNE_68060)
    ? (int) 
#line 5178 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TUNE_68060)
    : -1 },
#line 656 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!(REG_P (operands[1]) && !IN_RANGE (INTVAL (operands[2]), 0, 31))",
    __builtin_constant_p 
#line 656 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!(REG_P (operands[1]) && !IN_RANGE (INTVAL (operands[2]), 0, 31)))
    ? (int) 
#line 656 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!(REG_P (operands[1]) && !IN_RANGE (INTVAL (operands[2]), 0, 31)))
    : -1 },
#line 5117 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "reload_completed && !TARGET_COLDFIRE\n\
   && INTVAL (operands[2]) > 40 && INTVAL (operands[2]) <= 62",
    __builtin_constant_p 
#line 5117 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 40 && INTVAL (operands[2]) <= 62)
    ? (int) 
#line 5117 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 40 && INTVAL (operands[2]) <= 62)
    : -1 },
#line 867 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TUNE_68000_10",
    __builtin_constant_p 
#line 867 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68000_10)
    ? (int) 
#line 867 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68000_10)
    : -1 },
#line 6599 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68881",
    __builtin_constant_p 
#line 6599 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)
    ? (int) 
#line 6599 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)
    : -1 },
#line 5086 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "reload_completed && !TARGET_COLDFIRE\n\
   && INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 40",
    __builtin_constant_p 
#line 5086 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 40)
    ? (int) 
#line 5086 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 40)
    : -1 },
#line 690 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD\n\
   && (!REG_P (operands[1]) || !CONST_INT_P (operands[3])\n\
       || IN_RANGE (INTVAL (operands[3]), 0, 31))",
    __builtin_constant_p 
#line 690 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (!REG_P (operands[1]) || !CONST_INT_P (operands[3])
       || IN_RANGE (INTVAL (operands[3]), 0, 31)))
    ? (int) 
#line 690 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (!REG_P (operands[1]) || !CONST_INT_P (operands[3])
       || IN_RANGE (INTVAL (operands[3]), 0, 31)))
    : -1 },
#line 3129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 || TARGET_COLDFIRE",
    __builtin_constant_p 
#line 3129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || TARGET_COLDFIRE)
    ? (int) 
#line 3129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || TARGET_COLDFIRE)
    : -1 },
#line 6201 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 || INTVAL (operands[1]) >= -0x8004",
    __builtin_constant_p 
#line 6201 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || INTVAL (operands[1]) >= -0x8004)
    ? (int) 
#line 6201 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || INTVAL (operands[1]) >= -0x8004)
    : -1 },
#line 707 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD\n\
   && (!REG_P (operands[2]) || !CONST_INT_P (operands[4])\n\
       || IN_RANGE (INTVAL (operands[4]), 0, 31))",
    __builtin_constant_p 
#line 707 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (!REG_P (operands[2]) || !CONST_INT_P (operands[4])
       || IN_RANGE (INTVAL (operands[4]), 0, 31)))
    ? (int) 
#line 707 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (!REG_P (operands[2]) || !CONST_INT_P (operands[4])
       || IN_RANGE (INTVAL (operands[4]), 0, 31)))
    : -1 },
#line 6185 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "m68k_movem_pattern_p (operands[0], operands[1],\n\
			 INTVAL (operands[3]), false)",
    __builtin_constant_p 
#line 6185 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], operands[1],
			 INTVAL (operands[3]), false))
    ? (int) 
#line 6185 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], operands[1],
			 INTVAL (operands[3]), false))
    : -1 },
  { "(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)) && (TARGET_68881)",
    __builtin_constant_p (
#line 734 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 734 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
#line 1708 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "(GET_MODE (operands[0]) != HImode || XEXP (XEXP (operands[0], 0), 0) != stack_pointer_rtx) &&\n\
   GET_MODE_CLASS (GET_MODE (operands[0])) == MODE_INT &&\n\
   GET_MODE_CLASS (GET_MODE (operands[1])) == MODE_INT &&\n\
   GET_MODE_SIZE (GET_MODE (operands[0])) == GET_MODE_SIZE (GET_MODE (operands[1])) * 2",
    __builtin_constant_p 
#line 1708 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((GET_MODE (operands[0]) != HImode || XEXP (XEXP (operands[0], 0), 0) != stack_pointer_rtx) &&
   GET_MODE_CLASS (GET_MODE (operands[0])) == MODE_INT &&
   GET_MODE_CLASS (GET_MODE (operands[1])) == MODE_INT &&
   GET_MODE_SIZE (GET_MODE (operands[0])) == GET_MODE_SIZE (GET_MODE (operands[1])) * 2)
    ? (int) 
#line 1708 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((GET_MODE (operands[0]) != HImode || XEXP (XEXP (operands[0], 0), 0) != stack_pointer_rtx) &&
   GET_MODE_CLASS (GET_MODE (operands[0])) == MODE_INT &&
   GET_MODE_CLASS (GET_MODE (operands[1])) == MODE_INT &&
   GET_MODE_SIZE (GET_MODE (operands[0])) == GET_MODE_SIZE (GET_MODE (operands[1])) * 2)
    : -1 },
#line 5189 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TUNE_68000_10\n\
   && INTVAL (operands[2]) > 16\n\
   && INTVAL (operands[2]) <= 24",
    __builtin_constant_p 
#line 5189 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68000_10
   && INTVAL (operands[2]) > 16
   && INTVAL (operands[2]) <= 24)
    ? (int) 
#line 5189 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68000_10
   && INTVAL (operands[2]) > 16
   && INTVAL (operands[2]) <= 24)
    : -1 },
#line 6809 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "INTVAL (operands[1]) <= 255\n\
   && peep2_reg_dead_p (1, operands[0])\n\
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)\n\
   && (optimize_size || TUNE_68040_60)\n\
   && DATA_REG_P (operands[0])",
    __builtin_constant_p 
#line 6809 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   && peep2_reg_dead_p (1, operands[0])
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[0]))
    ? (int) 
#line 6809 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   && peep2_reg_dead_p (1, operands[0])
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[0]))
    : -1 },
#line 1518 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "! TARGET_68881 && ! TARGET_COLDFIRE",
    __builtin_constant_p 
#line 1518 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_68881 && ! TARGET_COLDFIRE)
    ? (int) 
#line 1518 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_68881 && ! TARGET_COLDFIRE)
    : -1 },
#line 847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "INTVAL (operands[1]) >= -0x8000 && INTVAL (operands[1]) < 0x8000",
    __builtin_constant_p 
#line 847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) >= -0x8000 && INTVAL (operands[1]) < 0x8000)
    ? (int) 
#line 847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) >= -0x8000 && INTVAL (operands[1]) < 0x8000)
    : -1 },
#line 1995 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 || (TARGET_COLDFIRE && !ISA_HAS_MVS_MVZ)",
    __builtin_constant_p 
#line 1995 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || (TARGET_COLDFIRE && !ISA_HAS_MVS_MVZ))
    ? (int) 
#line 1995 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || (TARGET_COLDFIRE && !ISA_HAS_MVS_MVZ))
    : -1 },
#line 2192 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68881 && TUNE_68040",
    __builtin_constant_p 
#line 2192 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TUNE_68040)
    ? (int) 
#line 2192 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TUNE_68040)
    : -1 },
#line 6866 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "peep2_reg_dead_p (1, operands[0])\n\
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)\n\
   && (optimize_size || TUNE_68000_10)\n\
   && DATA_REG_P (operands[0])",
    __builtin_constant_p 
#line 6866 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(peep2_reg_dead_p (1, operands[0])
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)
   && (optimize_size || TUNE_68000_10)
   && DATA_REG_P (operands[0]))
    ? (int) 
#line 6866 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(peep2_reg_dead_p (1, operands[0])
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)
   && (optimize_size || TUNE_68000_10)
   && DATA_REG_P (operands[0]))
    : -1 },
#line 5374 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TARGET_COLDFIRE",
    __builtin_constant_p 
#line 5374 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE)
    ? (int) 
#line 5374 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE)
    : -1 },
#line 6448 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "REGNO (operands[0]) == REGNO (operands[1])\n\
   && strict_low_part_peephole_ok (QImode, insn, operands[0])",
    __builtin_constant_p 
#line 6448 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(REGNO (operands[0]) == REGNO (operands[1])
   && strict_low_part_peephole_ok (QImode, insn, operands[0]))
    ? (int) 
#line 6448 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(REGNO (operands[0]) == REGNO (operands[1])
   && strict_low_part_peephole_ok (QImode, insn, operands[0]))
    : -1 },
#line 5773 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "! TARGET_COLDFIRE",
    __builtin_constant_p 
#line 5773 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_COLDFIRE)
    ? (int) 
#line 5773 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_COLDFIRE)
    : -1 },
#line 6175 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "m68k_movem_pattern_p (operands[0], NULL, 0, false)",
    __builtin_constant_p 
#line 6175 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], NULL, 0, false))
    ? (int) 
#line 6175 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], NULL, 0, false))
    : -1 },
  { "(TARGET_HARD_FLOAT && !TUNE_68040) && (TARGET_68881)",
    __builtin_constant_p (
#line 2203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !TUNE_68040) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    ? (int) (
#line 2203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !TUNE_68040) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
    : -1 },
#line 5562 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "TARGET_68020 && TARGET_BITFIELD\n\
   && (INTVAL (operands[2]) % 8) == 0\n\
   && ! mode_dependent_address_p (XEXP (operands[1], 0),\n\
                                  MEM_ADDR_SPACE (operands[1]))",
    __builtin_constant_p 
#line 5562 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[2]) % 8) == 0
   && ! mode_dependent_address_p (XEXP (operands[1], 0),
                                  MEM_ADDR_SPACE (operands[1])))
    ? (int) 
#line 5562 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[2]) % 8) == 0
   && ! mode_dependent_address_p (XEXP (operands[1], 0),
                                  MEM_ADDR_SPACE (operands[1])))
    : -1 },
#line 639 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TARGET_COLDFIRE && (unsigned) INTVAL (operands[2]) < 8",
    __builtin_constant_p 
#line 639 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && (unsigned) INTVAL (operands[2]) < 8)
    ? (int) 
#line 639 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && (unsigned) INTVAL (operands[2]) < 8)
    : -1 },
#line 4331 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "ISA_HAS_FF1 || (TARGET_68020 && TARGET_BITFIELD)",
    __builtin_constant_p 
#line 4331 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_FF1 || (TARGET_68020 && TARGET_BITFIELD))
    ? (int) 
#line 4331 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_FF1 || (TARGET_68020 && TARGET_BITFIELD))
    : -1 },
#line 3623 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
  { "!TARGET_COLDFIRE || TARGET_CF_HWDIV",
    __builtin_constant_p 
#line 3623 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE || TARGET_CF_HWDIV)
    ? (int) 
#line 3623 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE || TARGET_CF_HWDIV)
    : -1 },

};
#endif /* gcc >= 3.0.1 */

int
main(void)
{
  unsigned int i;
  const char *p;
  puts ("(define_conditions [");
#if GCC_VERSION >= 3001
  for (i = 0; i < ARRAY_SIZE (insn_conditions); i++)
    {
      printf ("  (%d \"", insn_conditions[i].value);
      for (p = insn_conditions[i].expr; *p; p++)
        {
          switch (*p)
	     {
	     case '\\':
	     case '\"': putchar ('\\'); break;
	     default: break;
	     }
          putchar (*p);
        }
      puts ("\")");
    }
#endif /* gcc >= 3.0.1 */
  puts ("])");
  fflush (stdout);
return ferror (stdout) != 0 ? FATAL_EXIT_CODE : SUCCESS_EXIT_CODE;
}
