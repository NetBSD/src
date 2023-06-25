/* Generated automatically by the program 'build/genpreds'
   from the machine description file '/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md'.  */

#ifndef GCC_TM_CONSTRS_H
#define GCC_TM_CONSTRS_H

static inline bool
satisfies_constraint_m (rtx op)
{
  return (GET_CODE (op) == MEM) && (
#line 26 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(memory_address_addr_space_p (GET_MODE (op), XEXP (op, 0),
						 MEM_ADDR_SPACE (op))));
}
static inline bool
satisfies_constraint_o (rtx op)
{
  return (GET_CODE (op) == MEM) && (
#line 32 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(offsettable_nonstrict_memref_p (op)));
}
static inline bool
satisfies_constraint_V (rtx op)
{
  return (GET_CODE (op) == MEM) && ((
#line 41 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(memory_address_addr_space_p (GET_MODE (op), XEXP (op, 0),
						 MEM_ADDR_SPACE (op)))) && (!(
#line 43 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(offsettable_nonstrict_memref_p (op)))));
}
static inline bool
satisfies_constraint__l (rtx op)
{
  return (GET_CODE (op) == MEM) && ((
#line 50 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(GET_CODE (XEXP (op, 0)) == PRE_DEC)) || (
#line 51 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(GET_CODE (XEXP (op, 0)) == POST_DEC)));
}
static inline bool
satisfies_constraint__g (rtx op)
{
  return (GET_CODE (op) == MEM) && ((
#line 57 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(GET_CODE (XEXP (op, 0)) == PRE_INC)) || (
#line 58 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(GET_CODE (XEXP (op, 0)) == POST_INC)));
}
static inline bool
satisfies_constraint_p (rtx ARG_UNUSED (op))
{
  return 
#line 62 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(address_operand (op, VOIDmode));
}
static inline bool
satisfies_constraint_i (rtx op)
{
  return (
#line 66 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(CONSTANT_P (op))) && (
#line 67 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(!flag_pic || LEGITIMATE_PIC_OPERAND_P (op)));
}
static inline bool
satisfies_constraint_s (rtx op)
{
  return (
#line 71 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(CONSTANT_P (op))) && ((
#line 72 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(!CONST_SCALAR_INT_P (op))) && (
#line 73 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(!flag_pic || LEGITIMATE_PIC_OPERAND_P (op))));
}
static inline bool
satisfies_constraint_n (rtx op)
{
  return (
#line 77 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(CONST_SCALAR_INT_P (op))) && (
#line 78 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(!flag_pic || LEGITIMATE_PIC_OPERAND_P (op)));
}
static inline bool
satisfies_constraint_E (rtx op)
{
  return (
#line 82 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(CONST_DOUBLE_AS_FLOAT_P (op))) || (
#line 83 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(GET_CODE (op) == CONST_VECTOR
		    && GET_MODE_CLASS (GET_MODE (op)) == MODE_VECTOR_FLOAT));
}
static inline bool
satisfies_constraint_F (rtx op)
{
  return (
#line 89 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(CONST_DOUBLE_AS_FLOAT_P (op))) || (
#line 90 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(GET_CODE (op) == CONST_VECTOR
		    && GET_MODE_CLASS (GET_MODE (op)) == MODE_VECTOR_FLOAT));
}
static inline bool
satisfies_constraint_X (rtx ARG_UNUSED (op))
{
  return 
#line 95 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/common.md"
(true);
}
static inline bool
satisfies_constraint_I (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 32 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival > 0 && ival <= 8));
}
static inline bool
satisfies_constraint_J (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 37 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival >= -0x8000 && ival <= 0x7fff));
}
static inline bool
satisfies_constraint_K (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 42 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival < -0x80 || ival >= 0x80));
}
static inline bool
satisfies_constraint_L (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 47 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival < 0 && ival >= -8));
}
static inline bool
satisfies_constraint_M (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 52 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival < -0x100 || ival >= 0x100));
}
static inline bool
satisfies_constraint_N (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 57 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival >= 24 && ival <= 31));
}
static inline bool
satisfies_constraint_O (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 62 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival == 16));
}
static inline bool
satisfies_constraint_P (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 67 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival >= 8 && ival <= 15));
}
static inline bool
satisfies_constraint_R (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 72 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(valid_mov3q_const (ival)));
}
static inline bool
satisfies_constraint_G (rtx op)
{
  return (GET_CODE (op) == CONST_DOUBLE) && (
#line 79 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(!(TARGET_68881 && standard_68881_constant_p (op))));
}
static inline bool
satisfies_constraint_H (rtx op)
{
  return (GET_CODE (op) == CONST_DOUBLE) && (
#line 84 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(op == CONST0_RTX (GET_MODE (op))));
}
static inline bool
satisfies_constraint_S (rtx op)
{
  return (GET_CODE (op) == MEM) && (
#line 89 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(TARGET_PCREL
		    && (GET_CODE (XEXP (op, 0)) == SYMBOL_REF
			|| GET_CODE (XEXP (op, 0)) == LABEL_REF
			|| GET_CODE (XEXP (op, 0)) == CONST)));
}
static inline bool
satisfies_constraint_T (rtx op)
{
  switch (GET_CODE (op))
    {
    case SYMBOL_REF:
    case LABEL_REF:
    case CONST:
      break;
    default:
      return false;
    }
  return (
#line 97 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(!TARGET_PCREL)) && (
#line 98 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(!flag_pic || LEGITIMATE_PIC_OPERAND_P (op)));
}
static inline bool
satisfies_constraint_Q (rtx op)
{
  return (GET_CODE (op) == MEM) && (
#line 103 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(m68k_matches_q_p (op)));
}
static inline bool
satisfies_constraint_U (rtx op)
{
  return (GET_CODE (op) == MEM) && (
#line 108 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(m68k_matches_u_p (op)));
}
static inline bool
satisfies_constraint_W (rtx op)
{
  machine_mode mode = GET_MODE (op);
  return const_call_operand (op, mode);
}
static inline bool
satisfies_constraint_Cs (rtx op)
{
  switch (GET_CODE (op))
    {
    case SYMBOL_REF:
    case CONST:
      return true;
    default:
      break;
    }
  return false;
}
static inline bool
satisfies_constraint_Ci (rtx op)
{
  return (GET_CODE (op) == CONST_INT) && (
#line 121 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(true));
}
static inline bool
satisfies_constraint_C0 (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 126 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival == 0));
}
static inline bool
satisfies_constraint_Cj (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 131 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(ival < -0x8000 || ival > 0x7FFF));
}
static inline bool
satisfies_constraint_Cu (rtx op)
{
  return (GET_CODE (op) == CONST) && (
#line 136 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(m68k_unwrap_symbol (op, false) != op));
}
static inline bool
satisfies_constraint_CQ (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 141 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(m68k_const_method (ival) == MOVQ));
}
static inline bool
satisfies_constraint_CW (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 146 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(m68k_const_method (ival) == SWAP));
}
static inline bool
satisfies_constraint_CZ (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 151 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(m68k_const_method (ival) == MVZ));
}
static inline bool
satisfies_constraint_CS (rtx op)
{
  HOST_WIDE_INT ival = 0;
  if (CONST_INT_P (op))
    ival = INTVAL (op);
  return (GET_CODE (op) == CONST_INT) && (
#line 156 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(m68k_const_method (ival) == MVS));
}
static inline bool
satisfies_constraint_Ap (rtx op)
{
  machine_mode mode = GET_MODE (op);
  return push_operand (op, mode);
}
static inline bool
satisfies_constraint_Ac (rtx op)
{
  machine_mode mode = GET_MODE (op);
  return (movsi_const0_operand (op, mode)) && (
#line 165 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/constraints.md"
(!REG_P (op)));
}
#endif /* tm-constrs.h */
