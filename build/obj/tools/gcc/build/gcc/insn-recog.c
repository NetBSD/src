/* Generated automatically by the program `genrecog' from the target
   machine description file.  */

#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "predict.h"
#include "rtl.h"
#include "memmodel.h"
#include "tm_p.h"
#include "emit-rtl.h"
#include "insn-config.h"
#include "recog.h"
#include "output.h"
#include "flags.h"
#include "df.h"
#include "resource.h"
#include "diagnostic-core.h"
#include "reload.h"
#include "regs.h"
#include "tm-constrs.h"



/* `recog' contains a decision tree that recognizes whether the rtx
   X0 is a valid instruction.

   recog returns -1 if the rtx is not valid.  If the rtx is valid, recog
   returns a nonnegative number which is the insn code number for the
   pattern that matched.  This is the same as the order in the machine
   description of the entry that matched.  This number can be used as an
   index into `insn_data' and other tables.

   The third parameter to recog is an optional pointer to an int.  If
   present, recog will accept a pattern if it matches except for missing
   CLOBBER expressions at the end.  In that case, the value pointed to by
   the optional pointer will be set to the number of CLOBBERs that need
   to be added (it should be initialized to zero by the caller).  If it
   is set nonzero, the caller should allocate a PARALLEL of the
   appropriate size, copy the initial entries, and call add_clobbers
   (found in insn-emit.c) to fill in the CLOBBERs.


   The function split_insns returns 0 if the rtl could not
   be split or the split rtl as an INSN list if it can be.

   The function peephole2_insns returns 0 if the rtl could not
   be matched. If there was a match, the new rtl is returned in an INSN list,
   and LAST_INSN will point to the last recognized insn in the old sequence.
*/


extern rtx_insn *gen_split_1 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_2 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_3 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_4 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_5 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_6 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_7 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_8 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_9 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_10 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_11 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_12 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_13 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_14 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_15 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_16 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_17 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_18 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_19 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_20 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_21 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_22 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_23 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_24 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_25 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_26 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_27 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_28 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_29 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_30 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_31 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_32 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_33 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_34 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_35 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_36 (rtx_insn *, rtx *);
extern rtx_insn *gen_split_37 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_1 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_2 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_3 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_4 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_5 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_6 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_7 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_8 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_9 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_10 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_11 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_12 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_13 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_14 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_15 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_16 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_17 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_18 (rtx_insn *, rtx *);
extern rtx_insn *gen_peephole2_19 (rtx_insn *, rtx *);




static int
pattern0 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  int res ATTRIBUTE_UNUSED;
  switch (GET_MODE (operands[0]))
    {
    case E_HImode:
      if (!nonimmediate_operand (operands[0], E_HImode)
          || GET_MODE (x1) != E_HImode)
        return -1;
      return 0;

    case E_QImode:
      if (!nonimmediate_operand (operands[0], E_QImode)
          || GET_MODE (x1) != E_QImode)
        return -1;
      return 1;

    default:
      return -1;
    }
}

static int
pattern1 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  switch (GET_CODE (x2))
    {
    case REG:
    case SUBREG:
    case MEM:
      operands[0] = x2;
      x3 = XEXP (x1, 1);
      x4 = XEXP (x3, 0);
      operands[1] = x4;
      return 0;

    case STRICT_LOW_PART:
      x5 = XEXP (x2, 0);
      operands[0] = x5;
      x3 = XEXP (x1, 1);
      x4 = XEXP (x3, 0);
      if (!rtx_equal_p (x4, operands[0]))
        return -1;
      res = pattern0 (x3);
      if (res >= 0)
        return res + 1; /* [1, 2] */
      return -1;

    default:
      return -1;
    }
}

static int
pattern2 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  operands[1] = x4;
  switch (GET_MODE (operands[0]))
    {
    case E_SFmode:
      if (!nonimmediate_operand (operands[0], E_SFmode)
          || GET_MODE (x3) != E_SFmode
          || !general_operand (operands[1], E_SFmode))
        return -1;
      return 0;

    case E_DFmode:
      if (!nonimmediate_operand (operands[0], E_DFmode)
          || GET_MODE (x3) != E_DFmode
          || !general_operand (operands[1], E_DFmode))
        return -1;
      return 1;

    case E_XFmode:
      if (!nonimmediate_operand (operands[0], E_XFmode)
          || GET_MODE (x3) != E_XFmode
          || !general_operand (operands[1], E_XFmode))
        return -1;
      return 2;

    default:
      return -1;
    }
}

static int
pattern3 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) != E_SImode)
    return -1;
  x4 = XEXP (x3, 0);
  if (x4 != const_int_rtx[MAX_SAVED_CONST_INT + 1])
    return -1;
  x5 = XEXP (x3, 1);
  if (GET_CODE (x5) != AND
      || GET_MODE (x5) != E_SImode)
    return -1;
  x6 = XEXP (x5, 1);
  if (x6 != const_int_rtx[MAX_SAVED_CONST_INT + 31])
    return -1;
  x7 = XEXP (x1, 0);
  operands[0] = x7;
  if (!register_operand (operands[0], E_SImode)
      || GET_MODE (x2) != E_SImode)
    return -1;
  x8 = XEXP (x5, 0);
  operands[1] = x8;
  if (!register_operand (operands[1], E_SImode))
    return -1;
  x9 = XEXP (x2, 1);
  operands[2] = x9;
  if (!register_operand (operands[2], E_SImode))
    return -1;
  return 0;
}

static int
pattern4 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  int res ATTRIBUTE_UNUSED;
  if (!register_operand (operands[0], i1)
      || GET_MODE (x1) != i1
      || !register_operand (operands[1], i1)
      || !general_operand (operands[2], i1))
    return -1;
  return 0;
}

static int
pattern5 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  operands[1] = x4;
  x5 = XEXP (x3, 1);
  operands[2] = x5;
  switch (GET_MODE (operands[0]))
    {
    case E_SImode:
      return pattern4 (x3, E_SImode); /* [-1, 0] */

    case E_HImode:
      if (pattern4 (x3, E_HImode) != 0)
        return -1;
      return 1;

    case E_QImode:
      if (pattern4 (x3, E_QImode) != 0)
        return -1;
      return 2;

    default:
      return -1;
    }
}

static int
pattern6 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 0);
  operands[0] = x3;
  x4 = XEXP (x1, 1);
  x5 = XEXP (x4, 1);
  operands[1] = x5;
  x6 = XEXP (x4, 0);
  if (!rtx_equal_p (x6, operands[0]))
    return -1;
  switch (GET_MODE (operands[0]))
    {
    case E_HImode:
      if (!register_operand (operands[0], E_HImode)
          || GET_MODE (x4) != E_HImode
          || !general_operand (operands[1], E_HImode))
        return -1;
      return 0;

    case E_QImode:
      if (!register_operand (operands[0], E_QImode)
          || GET_MODE (x4) != E_QImode
          || !general_operand (operands[1], E_QImode))
        return -1;
      return 1;

    default:
      return -1;
    }
}

static int
pattern7 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  if (!register_operand (operands[0], i1))
    return -1;
  x2 = XVECEXP (x1, 0, 0);
  x3 = XEXP (x2, 1);
  if (GET_MODE (x3) != i1
      || !general_operand (operands[1], i1)
      || !general_src_operand (operands[2], i1)
      || !register_operand (operands[3], i1))
    return -1;
  x4 = XVECEXP (x1, 0, 1);
  x5 = XEXP (x4, 1);
  if (GET_MODE (x5) != i1)
    return -1;
  return 0;
}

static int
pattern8 (rtx x1, rtx_code i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11;
  int res ATTRIBUTE_UNUSED;
  x2 = XVECEXP (x1, 0, 1);
  if (GET_CODE (x2) != SET)
    return -1;
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) != i1)
    return -1;
  x4 = XVECEXP (x1, 0, 0);
  x5 = XEXP (x4, 0);
  operands[0] = x5;
  x6 = XEXP (x4, 1);
  x7 = XEXP (x6, 0);
  operands[1] = x7;
  x8 = XEXP (x6, 1);
  operands[2] = x8;
  x9 = XEXP (x2, 0);
  operands[3] = x9;
  x10 = XEXP (x3, 0);
  if (!rtx_equal_p (x10, operands[1]))
    return -1;
  x11 = XEXP (x3, 1);
  if (!rtx_equal_p (x11, operands[2]))
    return -1;
  switch (GET_MODE (operands[0]))
    {
    case E_SImode:
      return pattern7 (x1, E_SImode); /* [-1, 0] */

    case E_HImode:
      if (pattern7 (x1, E_HImode) != 0)
        return -1;
      return 1;

    default:
      return -1;
    }
}

static int
pattern9 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) != PLUS
      || GET_MODE (x2) != E_SImode)
    return -1;
  x3 = XEXP (x1, 0);
  operands[1] = x3;
  if (!register_operand (operands[1], E_SImode))
    return -1;
  x4 = XEXP (x2, 0);
  operands[2] = x4;
  if (!register_operand (operands[2], E_SImode))
    return -1;
  x5 = XEXP (x2, 1);
  operands[3] = x5;
  if (!const_int_operand (operands[3], E_SImode))
    return -1;
  return 0;
}

static int
pattern10 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  int res ATTRIBUTE_UNUSED;
  if (!nonimmediate_operand (operands[0], i1)
      || GET_MODE (x1) != i1
      || !general_operand (operands[1], i1)
      || !general_operand (operands[2], i1))
    return -1;
  return 0;
}

static int
pattern11 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  int res ATTRIBUTE_UNUSED;
  if (!nonimmediate_operand (operands[0], i1)
      || GET_MODE (x1) != i1
      || !general_operand (operands[1], i1)
      || !general_src_operand (operands[2], i1))
    return -1;
  return 0;
}

static int
pattern12 (rtx x1, int *pnum_clobbers)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  operands[1] = x4;
  x5 = XEXP (x3, 1);
  operands[2] = x5;
  switch (GET_MODE (operands[0]))
    {
    case E_DImode:
      if (pnum_clobbers == NULL
          || pattern10 (x3, E_DImode) != 0)
        return -1;
      return 0;

    case E_HImode:
      if (pattern11 (x3, E_HImode) != 0)
        return -1;
      return 1;

    case E_QImode:
      if (pattern11 (x3, E_QImode) != 0)
        return -1;
      return 2;

    case E_SFmode:
      if (pattern10 (x3, E_SFmode) != 0)
        return -1;
      return 3;

    case E_DFmode:
      if (pattern10 (x3, E_DFmode) != 0)
        return -1;
      return 4;

    case E_XFmode:
      if (pattern10 (x3, E_XFmode) != 0)
        return -1;
      return 5;

    default:
      return -1;
    }
}

static int
pattern13 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2;
  int res ATTRIBUTE_UNUSED;
  if (!nonimmediate_operand (operands[0], i1)
      || GET_MODE (x1) != i1)
    return -1;
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) != i1
      || !general_operand (operands[1], i1))
    return -1;
  return 0;
}

static int
pattern14 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 0);
  operands[2] = x3;
  x4 = XEXP (x1, 1);
  operands[1] = x4;
  switch (GET_MODE (operands[0]))
    {
    case E_SFmode:
      return pattern13 (x1, E_SFmode); /* [-1, 0] */

    case E_DFmode:
      if (pattern13 (x1, E_DFmode) != 0)
        return -1;
      return 1;

    case E_XFmode:
      if (pattern13 (x1, E_XFmode) != 0)
        return -1;
      return 2;

    default:
      return -1;
    }
}

static int
pattern15 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2;
  int res ATTRIBUTE_UNUSED;
  if (!nonimmediate_operand (operands[0], i1)
      || GET_MODE (x1) != i1
      || !general_operand (operands[1], i1))
    return -1;
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) != i1)
    return -1;
  return 0;
}

static int
pattern16 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  operands[2] = x3;
  switch (GET_MODE (operands[0]))
    {
    case E_SFmode:
      return pattern15 (x1, E_SFmode); /* [-1, 0] */

    case E_DFmode:
      if (pattern15 (x1, E_DFmode) != 0)
        return -1;
      return 1;

    case E_XFmode:
      if (pattern15 (x1, E_XFmode) != 0)
        return -1;
      return 2;

    default:
      return -1;
    }
}

static int
pattern17 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 1);
  if (x3 != const_int_rtx[MAX_SAVED_CONST_INT + 32])
    return -1;
  x4 = XEXP (x1, 0);
  operands[0] = x4;
  if (!nonimmediate_operand (operands[0], E_DImode)
      || GET_MODE (x2) != E_DImode)
    return -1;
  x5 = XEXP (x2, 0);
  x6 = XEXP (x5, 0);
  operands[1] = x6;
  if (!general_operand (operands[1], i1))
    return -1;
  return 0;
}

static int
pattern18 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2;
  int res ATTRIBUTE_UNUSED;
  x2 = XVECEXP (x1, 0, 0);
  operands[1] = x2;
  switch (GET_MODE (operands[0]))
    {
    case E_SFmode:
      if (!nonimmediate_operand (operands[0], E_SFmode)
          || GET_MODE (x1) != E_SFmode
          || !general_operand (operands[1], E_SFmode))
        return -1;
      return 0;

    case E_DFmode:
      if (!nonimmediate_operand (operands[0], E_DFmode)
          || GET_MODE (x1) != E_DFmode
          || !general_operand (operands[1], E_DFmode))
        return -1;
      return 1;

    case E_XFmode:
      if (!nonimmediate_operand (operands[0], E_XFmode)
          || GET_MODE (x1) != E_XFmode
          || !general_operand (operands[1], E_XFmode))
        return -1;
      return 2;

    default:
      return -1;
    }
}

static int
pattern19 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[1] = x2;
  if (!general_operand (operands[1], E_SImode))
    return -1;
  x3 = XEXP (x1, 1);
  switch (GET_CODE (x3))
    {
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
      operands[2] = x3;
      if (!general_src_operand (operands[2], E_SImode))
        return -1;
      return 0;

    case SIGN_EXTEND:
      if (GET_MODE (x3) != E_SImode)
        return -1;
      x4 = XEXP (x3, 0);
      operands[2] = x4;
      if (!nonimmediate_src_operand (operands[2], E_HImode))
        return -1;
      return 1;

    default:
      return -1;
    }
}

static int
pattern20 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  int res ATTRIBUTE_UNUSED;
  if (!nonimmediate_operand (operands[0], i1)
      || GET_MODE (x1) != i1)
    return -1;
  switch (GET_MODE (operands[1]))
    {
    case E_SFmode:
      if (!general_operand (operands[1], E_SFmode))
        return -1;
      return 0;

    case E_DFmode:
      if (!general_operand (operands[1], E_DFmode))
        return -1;
      return 1;

    case E_XFmode:
      if (!general_operand (operands[1], E_XFmode))
        return -1;
      return 2;

    default:
      return -1;
    }
}

static int
pattern21 (rtx x1, int i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7;
  int res ATTRIBUTE_UNUSED;
  x2 = XVECEXP (x1, 0, 0);
  x3 = XEXP (x2, 1);
  x4 = XEXP (x3, 0);
  x5 = XEXP (x4, 1);
  if (x5 != const_int_rtx[MAX_SAVED_CONST_INT + i1])
    return -1;
  x6 = XVECEXP (x1, 0, 1);
  if (GET_CODE (x6) != CLOBBER)
    return -1;
  x7 = XEXP (x2, 0);
  operands[0] = x7;
  if (!nonimmediate_operand (operands[0], E_DImode)
      || GET_MODE (x3) != E_DImode)
    return -1;
  return 0;
}

static int
pattern22 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  int res ATTRIBUTE_UNUSED;
  if (!register_operand (operands[0], E_SImode)
      || GET_MODE (x1) != E_SImode
      || !register_operand (operands[1], E_SImode)
      || !const_int_operand (operands[2], E_SImode))
    return -1;
  return 0;
}

static int
pattern23 (rtx x1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  int res ATTRIBUTE_UNUSED;
  if (!register_operand (operands[0], E_DImode)
      || GET_MODE (x1) != E_DImode
      || !register_operand (operands[1], E_DImode)
      || !const_int_operand (operands[2], E_VOIDmode))
    return -1;
  return 0;
}

static int
pattern24 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2;
  int res ATTRIBUTE_UNUSED;
  if (!nonimmediate_operand (operands[0], i1)
      || GET_MODE (x1) != i1)
    return -1;
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) != i1
      || !general_operand (operands[2], i1))
    return -1;
  return 0;
}

static int
pattern25 ()
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  int res ATTRIBUTE_UNUSED;
  switch (GET_MODE (operands[1]))
    {
    case E_SFmode:
      if (!fp_src_operand (operands[1], E_SFmode)
          || !fp_src_operand (operands[2], E_SFmode))
        return -1;
      return 0;

    case E_DFmode:
      if (!fp_src_operand (operands[1], E_DFmode)
          || !fp_src_operand (operands[2], E_DFmode))
        return -1;
      return 1;

    case E_XFmode:
      if (!fp_src_operand (operands[1], E_XFmode)
          || !fp_src_operand (operands[2], E_XFmode))
        return -1;
      return 2;

    default:
      return -1;
    }
}

static int
pattern26 ()
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  int res ATTRIBUTE_UNUSED;
  switch (GET_MODE (operands[1]))
    {
    case E_QImode:
      if (!nonimmediate_operand (operands[1], E_QImode))
        return -1;
      return 0;

    case E_HImode:
      if (!nonimmediate_operand (operands[1], E_HImode))
        return -1;
      return 1;

    case E_SImode:
      if (!nonimmediate_operand (operands[1], E_SImode)
          || !general_operand (operands[2], E_SImode))
        return -1;
      return 2;

    default:
      return -1;
    }
}

static int
pattern27 (rtx x1, machine_mode i1)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  if (!register_operand (operands[1], i1))
    return -1;
  x2 = XVECEXP (x1, 0, 0);
  x3 = XEXP (x2, 1);
  if (GET_MODE (x3) != i1
      || !memory_operand (operands[2], i1)
      || !register_operand (operands[3], i1)
      || !register_operand (operands[4], i1))
    return -1;
  x4 = XVECEXP (x1, 0, 1);
  x5 = XEXP (x4, 1);
  if (GET_MODE (x5) != i1)
    return -1;
  return 0;
}

static int
recog_1 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  operands[1] = x3;
  switch (GET_MODE (operands[0]))
    {
    case E_DFmode:
      if (!general_operand (operands[1], E_DFmode))
        return -1;
      if (push_operand (operands[0], E_DFmode))
        return 1; /* *movdf_internal */
      if (!nonimmediate_operand (operands[0], E_DFmode))
        return -1;
      if (
#line 1360 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return 68; /* *m68k.md:1355 */
      if (
#line 1396 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU))
        return 69; /* movdf_cf_soft */
      if (!
#line 1408 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
        return -1;
      return 70; /* movdf_cf_hard */

    case E_DImode:
      if (!general_operand (operands[1], E_DImode))
        return -1;
      if (push_operand (operands[0], E_DImode))
        return 2; /* pushdi */
      if (!nonimmediate_operand (operands[0], E_DImode))
        return -1;
      if (
#line 1576 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return 74; /* *m68k.md:1568 */
      if (!
#line 1611 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
        return -1;
      return 75; /* *m68k.md:1608 */

    case E_SImode:
      if (!push_operand (operands[0], E_SImode)
          || !const_int_operand (operands[1], E_SImode)
          || !
#line 847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) >= -0x8000 && INTVAL (operands[1]) < 0x8000))
        return -1;
      return 49; /* pushexthisi_const */

    case E_HImode:
      if (!nonimmediate_operand (operands[0], E_HImode))
        return -1;
      if (general_src_operand (operands[1], E_HImode)
          && 
#line 1087 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return 57; /* *m68k.md:1084 */
      if (!general_operand (operands[1], E_HImode)
          || !
#line 1097 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
        return -1;
      return 58; /* *m68k.md:1094 */

    case E_QImode:
      if (!nonimmediate_operand (operands[0], E_QImode)
          || !general_src_operand (operands[1], E_QImode))
        return -1;
      if (
#line 1133 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return 61; /* *m68k.md:1130 */
      if (!
#line 1140 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
        return -1;
      return 62; /* *m68k.md:1137 */

    case E_SFmode:
      if (!nonimmediate_operand (operands[0], E_SFmode)
          || !general_operand (operands[1], E_SFmode))
        return -1;
      if (
#line 1212 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return 65; /* *m68k.md:1209 */
      if (
#line 1255 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU))
        return 66; /* movsf_cf_soft */
      if (!
#line 1266 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
        return -1;
      return 67; /* movsf_cf_hard */

    case E_XFmode:
      if (!nonimmediate_operand (operands[0], E_XFmode)
          || !nonimmediate_operand (operands[1], E_XFmode))
        return -1;
      if (
#line 1478 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
        return 71; /* *m68k.md:1475 */
      if (
#line 1518 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_68881 && ! TARGET_COLDFIRE))
        return 72; /* *m68k.md:1515 */
      if (!
#line 1557 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_68881 && TARGET_COLDFIRE))
        return -1;
      return 73; /* *m68k.md:1554 */

    default:
      return -1;
    }
}

static int
recog_2 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 0);
  operands[0] = x3;
  x4 = XEXP (x1, 1);
  operands[1] = x4;
  switch (GET_MODE (operands[0]))
    {
    case E_HImode:
      if (!nonimmediate_operand (operands[0], E_HImode)
          || !general_src_operand (operands[1], E_HImode))
        return -1;
      if (
#line 1113 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return 59; /* *m68k.md:1110 */
      if (!
#line 1120 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
        return -1;
      return 60; /* *m68k.md:1117 */

    case E_QImode:
      if (!nonimmediate_operand (operands[0], E_QImode)
          || !general_src_operand (operands[1], E_QImode))
        return -1;
      if (
#line 1153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return 63; /* *m68k.md:1150 */
      if (!
#line 1160 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
        return -1;
      return 64; /* *movstrictqi_cf */

    default:
      return -1;
    }
}

static int
recog_3 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 2);
  if (!const_int_operand (x3, E_SImode))
    return -1;
  operands[1] = x3;
  x4 = XEXP (x2, 1);
  if (XWINT (x4, 0) ==  HOST_WIDE_INT_C (32)
      && memory_operand (operands[0], E_QImode))
    {
      x5 = XEXP (x1, 1);
      operands[2] = x5;
      if (general_src_operand (operands[2], E_SImode)
          && 
#line 5478 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[1]) % 8) == 0
   && ! mode_dependent_address_p (XEXP (operands[0], 0),
                                  MEM_ADDR_SPACE (operands[0]))))
        return 360; /* *insv_32_mem */
    }
  operands[2] = x3;
  if (!register_operand (operands[0], E_SImode))
    return -1;
  operands[1] = x4;
  if (!const_int_operand (operands[1], E_SImode))
    return -1;
  x5 = XEXP (x1, 1);
  operands[3] = x5;
  if (!register_operand (operands[3], E_SImode))
    return -1;
  if (
#line 5494 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && IN_RANGE (INTVAL (operands[2]), 0, 31)
   && (INTVAL (operands[1]) == 8 || INTVAL (operands[1]) == 16)
   && INTVAL (operands[2]) % INTVAL (operands[1]) == 0))
    return 361; /* *insv_8_16_reg */
  if (!
#line 5736 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[2]), 0, 31)))
    return -1;
  return 376; /* *insv_bfins_reg */
}

static int
recog_4 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) != E_SImode)
    return -1;
  x3 = XEXP (x2, 0);
  operands[0] = x3;
  x4 = XEXP (x2, 1);
  if (GET_CODE (x4) == CONST_INT)
    {
      res = recog_3 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
    }
  if (!memory_operand (operands[0], E_QImode))
    return -1;
  operands[1] = x4;
  if (!nonmemory_operand (operands[1], E_SImode))
    return -1;
  x5 = XEXP (x2, 2);
  operands[2] = x5;
  if (!nonmemory_operand (operands[2], E_SImode))
    return -1;
  x6 = XEXP (x1, 1);
  operands[3] = x6;
  if (!register_operand (operands[3], E_SImode)
      || !
#line 5687 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD))
    return -1;
  return 371; /* *insv_bfins_mem */
}

static int
recog_5 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  x3 = XEXP (x2, 1);
  if (XWINT (x3, 0) ==  HOST_WIDE_INT_C (1))
    {
      x4 = XEXP (x2, 2);
      if (GET_CODE (x4) == MINUS
          && GET_MODE (x4) == E_SImode)
        {
          x5 = XEXP (x4, 0);
          if (x5 == const_int_rtx[MAX_SAVED_CONST_INT + 7]
              && memory_operand (operands[0], E_QImode))
            {
              x6 = XEXP (x4, 1);
              switch (GET_CODE (x6))
                {
                case CONST_INT:
                case CONST_WIDE_INT:
                case CONST_POLY_INT:
                case CONST_FIXED:
                case CONST_DOUBLE:
                case CONST_VECTOR:
                case CONST:
                case REG:
                case SUBREG:
                case MEM:
                case LABEL_REF:
                case SYMBOL_REF:
                case HIGH:
                  operands[1] = x6;
                  if (general_operand (operands[1], E_SImode))
                    return 358; /* bclrmemqi */
                  break;

                case SIGN_EXTEND:
                case ZERO_EXTEND:
                  operands[2] = x6;
                  if (extend_operator (operands[2], E_SImode))
                    {
                      x7 = XEXP (x6, 0);
                      operands[1] = x7;
                      if (general_operand (operands[1], E_VOIDmode))
                        return 359; /* *bclrmemqi_ext */
                    }
                  break;

                default:
                  break;
                }
            }
        }
    }
  operands[1] = x3;
  if (!const_int_operand (operands[1], E_SImode)
      || !register_operand (operands[0], E_SImode))
    return -1;
  x4 = XEXP (x2, 2);
  operands[2] = x4;
  if (!const_int_operand (operands[2], E_SImode)
      || !
#line 5716 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[2]), 0, 31)))
    return -1;
  return 374; /* *insv_bfclr_reg */
}

static int
recog_6 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  switch (XWINT (x2, 0))
    {
    case  HOST_WIDE_INT_C (0):
      x3 = XEXP (x1, 0);
      switch (GET_CODE (x3))
        {
        case REG:
        case SUBREG:
        case MEM:
          operands[0] = x3;
          if (!movsi_const0_operand (operands[0], E_SImode))
            return -1;
          if (
#line 867 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68000_10))
            return 50; /* *movsi_const0_68000_10 */
          if (
#line 880 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68040_60))
            return 51; /* *movsi_const0_68040_60 */
          if (!
#line 898 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!(TUNE_68000_10 || TUNE_68040_60)))
            return -1;
          return 52; /* *movsi_const0 */

        case ZERO_EXTRACT:
          if (GET_MODE (x3) != E_SImode)
            return -1;
          x4 = XEXP (x3, 0);
          operands[0] = x4;
          x5 = XEXP (x3, 1);
          if (GET_CODE (x5) == CONST_INT)
            {
              res = recog_5 (x1, insn, pnum_clobbers);
              if (res >= 0)
                return res;
            }
          if (!memory_operand (operands[0], E_QImode))
            return -1;
          operands[1] = x5;
          if (!nonmemory_operand (operands[1], E_SImode))
            return -1;
          x6 = XEXP (x3, 2);
          operands[2] = x6;
          if (!nonmemory_operand (operands[2], E_SImode)
              || !
#line 5648 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD))
            return -1;
          return 369; /* *insv_bfclr_mem */

        default:
          return -1;
        }

    case  HOST_WIDE_INT_C (-1):
      x3 = XEXP (x1, 0);
      if (GET_CODE (x3) != ZERO_EXTRACT
          || GET_MODE (x3) != E_SImode)
        return -1;
      x4 = XEXP (x3, 0);
      operands[0] = x4;
      x5 = XEXP (x3, 1);
      operands[1] = x5;
      x6 = XEXP (x3, 2);
      operands[2] = x6;
      switch (GET_MODE (operands[0]))
        {
        case E_QImode:
          if (!memory_operand (operands[0], E_QImode)
              || !general_operand (operands[1], E_SImode)
              || !general_operand (operands[2], E_SImode)
              || !
#line 5658 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD))
            return -1;
          return 370; /* *insv_bfset_mem */

        case E_SImode:
          if (!register_operand (operands[0], E_SImode)
              || !const_int_operand (operands[1], E_SImode)
              || !const_int_operand (operands[2], E_SImode)
              || !
#line 5726 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[2]), 0, 31)))
            return -1;
          return 375; /* *insv_bfset_reg */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_7 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  switch (GET_CODE (x2))
    {
    case CONST_INT:
      res = recog_6 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case SUBREG:
      if (known_eq (SUBREG_BYTE (x2), 6)
          && GET_MODE (x2) == E_HImode)
        {
          x3 = XEXP (x2, 0);
          if (GET_CODE (x3) == ASHIFTRT
              && GET_MODE (x3) == E_DImode)
            {
              x4 = XEXP (x3, 1);
              if (x4 == const_int_rtx[MAX_SAVED_CONST_INT + 32])
                {
                  x5 = XEXP (x1, 0);
                  operands[0] = x5;
                  if (nonimmediate_operand (operands[0], E_HImode))
                    {
                      x6 = XEXP (x3, 0);
                      operands[1] = x6;
                      if (general_operand (operands[1], E_DImode))
                        return 317; /* subreghi1ashrdi_const32 */
                    }
                }
            }
        }
      break;

    case LABEL_REF:
      x5 = XEXP (x1, 0);
      if (GET_CODE (x5) == PC)
        {
          x3 = XEXP (x2, 0);
          operands[0] = x3;
          return 381; /* jump */
        }
      break;

    default:
      break;
    }
  operands[1] = x2;
  if (!general_src_operand (operands[1], E_SImode))
    return -1;
  x5 = XEXP (x1, 0);
  operands[0] = x5;
  if (!nonimmediate_operand (operands[0], E_SImode))
    return -1;
  if (
#line 995 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && reload_completed))
    return 53; /* *movsi_m68k */
  if (!
#line 1008 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
    return -1;
  return 54; /* *movsi_m68k2 */
}

static int
recog_8 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  switch (GET_CODE (x4))
    {
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
      operands[1] = x4;
      switch (GET_MODE (operands[0]))
        {
        case E_DImode:
          if (!nonimmediate_operand (operands[0], E_DImode)
              || GET_MODE (x3) != E_DImode)
            return -1;
          if (general_src_operand (operands[1], E_QImode))
            return 90; /* extendqidi2 */
          if (general_src_operand (operands[1], E_HImode))
            return 91; /* extendhidi2 */
          if (pnum_clobbers == NULL
              || !nonimmediate_src_operand (operands[1], E_SImode))
            return -1;
          *pnum_clobbers = 1;
          return 92; /* extendsidi2 */

        case E_HImode:
          if (!nonimmediate_operand (operands[0], E_HImode)
              || GET_MODE (x3) != E_HImode
              || !nonimmediate_operand (operands[1], E_QImode))
            return -1;
          return 96; /* extendqihi2 */

        default:
          return -1;
        }

    case PLUS:
      if (GET_MODE (x4) != E_SImode
          || !register_operand (operands[0], E_DImode)
          || GET_MODE (x3) != E_DImode)
        return -1;
      x5 = XEXP (x4, 0);
      operands[1] = x5;
      if (!general_operand (operands[1], E_SImode))
        return -1;
      x6 = XEXP (x4, 1);
      operands[2] = x6;
      if (!general_operand (operands[2], E_SImode))
        return -1;
      return 93; /* extendplussidi */

    default:
      return -1;
    }
}

static int
recog_9 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  switch (GET_CODE (x3))
    {
    case LSHIFTRT:
      if (GET_MODE (x3) != E_DImode)
        return -1;
      x4 = XEXP (x3, 1);
      if (GET_CODE (x4) != CONST_INT)
        return -1;
      x5 = XEXP (x1, 0);
      operands[0] = x5;
      if (GET_MODE (x2) != E_DImode)
        return -1;
      x6 = XEXP (x3, 0);
      operands[1] = x6;
      switch (XWINT (x4, 0))
        {
        case  HOST_WIDE_INT_C (63):
          if (pnum_clobbers == NULL
              || !nonimmediate_operand (operands[0], E_DImode)
              || !general_operand (operands[1], E_DImode))
            return -1;
          x7 = XEXP (x2, 1);
          if (!rtx_equal_p (x7, operands[1]))
            return -1;
          *pnum_clobbers = 1;
          return 142; /* adddi_lshrdi_63 */

        case  HOST_WIDE_INT_C (32):
          x7 = XEXP (x2, 1);
          operands[2] = x7;
          if (general_operand (operands[2], E_DImode)
              && nonimmediate_operand (operands[0], E_DImode)
              && general_operand (operands[1], E_DImode)
              && 
#line 2348 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return 144; /* *adddi_dilshr32 */
          if (!register_operand (operands[2], E_DImode)
              || !register_operand (operands[0], E_DImode)
              || !nonimmediate_operand (operands[1], E_DImode)
              || !
#line 2362 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
            return -1;
          return 145; /* *adddi_dilshr32_cf */

        default:
          return -1;
        }

    case ASHIFT:
      if (GET_MODE (x3) != E_DImode)
        return -1;
      x4 = XEXP (x3, 1);
      if (x4 != const_int_rtx[MAX_SAVED_CONST_INT + 32])
        return -1;
      x5 = XEXP (x1, 0);
      operands[0] = x5;
      if (!nonimmediate_operand (operands[0], E_DImode)
          || GET_MODE (x2) != E_DImode)
        return -1;
      x7 = XEXP (x2, 1);
      operands[2] = x7;
      if (!general_operand (operands[2], E_DImode))
        return -1;
      x6 = XEXP (x3, 0);
      switch (GET_CODE (x6))
        {
        case SIGN_EXTEND:
          if (pnum_clobbers == NULL
              || GET_MODE (x6) != E_DImode)
            return -1;
          x8 = XEXP (x6, 0);
          operands[1] = x8;
          if (!general_operand (operands[1], E_HImode)
              || !
#line 2333 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          *pnum_clobbers = 1;
          return 143; /* adddi_sexthishl32 */

        case CONST_INT:
        case CONST_WIDE_INT:
        case CONST_POLY_INT:
        case CONST_FIXED:
        case CONST_DOUBLE:
        case CONST_VECTOR:
        case CONST:
        case REG:
        case SUBREG:
        case MEM:
        case LABEL_REF:
        case SYMBOL_REF:
        case HIGH:
          operands[1] = x6;
          if (!general_operand (operands[1], E_DImode))
            return -1;
          return 146; /* adddi_dishl32 */

        default:
          return -1;
        }

    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      x5 = XEXP (x1, 0);
      switch (GET_CODE (x5))
        {
        case REG:
        case SUBREG:
        case MEM:
          switch (pattern12 (x1, pnum_clobbers))
            {
            case 0:
              *pnum_clobbers = 1;
              return 147; /* adddi3 */

            case 1:
              if (!
#line 2573 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 152; /* addhi3 */

            case 2:
              if (!
#line 2730 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 155; /* addqi3 */

            case 3:
              if (
#line 2834 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 167; /* addsf3_68881 */
              if (!
#line 2847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 170; /* addsf3_cf */

            case 4:
              if (
#line 2834 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 168; /* adddf3_68881 */
              if (!
#line 2847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 171; /* adddf3_cf */

            case 5:
              if (!
#line 2834 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return -1;
              return 169; /* addxf3_68881 */

            default:
              return -1;
            }

        case STRICT_LOW_PART:
          x9 = XEXP (x5, 0);
          operands[0] = x9;
          switch (pattern0 (x2))
            {
            case 0:
              x7 = XEXP (x2, 1);
              operands[1] = x7;
              if (general_src_operand (operands[1], E_HImode)
                  && rtx_equal_p (x3, operands[0])
                  && 
#line 2632 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 153; /* *m68k.md:2628 */
              operands[1] = x3;
              if (!general_src_operand (operands[1], E_HImode)
                  || !rtx_equal_p (x7, operands[0])
                  || !
#line 2681 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 154; /* *m68k.md:2677 */

            case 1:
              x7 = XEXP (x2, 1);
              operands[1] = x7;
              if (general_src_operand (operands[1], E_QImode)
                  && rtx_equal_p (x3, operands[0])
                  && 
#line 2754 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 156; /* *m68k.md:2750 */
              operands[1] = x3;
              if (!general_src_operand (operands[1], E_QImode)
                  || !rtx_equal_p (x7, operands[0])
                  || !
#line 2777 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 157; /* *m68k.md:2773 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case FLOAT:
      x5 = XEXP (x1, 0);
      operands[0] = x5;
      switch (pattern14 (x2))
        {
        case 0:
          if (general_operand (operands[2], E_SImode)
              && 
#line 2807 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 158; /* addsf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 2816 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 161; /* addsf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 2825 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 164; /* addsf3_floatqi_68881 */

        case 1:
          if (general_operand (operands[2], E_SImode)
              && 
#line 2807 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 159; /* adddf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 2816 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 162; /* adddf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 2825 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 165; /* adddf3_floatqi_68881 */

        case 2:
          if (general_operand (operands[2], E_SImode)
              && 
#line 2807 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 160; /* addxf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 2816 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 163; /* addxf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 2825 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 166; /* addxf3_floatqi_68881 */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_10 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 1);
  switch (GET_CODE (x3))
    {
    case ASHIFT:
      if (GET_MODE (x3) != E_DImode)
        return -1;
      x4 = XEXP (x3, 1);
      if (x4 != const_int_rtx[MAX_SAVED_CONST_INT + 32])
        return -1;
      x5 = XEXP (x1, 0);
      operands[0] = x5;
      if (!nonimmediate_operand (operands[0], E_DImode)
          || GET_MODE (x2) != E_DImode)
        return -1;
      x6 = XEXP (x3, 0);
      switch (GET_CODE (x6))
        {
        case SIGN_EXTEND:
          if (pnum_clobbers == NULL
              || GET_MODE (x6) != E_DImode)
            return -1;
          x7 = XEXP (x2, 0);
          operands[1] = x7;
          if (!general_operand (operands[1], E_DImode))
            return -1;
          x8 = XEXP (x6, 0);
          operands[2] = x8;
          if (!general_operand (operands[2], E_HImode)
              || !
#line 2864 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          *pnum_clobbers = 1;
          return 172; /* subdi_sexthishl32 */

        case CONST_INT:
        case CONST_WIDE_INT:
        case CONST_POLY_INT:
        case CONST_FIXED:
        case CONST_DOUBLE:
        case CONST_VECTOR:
        case CONST:
        case REG:
        case SUBREG:
        case MEM:
        case LABEL_REF:
        case SYMBOL_REF:
        case HIGH:
          operands[1] = x6;
          if (!general_operand (operands[1], E_DImode))
            return -1;
          x7 = XEXP (x2, 0);
          if (!rtx_equal_p (x7, operands[0]))
            return -1;
          return 173; /* subdi_dishl32 */

        default:
          return -1;
        }

    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      x5 = XEXP (x1, 0);
      switch (GET_CODE (x5))
        {
        case REG:
        case SUBREG:
        case MEM:
          switch (pattern12 (x1, pnum_clobbers))
            {
            case 0:
              *pnum_clobbers = 1;
              return 174; /* subdi3 */

            case 1:
              if (!
#line 2999 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 177; /* subhi3 */

            case 2:
              if (!
#line 3015 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 179; /* subqi3 */

            case 3:
              if (
#line 3065 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 190; /* subsf3_68881 */
              if (!
#line 3078 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 193; /* subsf3_cf */

            case 4:
              if (
#line 3065 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 191; /* subdf3_68881 */
              if (!
#line 3078 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 194; /* subdf3_cf */

            case 5:
              if (!
#line 3065 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return -1;
              return 192; /* subxf3_68881 */

            default:
              return -1;
            }

        case STRICT_LOW_PART:
          x9 = XEXP (x5, 0);
          operands[0] = x9;
          operands[1] = x3;
          x7 = XEXP (x2, 0);
          if (!rtx_equal_p (x7, operands[0]))
            return -1;
          switch (GET_MODE (operands[0]))
            {
            case E_HImode:
              if (!nonimmediate_operand (operands[0], E_HImode)
                  || GET_MODE (x2) != E_HImode
                  || !general_src_operand (operands[1], E_HImode)
                  || !
#line 3007 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 178; /* *m68k.md:3003 */

            case E_QImode:
              if (!nonimmediate_operand (operands[0], E_QImode)
                  || GET_MODE (x2) != E_QImode
                  || !general_src_operand (operands[1], E_QImode)
                  || !
#line 3023 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 180; /* *m68k.md:3019 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case FLOAT:
      x5 = XEXP (x1, 0);
      operands[0] = x5;
      x7 = XEXP (x2, 0);
      operands[1] = x7;
      switch (pattern16 (x2))
        {
        case 0:
          if (general_operand (operands[2], E_SImode)
              && 
#line 3038 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 181; /* subsf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 3047 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 184; /* subsf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 3056 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 187; /* subsf3_floatqi_68881 */

        case 1:
          if (general_operand (operands[2], E_SImode)
              && 
#line 3038 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 182; /* subdf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 3047 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 185; /* subdf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 3056 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 188; /* subdf3_floatqi_68881 */

        case 2:
          if (general_operand (operands[2], E_SImode)
              && 
#line 3038 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 183; /* subxf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 3047 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 186; /* subxf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 3056 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 189; /* subxf3_floatqi_68881 */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_11 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  switch (GET_CODE (x4))
    {
    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      operands[1] = x4;
      x5 = XEXP (x3, 1);
      operands[2] = x5;
      switch (GET_MODE (operands[0]))
        {
        case E_HImode:
          if (pattern11 (x3, E_HImode) != 0)
            return -1;
          return 195; /* mulhi3 */

        case E_DFmode:
          if (pattern10 (x3, E_DFmode) != 0)
            return -1;
          if (
#line 3392 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 219; /* muldf_68881 */
          if (!
#line 3434 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 223; /* fmuldf3_cf */

        case E_SFmode:
          if (pattern10 (x3, E_SFmode) != 0)
            return -1;
          if (
#line 3410 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 220; /* mulsf_68881 */
          if (!
#line 3434 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 222; /* fmulsf3_cf */

        case E_XFmode:
          if (!nonimmediate_operand (operands[0], E_XFmode)
              || GET_MODE (x3) != E_XFmode
              || !nonimmediate_operand (operands[1], E_XFmode)
              || !nonimmediate_operand (operands[2], E_XFmode)
              || !
#line 3425 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 221; /* mulxf3_68881 */

        default:
          return -1;
        }

    case FLOAT:
      switch (pattern14 (x3))
        {
        case 0:
          if (general_operand (operands[2], E_SImode)
              && 
#line 3353 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 210; /* mulsf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 3366 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 213; /* mulsf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 3379 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 216; /* mulsf3_floatqi_68881 */

        case 1:
          if (general_operand (operands[2], E_SImode)
              && 
#line 3353 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 211; /* muldf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 3366 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 214; /* muldf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 3379 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 217; /* muldf3_floatqi_68881 */

        case 2:
          if (general_operand (operands[2], E_SImode)
              && 
#line 3353 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 212; /* mulxf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 3366 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 215; /* mulxf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 3379 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 218; /* mulxf3_floatqi_68881 */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_12 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  switch (GET_CODE (x2))
    {
    case REG:
    case SUBREG:
    case MEM:
      operands[0] = x2;
      x3 = XEXP (x1, 1);
      x4 = XEXP (x3, 0);
      operands[1] = x4;
      x5 = XEXP (x3, 1);
      operands[2] = x5;
      switch (GET_MODE (operands[0]))
        {
        case E_HImode:
          if (pattern11 (x3, E_HImode) != 0
              || !
#line 3697 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 247; /* andhi3 */

        case E_QImode:
          if (pattern11 (x3, E_QImode) != 0
              || !
#line 3721 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 250; /* andqi3 */

        default:
          return -1;
        }

    case STRICT_LOW_PART:
      x6 = XEXP (x2, 0);
      operands[0] = x6;
      x3 = XEXP (x1, 1);
      switch (pattern0 (x3))
        {
        case 0:
          x5 = XEXP (x3, 1);
          operands[1] = x5;
          if (general_src_operand (operands[1], E_HImode))
            {
              x4 = XEXP (x3, 0);
              if (rtx_equal_p (x4, operands[0])
                  && 
#line 3705 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 248; /* *m68k.md:3701 */
            }
          x4 = XEXP (x3, 0);
          operands[1] = x4;
          if (!general_src_operand (operands[1], E_HImode)
              || !rtx_equal_p (x5, operands[0])
              || !
#line 3713 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 249; /* *m68k.md:3709 */

        case 1:
          x5 = XEXP (x3, 1);
          operands[1] = x5;
          if (general_src_operand (operands[1], E_QImode))
            {
              x4 = XEXP (x3, 0);
              if (rtx_equal_p (x4, operands[0])
                  && 
#line 3729 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 251; /* *m68k.md:3725 */
            }
          x4 = XEXP (x3, 0);
          operands[1] = x4;
          if (!general_src_operand (operands[1], E_QImode)
              || !rtx_equal_p (x5, operands[0])
              || !
#line 3737 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 252; /* *m68k.md:3733 */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_13 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  operands[1] = x3;
  if (register_operand (operands[1], E_VOIDmode))
    {
      if (post_inc_operand (operands[0], E_VOIDmode)
          && 
#line 1692 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(GET_MODE_CLASS (GET_MODE (operands[0])) == MODE_INT &&
   GET_MODE_CLASS (GET_MODE (operands[1])) == MODE_INT &&
   GET_MODE_SIZE (GET_MODE (operands[0])) == GET_MODE_SIZE (GET_MODE (operands[1])) * 2))
        return 80; /* *zero_extend_inc */
      if (pre_dec_operand (operands[0], E_VOIDmode)
          && 
#line 1708 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((GET_MODE (operands[0]) != HImode || XEXP (XEXP (operands[0], 0), 0) != stack_pointer_rtx) &&
   GET_MODE_CLASS (GET_MODE (operands[0])) == MODE_INT &&
   GET_MODE_CLASS (GET_MODE (operands[1])) == MODE_INT &&
   GET_MODE_SIZE (GET_MODE (operands[0])) == GET_MODE_SIZE (GET_MODE (operands[1])) * 2))
        return 81; /* *zero_extend_dec */
    }
  if (nonimmediate_src_operand (operands[1], E_SImode)
      && nonimmediate_operand (operands[0], E_DImode)
      && GET_MODE (x2) == E_DImode
      && 
#line 1765 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(GET_CODE (operands[0]) != MEM || GET_CODE (operands[1]) != MEM))
    return 84; /* *zero_extendsidi2 */
  if (register_operand (operands[0], E_SImode)
      && GET_MODE (x2) == E_SImode)
    {
      switch (GET_MODE (operands[1]))
        {
        case E_HImode:
          if (nonimmediate_src_operand (operands[1], E_HImode))
            {
              if (
#line 1780 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_MVS_MVZ))
                return 85; /* *zero_extendhisi2_cf */
              return 86; /* zero_extendhisi2 */
            }
          break;

        case E_QImode:
          if (nonimmediate_src_operand (operands[1], E_QImode))
            {
              if (
#line 1805 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_MVS_MVZ))
                return 88; /* *zero_extendqisi2_cfv4 */
              return 89; /* zero_extendqisi2 */
            }
          break;

        default:
          break;
        }
    }
  if (!nonimmediate_src_operand (operands[1], E_QImode)
      || !register_operand (operands[0], E_HImode)
      || GET_MODE (x2) != E_HImode
      || !
#line 1799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
    return -1;
  return 87; /* *zero_extendqihi2 */
}

static int
recog_14 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) != E_SImode
      || !nonimmediate_operand (operands[0], E_SImode))
    return -1;
  x3 = XEXP (x2, 0);
  operands[1] = x3;
  switch (GET_MODE (operands[1]))
    {
    case E_HImode:
      if (!nonimmediate_src_operand (operands[1], E_HImode))
        return -1;
      if (
#line 1958 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_MVS_MVZ))
        return 94; /* *cfv4_extendhisi2 */
      if (!
#line 1966 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!ISA_HAS_MVS_MVZ))
        return -1;
      return 95; /* *68k_extendhisi2 */

    case E_QImode:
      if (!nonimmediate_operand (operands[1], E_QImode))
        return -1;
      if (
#line 1988 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_MVS_MVZ))
        return 97; /* *cfv4_extendqisi2 */
      if (!
#line 1995 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || (TARGET_COLDFIRE && !ISA_HAS_MVS_MVZ)))
        return -1;
      return 98; /* *68k_extendqisi2 */

    default:
      return -1;
    }
}

static int
recog_15 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) != E_SImode
      || !nonimmediate_operand (operands[0], E_SImode))
    return -1;
  x3 = XEXP (x2, 0);
  switch (GET_CODE (x3))
    {
    case LSHIFTRT:
      if (GET_MODE (x3) != E_SImode)
        return -1;
      x4 = XEXP (x3, 1);
      if (x4 != const_int_rtx[MAX_SAVED_CONST_INT + 31])
        return -1;
      x5 = XEXP (x3, 0);
      operands[1] = x5;
      if (!general_operand (operands[1], E_SImode))
        return -1;
      x6 = XEXP (x2, 1);
      if (!rtx_equal_p (x6, operands[1]))
        return -1;
      return 148; /* addsi_lshrsi_31 */

    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      switch (pattern19 (x2))
        {
        case 0:
          if (
#line 2508 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_COLDFIRE))
            return 149; /* *addsi3_internal */
          if (!
#line 2516 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
            return -1;
          return 150; /* *addsi3_5200 */

        case 1:
          if (!
#line 2566 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 151; /* *m68k.md:2561 */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_16 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) != E_SImode
      || !nonimmediate_operand (operands[0], E_SImode))
    return -1;
  x3 = XEXP (x2, 0);
  switch (GET_CODE (x3))
    {
    case SIGN_EXTEND:
      if (GET_MODE (x3) != E_SImode)
        return -1;
      x4 = XEXP (x3, 0);
      operands[1] = x4;
      if (!nonimmediate_operand (operands[1], E_HImode))
        return -1;
      x5 = XEXP (x2, 1);
      switch (GET_CODE (x5))
        {
        case SIGN_EXTEND:
          if (GET_MODE (x5) != E_SImode)
            return -1;
          x6 = XEXP (x5, 0);
          operands[2] = x6;
          if (!nonimmediate_src_operand (operands[2], E_HImode))
            return -1;
          return 196; /* mulhisi3 */

        case CONST_INT:
          operands[2] = x5;
          if (!const_int_operand (operands[2], E_SImode)
              || !
#line 3118 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[2]) >= -0x8000 && INTVAL (operands[2]) <= 0x7fff))
            return -1;
          return 197; /* *mulhisisi3_s */

        default:
          return -1;
        }

    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      operands[1] = x3;
      if (!general_operand (operands[1], E_SImode))
        return -1;
      x5 = XEXP (x2, 1);
      operands[2] = x5;
      if (general_src_operand (operands[2], E_SImode)
          && 
#line 3137 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020))
        return 198; /* *mulsi3_68020 */
      if (!general_operand (operands[2], E_SImode)
          || !
#line 3146 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
        return -1;
      return 199; /* *mulsi3_cf */

    case ZERO_EXTEND:
      if (GET_MODE (x3) != E_SImode)
        return -1;
      x4 = XEXP (x3, 0);
      operands[1] = x4;
      if (!nonimmediate_operand (operands[1], E_HImode))
        return -1;
      x5 = XEXP (x2, 1);
      switch (GET_CODE (x5))
        {
        case ZERO_EXTEND:
          if (GET_MODE (x5) != E_SImode)
            return -1;
          x6 = XEXP (x5, 0);
          operands[2] = x6;
          if (!nonimmediate_src_operand (operands[2], E_HImode))
            return -1;
          return 200; /* umulhisi3 */

        case CONST_INT:
          operands[2] = x5;
          if (!const_int_operand (operands[2], E_SImode)
              || !
#line 3169 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[2]) >= 0 && INTVAL (operands[2]) <= 0xffff))
            return -1;
          return 201; /* *mulhisisi3_z */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_17 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  if (GET_MODE (x2) != E_SImode)
    return -1;
  x3 = XEXP (x2, 1);
  operands[2] = x3;
  x4 = XEXP (x2, 0);
  switch (GET_CODE (x4))
    {
    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      operands[1] = x4;
      if (register_operand (operands[1], E_SImode)
          && register_operand (operands[0], E_SImode)
          && const_int_operand (operands[2], E_SImode)
          && 
#line 3660 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
        return 244; /* *andsi3_split */
      if (!general_operand (operands[1], E_SImode)
          || !not_sp_operand (operands[0], E_SImode)
          || !general_src_operand (operands[2], E_SImode))
        return -1;
      if (
#line 3669 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return 245; /* andsi3_internal */
      if (!
#line 3679 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
        return -1;
      return 246; /* andsi3_5200 */

    case ROTATE:
      if (GET_MODE (x4) != E_SImode)
        return -1;
      x5 = XEXP (x4, 0);
      if (x5 != const_int_rtx[MAX_SAVED_CONST_INT + -2])
        return -1;
      x6 = XEXP (x4, 1);
      if (GET_CODE (x6) != AND
          || GET_MODE (x6) != E_SImode)
        return -1;
      x7 = XEXP (x6, 1);
      if (x7 != const_int_rtx[MAX_SAVED_CONST_INT + 31]
          || !register_operand (operands[0], E_SImode))
        return -1;
      x8 = XEXP (x6, 0);
      operands[1] = x8;
      if (!register_operand (operands[1], E_SImode)
          || !register_operand (operands[2], E_SImode))
        return -1;
      return 357; /* *bclrdreg */

    default:
      return -1;
    }
}

static int
recog_18 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  x4 = XEXP (x3, 1);
  if (x4 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
    return -1;
  x5 = XEXP (x2, 1);
  if (GET_CODE (x5) != LABEL_REF)
    return -1;
  x6 = XEXP (x2, 2);
  if (GET_CODE (x6) != PC)
    return -1;
  x7 = XEXP (x3, 0);
  operands[0] = x7;
  if (!general_operand (operands[0], E_DImode))
    return -1;
  x8 = XEXP (x5, 0);
  operands[1] = x8;
  switch (GET_CODE (x3))
    {
    case EQ:
      *pnum_clobbers = 1;
      return 3; /* beq0_di */

    case NE:
      *pnum_clobbers = 1;
      return 4; /* bne0_di */

    default:
      return -1;
    }
}

static int
recog_19 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11, x12, x13;
  int res ATTRIBUTE_UNUSED;
  if (pnum_clobbers != NULL)
    {
      res = recog_18 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
    }
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  operands[0] = x3;
  if (!equality_comparison_operator (operands[0], E_VOIDmode))
    return -1;
  x4 = XEXP (x3, 0);
  if (GET_CODE (x4) != ZERO_EXTRACT
      || GET_MODE (x4) != E_SImode)
    return -1;
  x5 = XEXP (x4, 1);
  if (x5 != const_int_rtx[MAX_SAVED_CONST_INT + 1])
    return -1;
  x6 = XEXP (x3, 1);
  if (x6 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
    return -1;
  x7 = XEXP (x2, 1);
  if (GET_CODE (x7) != LABEL_REF)
    return -1;
  x8 = XEXP (x2, 2);
  if (GET_CODE (x8) != PC)
    return -1;
  x9 = XEXP (x4, 0);
  operands[1] = x9;
  x10 = XEXP (x7, 0);
  operands[3] = x10;
  x11 = XEXP (x4, 2);
  switch (GET_CODE (x11))
    {
    case MINUS:
      if (GET_MODE (x11) != E_SImode)
        return -1;
      x12 = XEXP (x11, 0);
      if (GET_CODE (x12) != CONST_INT)
        return -1;
      x13 = XEXP (x11, 1);
      operands[2] = x13;
      if (!general_operand (operands[2], E_SImode))
        return -1;
      switch (XWINT (x12, 0))
        {
        case  HOST_WIDE_INT_C (7):
          if (!memory_src_operand (operands[1], E_QImode))
            return -1;
          return 24; /* cbranchsi4_btst_mem_insn */

        case  HOST_WIDE_INT_C (31):
          if (!register_operand (operands[1], E_SImode)
              || !
#line 621 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!(CONST_INT_P (operands[1]) && !IN_RANGE (INTVAL (operands[1]), 0, 31))))
            return -1;
          return 25; /* cbranchsi4_btst_reg_insn */

        default:
          return -1;
        }

    case CONST_INT:
      operands[2] = x11;
      if (!const_int_operand (operands[2], E_SImode))
        return -1;
      switch (GET_MODE (operands[1]))
        {
        case E_QImode:
          if (!memory_operand (operands[1], E_QImode)
              || !
#line 639 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && (unsigned) INTVAL (operands[2]) < 8))
            return -1;
          return 26; /* cbranchsi4_btst_mem_insn_1 */

        case E_SImode:
          if (!nonimmediate_operand (operands[1], E_SImode)
              || !
#line 656 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!(REG_P (operands[1]) && !IN_RANGE (INTVAL (operands[2]), 0, 31))))
            return -1;
          return 27; /* cbranchsi4_btst_reg_insn_1 */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_20 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11, x12;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  x4 = XEXP (x3, 0);
  switch (GET_CODE (x4))
    {
    case REG:
    case SUBREG:
    case MEM:
      x5 = XEXP (x2, 1);
      switch (GET_CODE (x5))
        {
        case LABEL_REF:
          x6 = XEXP (x2, 2);
          if (GET_CODE (x6) != PC)
            return -1;
          if (pnum_clobbers != NULL)
            {
              operands[1] = x3;
              operands[2] = x4;
              if (nonimmediate_operand (operands[2], E_DImode))
                {
                  x7 = XEXP (x3, 1);
                  operands[3] = x7;
                  if (general_operand (operands[3], E_DImode))
                    {
                      x8 = XEXP (x5, 0);
                      operands[4] = x8;
                      *pnum_clobbers = 2;
                      return 5; /* cbranchdi4_insn */
                    }
                }
            }
          operands[0] = x3;
          operands[1] = x4;
          x7 = XEXP (x3, 1);
          operands[2] = x7;
          x8 = XEXP (x5, 0);
          operands[3] = x8;
          switch (pattern26 ())
            {
            case 0:
              if (general_operand (operands[2], E_QImode)
                  && 
#line 510 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 6; /* cbranchqi4_insn */
              if (!const0_operand (operands[2], E_QImode)
                  || !
#line 540 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
                return -1;
              return 12; /* cbranchqi4_insn_cf */

            case 1:
              if (general_operand (operands[2], E_HImode)
                  && 
#line 510 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 7; /* cbranchhi4_insn */
              if (!const0_operand (operands[2], E_HImode)
                  || !
#line 540 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
                return -1;
              return 13; /* cbranchhi4_insn_cf */

            case 2:
              if (
#line 510 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 8; /* cbranchsi4_insn */
              if (!
#line 540 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
                return -1;
              return 14; /* cbranchsi4_insn_cf */

            default:
              return -1;
            }

        case PC:
          x6 = XEXP (x2, 2);
          if (GET_CODE (x6) != LABEL_REF)
            return -1;
          operands[0] = x3;
          operands[1] = x4;
          x7 = XEXP (x3, 1);
          operands[2] = x7;
          x9 = XEXP (x6, 0);
          operands[3] = x9;
          switch (pattern26 ())
            {
            case 0:
              if (general_operand (operands[2], E_QImode)
                  && 
#line 525 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 9; /* cbranchqi4_insn_rev */
              if (!const0_operand (operands[2], E_QImode)
                  || !
#line 555 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
                return -1;
              return 15; /* cbranchqi4_insn_cf_rev */

            case 1:
              if (general_operand (operands[2], E_HImode)
                  && 
#line 525 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 10; /* cbranchhi4_insn_rev */
              if (!const0_operand (operands[2], E_HImode)
                  || !
#line 555 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
                return -1;
              return 16; /* cbranchhi4_insn_cf_rev */

            case 2:
              if (
#line 525 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 11; /* cbranchsi4_insn_rev */
              if (!
#line 555 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
                return -1;
              return 17; /* cbranchsi4_insn_cf_rev */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case ZERO_EXTRACT:
      if (GET_MODE (x4) != E_SImode)
        return -1;
      x7 = XEXP (x3, 1);
      if (x7 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
        return -1;
      x5 = XEXP (x2, 1);
      if (GET_CODE (x5) != LABEL_REF)
        return -1;
      x6 = XEXP (x2, 2);
      if (GET_CODE (x6) != PC)
        return -1;
      operands[0] = x3;
      x10 = XEXP (x4, 0);
      operands[1] = x10;
      x11 = XEXP (x4, 1);
      operands[2] = x11;
      if (!const_int_operand (operands[2], E_SImode))
        return -1;
      x12 = XEXP (x4, 2);
      operands[3] = x12;
      if (!general_operand (operands[3], E_SImode))
        return -1;
      x8 = XEXP (x5, 0);
      operands[4] = x8;
      switch (GET_MODE (operands[1]))
        {
        case E_QImode:
          if (!memory_operand (operands[1], E_QImode)
              || !
#line 690 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (!REG_P (operands[1]) || !CONST_INT_P (operands[3])
       || IN_RANGE (INTVAL (operands[3]), 0, 31))))
            return -1;
          return 28; /* cbranch_bftstqi_insn */

        case E_SImode:
          if (!register_operand (operands[1], E_SImode)
              || !
#line 690 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (!REG_P (operands[1]) || !CONST_INT_P (operands[3])
       || IN_RANGE (INTVAL (operands[3]), 0, 31))))
            return -1;
          return 29; /* cbranch_bftstsi_insn */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_21 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  switch (GET_CODE (x3))
    {
    case REG:
    case SUBREG:
    case MEM:
      operands[2] = x3;
      x4 = XEXP (x2, 1);
      operands[3] = x4;
      switch (GET_MODE (operands[2]))
        {
        case E_QImode:
          if (!nonimmediate_operand (operands[2], E_QImode))
            return -1;
          if (general_operand (operands[3], E_QImode)
              && 
#line 568 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return 18; /* cstoreqi4_insn */
          if (!const0_operand (operands[3], E_QImode)
              || !
#line 581 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
            return -1;
          return 21; /* cstoreqi4_insn_cf */

        case E_HImode:
          if (!nonimmediate_operand (operands[2], E_HImode))
            return -1;
          if (general_operand (operands[3], E_HImode)
              && 
#line 568 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return 19; /* cstorehi4_insn */
          if (!const0_operand (operands[3], E_HImode)
              || !
#line 581 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
            return -1;
          return 22; /* cstorehi4_insn_cf */

        case E_SImode:
          if (!nonimmediate_operand (operands[2], E_SImode)
              || !general_operand (operands[3], E_SImode))
            return -1;
          if (
#line 568 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return 20; /* cstoresi4_insn */
          if (!
#line 581 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
            return -1;
          return 23; /* cstoresi4_insn_cf */

        default:
          return -1;
        }

    case ZERO_EXTRACT:
      if (GET_MODE (x3) != E_SImode)
        return -1;
      x4 = XEXP (x2, 1);
      if (x4 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
        return -1;
      x5 = XEXP (x3, 0);
      operands[2] = x5;
      x6 = XEXP (x3, 1);
      operands[3] = x6;
      if (!const_int_operand (operands[3], E_SImode))
        return -1;
      x7 = XEXP (x3, 2);
      operands[4] = x7;
      if (!general_operand (operands[4], E_SImode))
        return -1;
      switch (GET_MODE (operands[2]))
        {
        case E_QImode:
          if (!memory_operand (operands[2], E_QImode)
              || !
#line 707 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (!REG_P (operands[2]) || !CONST_INT_P (operands[4])
       || IN_RANGE (INTVAL (operands[4]), 0, 31))))
            return -1;
          return 30; /* cstore_bftstqi_insn */

        case E_SImode:
          if (!register_operand (operands[2], E_SImode)
              || !
#line 707 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (!REG_P (operands[2]) || !CONST_INT_P (operands[4])
       || IN_RANGE (INTVAL (operands[4]), 0, 31))))
            return -1;
          return 31; /* cstore_bftstsi_insn */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_22 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  operands[1] = x2;
  if (ordered_comparison_operator (operands[1], E_QImode))
    {
      res = recog_21 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
    }
  if (!m68k_cstore_comparison_operator (operands[1], E_QImode))
    return -1;
  x3 = XEXP (x2, 0);
  operands[2] = x3;
  x4 = XEXP (x2, 1);
  operands[3] = x4;
  switch (GET_MODE (operands[2]))
    {
    case E_SFmode:
      if (!fp_src_operand (operands[2], E_SFmode))
        return -1;
      if (fp_src_operand (operands[3], E_SFmode)
          && 
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)
   && (register_operand (operands[2], SFmode)
       || register_operand (operands[3], SFmode)
       || const0_operand (operands[3], SFmode))))
        return 44; /* cstoresf4_insn_68881 */
      if (!const0_operand (operands[3], E_SFmode)
          || !
#line 832 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU))
        return -1;
      return 47; /* cstoresf4_insn_cf */

    case E_DFmode:
      if (!fp_src_operand (operands[2], E_DFmode))
        return -1;
      if (fp_src_operand (operands[3], E_DFmode)
          && 
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)
   && (register_operand (operands[2], DFmode)
       || register_operand (operands[3], DFmode)
       || const0_operand (operands[3], DFmode))))
        return 45; /* cstoredf4_insn_68881 */
      if (!const0_operand (operands[3], E_DFmode)
          || !
#line 832 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU))
        return -1;
      return 48; /* cstoredf4_insn_cf */

    case E_XFmode:
      if (!fp_src_operand (operands[2], E_XFmode)
          || !fp_src_operand (operands[3], E_XFmode)
          || !(
#line 815 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)
   && (register_operand (operands[2], XFmode)
       || register_operand (operands[3], XFmode)
       || const0_operand (operands[3], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)))
        return -1;
      return 46; /* cstorexf4_insn_68881 */

    default:
      return -1;
    }
}

static int
recog_23 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  operands[1] = x4;
  x5 = XEXP (x3, 1);
  operands[2] = x5;
  switch (GET_MODE (operands[0]))
    {
    case E_SImode:
      if (pattern11 (x3, E_SImode) != 0)
        return -1;
      if (
#line 3777 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_COLDFIRE))
        return 254; /* iorsi3_internal */
      if (!
#line 3787 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
        return -1;
      return 255; /* iorsi3_5200 */

    case E_HImode:
      if (pattern11 (x3, E_HImode) != 0
          || !
#line 3797 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return -1;
      return 256; /* iorhi3 */

    case E_QImode:
      if (pattern11 (x3, E_QImode) != 0
          || !
#line 3819 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return -1;
      return 259; /* iorqi3 */

    default:
      return -1;
    }
}

static int
recog_24 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  operands[1] = x4;
  x5 = XEXP (x3, 1);
  switch (XWINT (x5, 0))
    {
    case  HOST_WIDE_INT_C (16):
      if (register_operand (operands[0], E_SImode)
          && GET_MODE (x3) == E_SImode
          && register_operand (operands[1], E_SImode)
          && 
#line 4698 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TUNE_68060))
        return 315; /* ashrsi_16 */
      break;

    case  HOST_WIDE_INT_C (1):
      if (register_operand (operands[0], E_DImode)
          && GET_MODE (x3) == E_DImode
          && register_operand (operands[1], E_DImode)
          && 
#line 4741 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return 319; /* *ashrdi3_const1 */
      break;

    case  HOST_WIDE_INT_C (32):
      if (GET_MODE (x3) == E_DImode
          && nonimmediate_src_operand (operands[1], E_DImode))
        {
          if (register_operand (operands[0], E_DImode))
            return 320; /* *ashrdi_const32 */
          if (pnum_clobbers != NULL
              && memory_operand (operands[0], E_DImode))
            {
              *pnum_clobbers = 1;
              return 321; /* *ashrdi_const32_mem */
            }
        }
      break;

    default:
      break;
    }
  operands[2] = x5;
  switch (GET_MODE (operands[0]))
    {
    case E_SImode:
      if (pattern22 (x3) == 0
          && 
#line 4707 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68000_10
   && INTVAL (operands[2]) > 16
   && INTVAL (operands[2]) <= 24))
        return 316; /* *m68k.md:4703 */
      break;

    case E_DImode:
      if (pattern23 (x3) == 0
          && 
#line 4847 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE
    && ((INTVAL (operands[2]) >= 1 && INTVAL (operands[2]) <= 3)
	|| INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16
	|| INTVAL (operands[2]) == 31
	|| (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63))))
        return 322; /* ashrdi_const */
      break;

    default:
      break;
    }
  if (!register_operand (operands[1], E_SImode)
      || XWINT (x5, 0) !=  HOST_WIDE_INT_C (31)
      || !register_operand (operands[0], E_SImode)
      || GET_MODE (x3) != E_SImode)
    return -1;
  return 323; /* ashrsi_31 */
}

static int
recog_25 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) != CONST_INT)
    return -1;
  x4 = XEXP (x2, 0);
  operands[1] = x4;
  if (memory_src_operand (operands[1], E_QImode)
      && XWINT (x3, 0) ==  HOST_WIDE_INT_C (32))
    {
      x5 = XEXP (x2, 2);
      operands[2] = x5;
      if (const_int_operand (operands[2], E_SImode)
          && 
#line 5520 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[2]) % 8) == 0
   && ! mode_dependent_address_p (XEXP (operands[1], 0),
                                  MEM_ADDR_SPACE (operands[1]))))
        return 362; /* *extzv_32_mem */
    }
  if (!register_operand (operands[1], E_SImode))
    return -1;
  operands[2] = x3;
  if (!const_int_operand (operands[2], E_SImode))
    return -1;
  x5 = XEXP (x2, 2);
  operands[3] = x5;
  if (!const_int_operand (operands[3], E_SImode))
    return -1;
  if (
#line 5536 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && IN_RANGE (INTVAL (operands[3]), 0, 31)
   && (INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16)
   && INTVAL (operands[3]) % INTVAL (operands[2]) == 0))
    return 363; /* *extzv_8_16_reg */
  if (!
#line 5706 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[3]), 0, 31)))
    return -1;
  return 373; /* *extv_bfextu_reg */
}

static int
recog_26 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) != CONST_INT)
    return -1;
  x4 = XEXP (x2, 0);
  operands[1] = x4;
  if (memory_src_operand (operands[1], E_QImode)
      && XWINT (x3, 0) ==  HOST_WIDE_INT_C (32))
    {
      x5 = XEXP (x2, 2);
      operands[2] = x5;
      if (const_int_operand (operands[2], E_SImode)
          && 
#line 5562 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[2]) % 8) == 0
   && ! mode_dependent_address_p (XEXP (operands[1], 0),
                                  MEM_ADDR_SPACE (operands[1]))))
        return 364; /* *extv_32_mem */
    }
  if (!register_operand (operands[1], E_SImode))
    return -1;
  operands[2] = x3;
  if (!const_int_operand (operands[2], E_SImode))
    return -1;
  x5 = XEXP (x2, 2);
  operands[3] = x5;
  if (!const_int_operand (operands[3], E_SImode))
    return -1;
  if (
#line 5578 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && IN_RANGE (INTVAL (operands[3]), 0, 31)
   && (INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16)
   && INTVAL (operands[3]) % INTVAL (operands[2]) == 0))
    return 365; /* *extv_8_16_reg */
  if (!
#line 5698 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD && IN_RANGE (INTVAL (operands[3]), 0, 31)))
    return -1;
  return 372; /* *extv_bfexts_reg */
}

static int
recog_27 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  switch (GET_CODE (x4))
    {
    case FIX:
      if (pnum_clobbers == NULL
          || GET_MODE (x4) != E_DFmode)
        return -1;
      x5 = XEXP (x4, 0);
      operands[1] = x5;
      if (!register_operand (operands[1], E_DFmode))
        return -1;
      switch (GET_MODE (operands[0]))
        {
        case E_SImode:
          if (!nonimmediate_operand (operands[0], E_SImode)
              || GET_MODE (x3) != E_SImode
              || !
#line 2172 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TUNE_68040))
            return -1;
          *pnum_clobbers = 2;
          return 119; /* fix_truncdfsi2 */

        case E_HImode:
          if (!nonimmediate_operand (operands[0], E_HImode)
              || GET_MODE (x3) != E_HImode
              || !
#line 2182 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TUNE_68040))
            return -1;
          *pnum_clobbers = 2;
          return 120; /* fix_truncdfhi2 */

        case E_QImode:
          if (!nonimmediate_operand (operands[0], E_QImode)
              || GET_MODE (x3) != E_QImode
              || !
#line 2192 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TUNE_68040))
            return -1;
          *pnum_clobbers = 2;
          return 121; /* fix_truncdfqi2 */

        default:
          return -1;
        }

    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      operands[1] = x4;
      switch (GET_MODE (operands[0]))
        {
        case E_SFmode:
          if (!nonimmediate_operand (operands[0], E_SFmode)
              || GET_MODE (x3) != E_SFmode
              || !general_operand (operands[1], E_SFmode))
            return -1;
          if (
#line 2209 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && !TUNE_68040))
            return 122; /* ftruncsf2_68881 */
          if (!
#line 2220 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 125; /* ftruncsf2_cf */

        case E_DFmode:
          if (!nonimmediate_operand (operands[0], E_DFmode)
              || GET_MODE (x3) != E_DFmode
              || !general_operand (operands[1], E_DFmode))
            return -1;
          if (
#line 2209 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && !TUNE_68040))
            return 123; /* ftruncdf2_68881 */
          if (!
#line 2220 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 126; /* ftruncdf2_cf */

        case E_XFmode:
          if (!nonimmediate_operand (operands[0], E_XFmode)
              || GET_MODE (x3) != E_XFmode
              || !general_operand (operands[1], E_XFmode)
              || !(
#line 2209 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && !TUNE_68040) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)))
            return -1;
          return 124; /* ftruncxf2_68881 */

        case E_QImode:
          switch (pattern20 (x3, E_QImode))
            {
            case 0:
              if (
#line 2239 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 127; /* fixsfqi2_68881 */
              if (!
#line 2246 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 130; /* fixsfqi2_cf */

            case 1:
              if (
#line 2239 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 128; /* fixdfqi2_68881 */
              if (!
#line 2246 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 131; /* fixdfqi2_cf */

            case 2:
              if (!
#line 2239 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return -1;
              return 129; /* fixxfqi2_68881 */

            default:
              return -1;
            }

        case E_HImode:
          switch (pattern20 (x3, E_HImode))
            {
            case 0:
              if (
#line 2259 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 132; /* fixsfhi2_68881 */
              if (!
#line 2266 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 135; /* fixsfhi2_cf */

            case 1:
              if (
#line 2259 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 133; /* fixdfhi2_68881 */
              if (!
#line 2266 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 136; /* fixdfhi2_cf */

            case 2:
              if (!
#line 2259 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return -1;
              return 134; /* fixxfhi2_68881 */

            default:
              return -1;
            }

        case E_SImode:
          switch (pattern20 (x3, E_SImode))
            {
            case 0:
              if (
#line 2279 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 137; /* fixsfsi2_68881 */
              if (!
#line 2286 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 140; /* fixsfsi2_cf */

            case 1:
              if (
#line 2279 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return 138; /* fixdfsi2_68881 */
              if (!
#line 2286 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return -1;
              return 141; /* fixdfsi2_cf */

            case 2:
              if (!
#line 2279 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return -1;
              return 139; /* fixxfsi2_68881 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_28 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11, x12;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  switch (GET_CODE (x3))
    {
    case ZERO_EXTEND:
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      x5 = XEXP (x3, 0);
      operands[1] = x5;
      x6 = XEXP (x2, 1);
      switch (GET_CODE (x6))
        {
        case CONST_INT:
        case CONST_WIDE_INT:
        case CONST_POLY_INT:
        case CONST_FIXED:
        case CONST_DOUBLE:
        case CONST_VECTOR:
        case CONST:
        case REG:
        case SUBREG:
        case MEM:
        case LABEL_REF:
        case SYMBOL_REF:
        case HIGH:
          operands[2] = x6;
          if (!general_operand (operands[1], E_VOIDmode))
            return -1;
          switch (GET_MODE (operands[0]))
            {
            case E_DImode:
              if (pattern24 (x2, E_DImode) != 0
                  || !
#line 3746 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 253; /* iordi_zext */

            case E_SImode:
              if (pattern24 (x2, E_SImode) != 0
                  || !
#line 3861 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 263; /* iorsi_zext */

            default:
              return -1;
            }

        case ASHIFT:
          if (GET_MODE (x6) != E_SImode)
            return -1;
          x7 = XEXP (x6, 1);
          if (x7 != const_int_rtx[MAX_SAVED_CONST_INT + 16]
              || !nonimmediate_operand (operands[0], E_SImode)
              || GET_MODE (x2) != E_SImode
              || GET_MODE (x3) != E_SImode
              || !general_operand (operands[1], E_HImode))
            return -1;
          x8 = XEXP (x6, 0);
          operands[2] = x8;
          if (!general_operand (operands[2], E_SImode))
            return -1;
          return 262; /* iorsi_zexthi_ashl16 */

        default:
          return -1;
        }

    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      x4 = XEXP (x1, 0);
      switch (GET_CODE (x4))
        {
        case REG:
        case SUBREG:
        case MEM:
          res = recog_23 (x1, insn, pnum_clobbers);
          if (res >= 0)
            return res;
          break;

        case STRICT_LOW_PART:
          x9 = XEXP (x4, 0);
          operands[0] = x9;
          switch (pattern0 (x2))
            {
            case 0:
              x6 = XEXP (x2, 1);
              operands[1] = x6;
              if (general_src_operand (operands[1], E_HImode)
                  && rtx_equal_p (x3, operands[0])
                  && 
#line 3804 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 257; /* *m68k.md:3800 */
              operands[1] = x3;
              if (general_src_operand (operands[1], E_HImode)
                  && rtx_equal_p (x6, operands[0])
                  && 
#line 3811 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 258; /* *m68k.md:3807 */
              break;

            case 1:
              x6 = XEXP (x2, 1);
              operands[1] = x6;
              if (general_src_operand (operands[1], E_QImode)
                  && rtx_equal_p (x3, operands[0])
                  && 
#line 3827 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 260; /* *m68k.md:3823 */
              operands[1] = x3;
              if (general_src_operand (operands[1], E_QImode)
                  && rtx_equal_p (x6, operands[0])
                  && 
#line 3835 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 261; /* *m68k.md:3831 */
              break;

            default:
              break;
            }
          break;

        default:
          break;
        }
      if (GET_CODE (x3) != SUBREG
          || maybe_ne (SUBREG_BYTE (x3), 3)
          || GET_MODE (x3) != E_QImode)
        return -1;
      x5 = XEXP (x3, 0);
      if (GET_CODE (x5) != ASHIFT
          || GET_MODE (x5) != E_SImode)
        return -1;
      x10 = XEXP (x5, 0);
      if (x10 != const_int_rtx[MAX_SAVED_CONST_INT + 1])
        return -1;
      operands[0] = x4;
      if (!memory_operand (operands[0], E_QImode)
          || GET_MODE (x2) != E_QImode)
        return -1;
      x6 = XEXP (x2, 1);
      if (!rtx_equal_p (x6, operands[0]))
        return -1;
      x11 = XEXP (x5, 1);
      switch (GET_CODE (x11))
        {
        case CONST_INT:
        case CONST_WIDE_INT:
        case CONST_POLY_INT:
        case CONST_FIXED:
        case CONST_DOUBLE:
        case CONST_VECTOR:
        case CONST:
        case REG:
        case SUBREG:
        case MEM:
        case LABEL_REF:
        case SYMBOL_REF:
        case HIGH:
          operands[1] = x11;
          if (!general_operand (operands[1], E_SImode))
            return -1;
          return 353; /* bsetmemqi */

        case SIGN_EXTEND:
        case ZERO_EXTEND:
          operands[2] = x11;
          if (!extend_operator (operands[2], E_SImode))
            return -1;
          x12 = XEXP (x11, 0);
          operands[1] = x12;
          if (!general_operand (operands[1], E_VOIDmode))
            return -1;
          return 354; /* *bsetmemqi_ext */

        default:
          return -1;
        }

    case ASHIFT:
      if (pattern3 (x1) != 0)
        return -1;
      return 355; /* *bsetdreg */

    default:
      return -1;
    }
}

static int
recog_29 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2;
  int res ATTRIBUTE_UNUSED;
  switch (pattern1 (x1))
    {
    case 0:
      switch (GET_MODE (operands[0]))
        {
        case E_DImode:
          if (!nonimmediate_operand (operands[0], E_DImode))
            return -1;
          x2 = XEXP (x1, 1);
          if (GET_MODE (x2) != E_DImode
              || !general_operand (operands[1], E_DImode))
            return -1;
          if (
#line 3970 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return 272; /* negdi2_internal */
          if (!
#line 3987 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
            return -1;
          return 273; /* negdi2_5200 */

        case E_SImode:
          if (!nonimmediate_operand (operands[0], E_SImode))
            return -1;
          x2 = XEXP (x1, 1);
          if (GET_MODE (x2) != E_SImode
              || !general_operand (operands[1], E_SImode))
            return -1;
          if (
#line 4008 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return 274; /* negsi2_internal */
          if (!
#line 4016 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
            return -1;
          return 275; /* negsi2_5200 */

        case E_HImode:
          if (!nonimmediate_operand (operands[0], E_HImode))
            return -1;
          x2 = XEXP (x1, 1);
          if (GET_MODE (x2) != E_HImode
              || !general_operand (operands[1], E_HImode)
              || !
#line 4024 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 276; /* neghi2 */

        case E_QImode:
          if (!nonimmediate_operand (operands[0], E_QImode))
            return -1;
          x2 = XEXP (x1, 1);
          if (GET_MODE (x2) != E_QImode
              || !general_operand (operands[1], E_QImode)
              || !
#line 4038 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 278; /* negqi2 */

        case E_SFmode:
          if (!nonimmediate_operand (operands[0], E_SFmode))
            return -1;
          x2 = XEXP (x1, 1);
          if (GET_MODE (x2) != E_SFmode
              || !general_operand (operands[1], E_SFmode))
            return -1;
          if (
#line 4145 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 280; /* negsf2_68881 */
          if (!
#line 4160 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 283; /* negsf2_cf */

        case E_DFmode:
          if (!nonimmediate_operand (operands[0], E_DFmode))
            return -1;
          x2 = XEXP (x1, 1);
          if (GET_MODE (x2) != E_DFmode
              || !general_operand (operands[1], E_DFmode))
            return -1;
          if (
#line 4145 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 281; /* negdf2_68881 */
          if (!
#line 4160 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 284; /* negdf2_cf */

        case E_XFmode:
          if (!nonimmediate_operand (operands[0], E_XFmode))
            return -1;
          x2 = XEXP (x1, 1);
          if (GET_MODE (x2) != E_XFmode
              || !general_operand (operands[1], E_XFmode)
              || !
#line 4145 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 282; /* negxf2_68881 */

        default:
          return -1;
        }

    case 1:
      if (!
#line 4031 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return -1;
      return 277; /* *m68k.md:4028 */

    case 2:
      if (!
#line 4045 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
        return -1;
      return 279; /* *m68k.md:4042 */

    default:
      return -1;
    }
}

static int
recog_30 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  switch (GET_CODE (x3))
    {
    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      x4 = XEXP (x1, 0);
      switch (GET_CODE (x4))
        {
        case REG:
        case SUBREG:
        case MEM:
          operands[0] = x4;
          operands[1] = x3;
          x5 = XEXP (x2, 1);
          operands[2] = x5;
          switch (GET_MODE (operands[0]))
            {
            case E_SImode:
              if (pattern10 (x2, E_SImode) != 0)
                return -1;
              if (
#line 3889 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 264; /* xorsi3_internal */
              if (!
#line 3899 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
                return -1;
              return 265; /* xorsi3_5200 */

            case E_HImode:
              if (pattern10 (x2, E_HImode) != 0
                  || !
#line 3909 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 266; /* xorhi3 */

            case E_QImode:
              if (pattern10 (x2, E_QImode) != 0
                  || !
#line 3933 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 269; /* xorqi3 */

            default:
              return -1;
            }

        case STRICT_LOW_PART:
          x6 = XEXP (x4, 0);
          operands[0] = x6;
          switch (pattern0 (x2))
            {
            case 0:
              x5 = XEXP (x2, 1);
              operands[1] = x5;
              if (general_operand (operands[1], E_HImode)
                  && rtx_equal_p (x3, operands[0])
                  && 
#line 3917 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 267; /* *m68k.md:3913 */
              operands[1] = x3;
              if (!general_operand (operands[1], E_HImode)
                  || !rtx_equal_p (x5, operands[0])
                  || !
#line 3925 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 268; /* *m68k.md:3921 */

            case 1:
              x5 = XEXP (x2, 1);
              operands[1] = x5;
              if (general_operand (operands[1], E_QImode)
                  && rtx_equal_p (x3, operands[0])
                  && 
#line 3941 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 270; /* *m68k.md:3937 */
              operands[1] = x3;
              if (!general_operand (operands[1], E_QImode)
                  || !rtx_equal_p (x5, operands[0])
                  || !
#line 3949 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 271; /* *m68k.md:3945 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case ASHIFT:
      if (pattern3 (x1) != 0)
        return -1;
      return 356; /* *bchgdreg */

    case ZERO_EXTRACT:
      if (GET_MODE (x3) != E_SImode)
        return -1;
      x4 = XEXP (x1, 0);
      if (GET_CODE (x4) != ZERO_EXTRACT
          || GET_MODE (x4) != E_SImode)
        return -1;
      x6 = XEXP (x4, 0);
      operands[0] = x6;
      if (!memory_operand (operands[0], E_QImode))
        return -1;
      x7 = XEXP (x4, 1);
      operands[1] = x7;
      if (!nonmemory_operand (operands[1], E_SImode))
        return -1;
      x8 = XEXP (x4, 2);
      operands[2] = x8;
      if (!nonmemory_operand (operands[2], E_SImode)
          || GET_MODE (x2) != E_SImode)
        return -1;
      x5 = XEXP (x2, 1);
      operands[3] = x5;
      if (!const_int_operand (operands[3], E_VOIDmode))
        return -1;
      x9 = XEXP (x3, 0);
      if (!rtx_equal_p (x9, operands[0]))
        return -1;
      x10 = XEXP (x3, 1);
      if (!rtx_equal_p (x10, operands[1]))
        return -1;
      x11 = XEXP (x3, 2);
      if (!rtx_equal_p (x11, operands[2])
          || !
#line 5635 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD
   && (INTVAL (operands[3]) == -1
       || (GET_CODE (operands[1]) == CONST_INT
           && (~ INTVAL (operands[3]) & ((1 << INTVAL (operands[1]))- 1)) == 0))))
        return -1;
      return 368; /* *insv_bfchg_mem */

    default:
      return -1;
    }
}

static int
recog_31 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 0);
  switch (GET_CODE (x3))
    {
    case SIGN_EXTEND:
    case ZERO_EXTEND:
      operands[2] = x3;
      if (extend_operator (operands[2], E_DImode)
          && pattern17 (x1, E_SImode) == 0)
        return 303; /* ashldi_extsi */
      if (pnum_clobbers == NULL
          || GET_CODE (x3) != SIGN_EXTEND
          || GET_MODE (x3) != E_DImode
          || pattern17 (x1, E_HImode) != 0)
        return -1;
      *pnum_clobbers = 1;
      return 304; /* ashldi_sexthi */

    case REG:
    case SUBREG:
    case MEM:
      operands[1] = x3;
      x4 = XEXP (x2, 1);
      if (GET_CODE (x4) == CONST_INT)
        {
          x5 = XEXP (x1, 0);
          operands[0] = x5;
          switch (XWINT (x4, 0))
            {
            case  HOST_WIDE_INT_C (1):
              if (register_operand (operands[0], E_DImode)
                  && GET_MODE (x2) == E_DImode
                  && register_operand (operands[1], E_DImode)
                  && 
#line 4451 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 305; /* *ashldi3_const1 */
              break;

            case  HOST_WIDE_INT_C (32):
              if (nonimmediate_operand (operands[0], E_DImode)
                  && GET_MODE (x2) == E_DImode
                  && nonimmediate_operand (operands[1], E_DImode))
                return 306; /* *ashldi3_const32 */
              break;

            case  HOST_WIDE_INT_C (16):
              if (register_operand (operands[0], E_SImode)
                  && GET_MODE (x2) == E_SImode
                  && register_operand (operands[1], E_SImode)
                  && 
#line 4625 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TUNE_68060))
                return 308; /* ashlsi_16 */
              break;

            default:
              break;
            }
          operands[2] = x4;
          switch (GET_MODE (operands[0]))
            {
            case E_DImode:
              if (pattern23 (x2) == 0
                  && 
#line 4598 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE
    && ((INTVAL (operands[2]) >= 1 && INTVAL (operands[2]) <= 3)
	|| INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16
	|| (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63))))
                return 307; /* *ashldi3 */
              break;

            case E_SImode:
              if (pattern22 (x2) == 0
                  && 
#line 4637 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68000_10
   && INTVAL (operands[2]) > 16
   && INTVAL (operands[2]) <= 24))
                return 309; /* ashlsi_17_24 */
              break;

            default:
              break;
            }
        }
      x5 = XEXP (x1, 0);
      switch (GET_CODE (x5))
        {
        case REG:
        case SUBREG:
          switch (pattern5 (x1))
            {
            case 0:
              return 310; /* ashlsi3 */

            case 1:
              if (!
#line 4664 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 311; /* ashlhi3 */

            case 2:
              if (!
#line 4680 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 313; /* ashlqi3 */

            default:
              return -1;
            }

        case STRICT_LOW_PART:
          switch (pattern6 (x1))
            {
            case 0:
              if (!
#line 4672 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 312; /* *m68k.md:4668 */

            case 1:
              if (!
#line 4688 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 314; /* *m68k.md:4684 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_32 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  switch (GET_CODE (x2))
    {
    case REG:
    case SUBREG:
    case MEM:
      res = recog_1 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case STRICT_LOW_PART:
      res = recog_2 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case ZERO_EXTRACT:
      res = recog_4 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    default:
      break;
    }
  x3 = XEXP (x1, 1);
  switch (GET_CODE (x3))
    {
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
      res = recog_7 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case ZERO_EXTEND:
      if (GET_MODE (x3) == E_DImode)
        {
          operands[0] = x2;
          if (register_operand (operands[0], E_DImode))
            {
              x4 = XEXP (x3, 0);
              operands[1] = x4;
              switch (GET_MODE (operands[1]))
                {
                case E_QImode:
                  if (nonimmediate_src_operand (operands[1], E_QImode))
                    return 82; /* zero_extendqidi2 */
                  break;

                case E_HImode:
                  if (nonimmediate_src_operand (operands[1], E_HImode))
                    return 83; /* zero_extendhidi2 */
                  break;

                default:
                  break;
                }
            }
        }
      break;

    case SIGN_EXTEND:
      res = recog_8 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case PLUS:
      res = recog_9 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case MINUS:
      res = recog_10 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case MULT:
      res = recog_11 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case AND:
      res = recog_12 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    default:
      break;
    }
  switch (GET_CODE (x2))
    {
    case REG:
    case SUBREG:
    case MEM:
      operands[0] = x2;
      operands[1] = x3;
      if (nonimmediate_operand (operands[0], E_SImode))
        {
          if (general_operand (operands[1], E_SImode)
              && 
#line 1018 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
            return 55; /* *movsi_cf */
          if (pcrel_address (operands[1], E_SImode)
              && 
#line 1071 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_PCREL))
            return 56; /* *m68k.md:1068 */
        }
      if (address_operand (operands[1], E_SImode)
          && push_operand (operands[0], E_SImode))
        return 76; /* pushasi */
      break;

    case PC:
      operands[0] = x3;
      if (address_operand (operands[0], E_SImode))
        return 405; /* indirect_jump */
      break;

    default:
      break;
    }
  operands[0] = x2;
  switch (GET_CODE (x3))
    {
    case ZERO_EXTEND:
      res = recog_13 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case SIGN_EXTEND:
      res = recog_14 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case PLUS:
      res = recog_15 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case MINUS:
      if (GET_MODE (x3) == E_SImode
          && nonimmediate_operand (operands[0], E_SImode))
        {
          switch (pattern19 (x3))
            {
            case 0:
              return 175; /* subsi3 */

            case 1:
              if (
#line 2992 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 176; /* *m68k.md:2987 */
              break;

            default:
              break;
            }
        }
      break;

    case MULT:
      res = recog_16 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case AND:
      res = recog_17 (x1, insn, pnum_clobbers);
      if (res >= 0)
        return res;
      break;

    case SUBREG:
      if (known_eq (SUBREG_BYTE (x3), 4)
          && GET_MODE (x3) == E_SImode
          && nonimmediate_operand (operands[0], E_SImode))
        {
          x4 = XEXP (x3, 0);
          if (GET_MODE (x4) == E_DImode)
            {
              switch (GET_CODE (x4))
                {
                case ASHIFTRT:
                  x5 = XEXP (x4, 1);
                  if (x5 == const_int_rtx[MAX_SAVED_CONST_INT + 32])
                    {
                      x6 = XEXP (x4, 0);
                      operands[1] = x6;
                      if (general_operand (operands[1], E_DImode))
                        return 318; /* subregsi1ashrdi_const32 */
                    }
                  break;

                case LSHIFTRT:
                  x5 = XEXP (x4, 1);
                  if (x5 == const_int_rtx[MAX_SAVED_CONST_INT + 32])
                    {
                      x6 = XEXP (x4, 0);
                      operands[1] = x6;
                      if (general_operand (operands[1], E_DImode))
                        return 329; /* subreg1lshrdi_const32 */
                    }
                  break;

                default:
                  break;
                }
            }
        }
      break;

    default:
      break;
    }
  if (!nonimmediate_operand (operands[0], E_SImode))
    return -1;
  operands[1] = x3;
  if (!address_operand (operands[1], E_QImode))
    return -1;
  return 406; /* *lea */
}

static int
recog_33 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  x3 = XEXP (x2, 1);
  if (GET_CODE (x3) == CONST_INT)
    {
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      x5 = XEXP (x2, 0);
      operands[1] = x5;
      switch (XWINT (x3, 0))
        {
        case  HOST_WIDE_INT_C (1):
          if (register_operand (operands[0], E_DImode)
              && GET_MODE (x2) == E_DImode
              && register_operand (operands[1], E_DImode)
              && 
#line 4981 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return 330; /* *lshrdi3_const1 */
          break;

        case  HOST_WIDE_INT_C (32):
          if (nonimmediate_operand (operands[0], E_DImode)
              && GET_MODE (x2) == E_DImode
              && general_operand (operands[1], E_DImode))
            return 331; /* *lshrdi_const32 */
          break;

        case  HOST_WIDE_INT_C (63):
          if (register_operand (operands[0], E_DImode)
              && GET_MODE (x2) == E_DImode
              && register_operand (operands[1], E_DImode))
            return 332; /* *lshrdi_const63 */
          break;

        case  HOST_WIDE_INT_C (31):
          if (register_operand (operands[0], E_SImode)
              && GET_MODE (x2) == E_SImode
              && register_operand (operands[1], E_SImode))
            return 334; /* lshrsi_31 */
          break;

        case  HOST_WIDE_INT_C (16):
          if (register_operand (operands[0], E_SImode)
              && GET_MODE (x2) == E_SImode
              && register_operand (operands[1], E_SImode)
              && 
#line 5178 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TUNE_68060))
            return 335; /* lshrsi_16 */
          break;

        default:
          break;
        }
      operands[2] = x3;
      switch (GET_MODE (operands[0]))
        {
        case E_DImode:
          if (pattern23 (x2) == 0
              && 
#line 5140 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((!TARGET_COLDFIRE
    && ((INTVAL (operands[2]) >= 2 && INTVAL (operands[2]) <= 3)
	 || INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16
	 || (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63)))))
            return 333; /* *lshrdi3_const */
          break;

        case E_SImode:
          if (pattern22 (x2) == 0
              && 
#line 5189 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TUNE_68000_10
   && INTVAL (operands[2]) > 16
   && INTVAL (operands[2]) <= 24))
            return 336; /* lshrsi_17_24 */
          break;

        default:
          break;
        }
    }
  x4 = XEXP (x1, 0);
  switch (GET_CODE (x4))
    {
    case REG:
    case SUBREG:
      switch (pattern5 (x1))
        {
        case 0:
          return 337; /* lshrsi3 */

        case 1:
          if (!
#line 5212 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 338; /* lshrhi3 */

        case 2:
          if (!
#line 5228 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 340; /* lshrqi3 */

        default:
          return -1;
        }

    case STRICT_LOW_PART:
      switch (pattern6 (x1))
        {
        case 0:
          if (!
#line 5220 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 339; /* *m68k.md:5216 */

        case 1:
          if (!
#line 5236 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 341; /* *m68k.md:5232 */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_34 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  operands[1] = x4;
  switch (GET_MODE (operands[0]))
    {
    case E_SFmode:
      if (!nonimmediate_operand (operands[0], E_SFmode)
          || GET_MODE (x3) != E_SFmode)
        return -1;
      if (general_operand (operands[1], E_SImode))
        {
          if (
#line 2108 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 104; /* floatsisf2_68881 */
          if (
#line 2115 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return 107; /* floatsisf2_cf */
        }
      if (general_operand (operands[1], E_HImode))
        {
          if (
#line 2129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 109; /* floathisf2_68881 */
          if (
#line 2136 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return 112; /* floathisf2_cf */
        }
      if (!general_operand (operands[1], E_QImode))
        return -1;
      if (
#line 2150 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
        return 114; /* floatqisf2_68881 */
      if (!
#line 2157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
        return -1;
      return 117; /* floatqisf2_cf */

    case E_DFmode:
      if (!nonimmediate_operand (operands[0], E_DFmode)
          || GET_MODE (x3) != E_DFmode)
        return -1;
      if (general_operand (operands[1], E_SImode))
        {
          if (
#line 2108 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 105; /* floatsidf2_68881 */
          if (
#line 2115 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return 108; /* floatsidf2_cf */
        }
      if (general_operand (operands[1], E_HImode))
        {
          if (
#line 2129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 110; /* floathidf2_68881 */
          if (
#line 2136 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return 113; /* floathidf2_cf */
        }
      if (!general_operand (operands[1], E_QImode))
        return -1;
      if (
#line 2150 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
        return 115; /* floatqidf2_68881 */
      if (!
#line 2157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
        return -1;
      return 118; /* floatqidf2_cf */

    case E_XFmode:
      if (!nonimmediate_operand (operands[0], E_XFmode)
          || GET_MODE (x3) != E_XFmode)
        return -1;
      if (general_operand (operands[1], E_SImode)
          && 
#line 2108 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
        return 106; /* floatsixf2_68881 */
      if (general_operand (operands[1], E_HImode)
          && 
#line 2129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
        return 111; /* floathixf2_68881 */
      if (!general_operand (operands[1], E_QImode)
          || !
#line 2150 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
        return -1;
      return 116; /* floatqixf2_68881 */

    default:
      return -1;
    }
}

static int
recog_35 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  x4 = XEXP (x3, 0);
  operands[1] = x4;
  x5 = XEXP (x3, 1);
  switch (GET_CODE (x5))
    {
    case FLOAT:
      switch (pattern16 (x3))
        {
        case 0:
          if (general_operand (operands[2], E_SImode)
              && 
#line 3456 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 224; /* divsf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 3467 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 227; /* divsf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 3478 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 230; /* divsf3_floatqi_68881 */

        case 1:
          if (general_operand (operands[2], E_SImode)
              && 
#line 3456 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 225; /* divdf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 3467 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 228; /* divdf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 3478 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 231; /* divdf3_floatqi_68881 */

        case 2:
          if (general_operand (operands[2], E_SImode)
              && 
#line 3456 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 226; /* divxf3_floatsi_68881 */
          if (general_operand (operands[2], E_HImode)
              && 
#line 3467 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 229; /* divxf3_floathi_68881 */
          if (!general_operand (operands[2], E_QImode)
              || !
#line 3478 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 232; /* divxf3_floatqi_68881 */

        default:
          return -1;
        }

    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      operands[2] = x5;
      switch (GET_MODE (operands[0]))
        {
        case E_SFmode:
          if (pattern10 (x3, E_SFmode) != 0)
            return -1;
          if (
#line 3489 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 233; /* divsf3_68881 */
          if (!
#line 3504 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 236; /* divsf3_cf */

        case E_DFmode:
          if (pattern10 (x3, E_DFmode) != 0)
            return -1;
          if (
#line 3489 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 234; /* divdf3_68881 */
          if (!
#line 3504 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 237; /* divdf3_cf */

        case E_XFmode:
          if (pattern10 (x3, E_XFmode) != 0
              || !
#line 3489 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 235; /* divxf3_68881 */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_36 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11, x12, x13, x14, x15, x16, x17;
  rtx x18, x19, x20, x21, x22, x23, x24, x25;
  rtx x26, x27, x28;
  int res ATTRIBUTE_UNUSED;
  x2 = XVECEXP (x1, 0, 0);
  if (GET_CODE (x2) != SET)
    return -1;
  x3 = XEXP (x2, 1);
  switch (GET_CODE (x3))
    {
    case IF_THEN_ELSE:
      x4 = XEXP (x3, 1);
      if (GET_CODE (x4) != LABEL_REF)
        return -1;
      x5 = XEXP (x3, 2);
      if (GET_CODE (x5) != PC)
        return -1;
      x6 = XEXP (x2, 0);
      if (GET_CODE (x6) != PC)
        return -1;
      x7 = XEXP (x4, 0);
      operands[1] = x7;
      x8 = XEXP (x3, 0);
      switch (GET_CODE (x8))
        {
        case EQ:
          x9 = XEXP (x8, 1);
          if (x9 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
            return -1;
          x10 = XVECEXP (x1, 0, 1);
          if (GET_CODE (x10) != CLOBBER)
            return -1;
          x11 = XEXP (x8, 0);
          operands[0] = x11;
          if (!general_operand (operands[0], E_DImode))
            return -1;
          x12 = XEXP (x10, 0);
          operands[2] = x12;
          if (!scratch_operand (operands[2], E_SImode))
            return -1;
          return 3; /* beq0_di */

        case NE:
          x9 = XEXP (x8, 1);
          if (x9 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
            return -1;
          x11 = XEXP (x8, 0);
          operands[0] = x11;
          x10 = XVECEXP (x1, 0, 1);
          switch (GET_CODE (x10))
            {
            case CLOBBER:
              if (!general_operand (operands[0], E_DImode))
                return -1;
              x12 = XEXP (x10, 0);
              operands[2] = x12;
              if (!scratch_operand (operands[2], E_SImode))
                return -1;
              return 4; /* bne0_di */

            case SET:
              x13 = XEXP (x10, 1);
              if (GET_CODE (x13) != PLUS)
                return -1;
              x14 = XEXP (x13, 1);
              if (x14 != const_int_rtx[MAX_SAVED_CONST_INT + -1])
                return -1;
              x15 = XEXP (x13, 0);
              if (!rtx_equal_p (x15, operands[0]))
                return -1;
              x12 = XEXP (x10, 0);
              if (!rtx_equal_p (x12, operands[0]))
                return -1;
              switch (GET_MODE (operands[0]))
                {
                case E_HImode:
                  if (!nonimmediate_operand (operands[0], E_HImode)
                      || GET_MODE (x13) != E_HImode
                      || !
#line 5869 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                    return -1;
                  return 385; /* *dbne_hi */

                case E_SImode:
                  if (!nonimmediate_operand (operands[0], E_SImode)
                      || GET_MODE (x13) != E_SImode
                      || !
#line 5888 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                    return -1;
                  return 386; /* *dbne_si */

                default:
                  return -1;
                }

            default:
              return -1;
            }

        case GE:
          x11 = XEXP (x8, 0);
          if (GET_CODE (x11) != PLUS)
            return -1;
          x16 = XEXP (x11, 1);
          if (x16 != const_int_rtx[MAX_SAVED_CONST_INT + -1])
            return -1;
          x9 = XEXP (x8, 1);
          if (x9 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
            return -1;
          x10 = XVECEXP (x1, 0, 1);
          if (GET_CODE (x10) != SET)
            return -1;
          x13 = XEXP (x10, 1);
          if (GET_CODE (x13) != PLUS)
            return -1;
          x14 = XEXP (x13, 1);
          if (x14 != const_int_rtx[MAX_SAVED_CONST_INT + -1])
            return -1;
          x17 = XEXP (x11, 0);
          operands[0] = x17;
          x15 = XEXP (x13, 0);
          if (!rtx_equal_p (x15, operands[0]))
            return -1;
          x12 = XEXP (x10, 0);
          if (!rtx_equal_p (x12, operands[0]))
            return -1;
          switch (GET_MODE (x11))
            {
            case E_HImode:
              if (!nonimmediate_operand (operands[0], E_HImode)
                  || GET_MODE (x13) != E_HImode
                  || !
#line 5910 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && find_reg_note (insn, REG_NONNEG, 0)))
                return -1;
              return 387; /* *dbge_hi */

            case E_SImode:
              if (!nonimmediate_operand (operands[0], E_SImode)
                  || GET_MODE (x13) != E_SImode
                  || !
#line 5944 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE && find_reg_note (insn, REG_NONNEG, 0)))
                return -1;
              return 388; /* *dbge_si */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case SIGN_EXTEND:
      if (GET_MODE (x3) != E_DImode)
        return -1;
      x10 = XVECEXP (x1, 0, 1);
      if (GET_CODE (x10) != CLOBBER)
        return -1;
      x6 = XEXP (x2, 0);
      operands[0] = x6;
      if (!nonimmediate_operand (operands[0], E_DImode))
        return -1;
      x8 = XEXP (x3, 0);
      operands[1] = x8;
      if (!nonimmediate_src_operand (operands[1], E_SImode))
        return -1;
      x12 = XEXP (x10, 0);
      operands[2] = x12;
      if (!scratch_operand (operands[2], E_SImode))
        return -1;
      return 92; /* extendsidi2 */

    case PLUS:
      x8 = XEXP (x3, 0);
      switch (GET_CODE (x8))
        {
        case LSHIFTRT:
          if (GET_MODE (x8) != E_DImode
              || pattern21 (x1, 63) != 0)
            return -1;
          x11 = XEXP (x8, 0);
          operands[1] = x11;
          if (!general_operand (operands[1], E_DImode))
            return -1;
          x10 = XVECEXP (x1, 0, 1);
          x12 = XEXP (x10, 0);
          operands[2] = x12;
          if (!scratch_operand (operands[2], E_SImode))
            return -1;
          x4 = XEXP (x3, 1);
          if (!rtx_equal_p (x4, operands[1]))
            return -1;
          return 142; /* adddi_lshrdi_63 */

        case ASHIFT:
          if (GET_MODE (x8) != E_DImode)
            return -1;
          x11 = XEXP (x8, 0);
          if (GET_CODE (x11) != SIGN_EXTEND
              || GET_MODE (x11) != E_DImode
              || pattern21 (x1, 32) != 0)
            return -1;
          x17 = XEXP (x11, 0);
          operands[1] = x17;
          if (!general_operand (operands[1], E_HImode))
            return -1;
          x4 = XEXP (x3, 1);
          operands[2] = x4;
          if (!general_operand (operands[2], E_DImode))
            return -1;
          x10 = XVECEXP (x1, 0, 1);
          x12 = XEXP (x10, 0);
          operands[3] = x12;
          if (!scratch_operand (operands[3], E_SImode)
              || !
#line 2333 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 143; /* adddi_sexthishl32 */

        case CONST_INT:
        case CONST_WIDE_INT:
        case CONST_POLY_INT:
        case CONST_FIXED:
        case CONST_DOUBLE:
        case CONST_VECTOR:
        case CONST:
        case REG:
        case SUBREG:
        case MEM:
        case LABEL_REF:
        case SYMBOL_REF:
        case HIGH:
          operands[1] = x8;
          if (!general_operand (operands[1], E_DImode))
            return -1;
          x10 = XVECEXP (x1, 0, 1);
          if (GET_CODE (x10) != CLOBBER)
            return -1;
          x6 = XEXP (x2, 0);
          operands[0] = x6;
          if (!nonimmediate_operand (operands[0], E_DImode)
              || GET_MODE (x3) != E_DImode)
            return -1;
          x4 = XEXP (x3, 1);
          operands[2] = x4;
          if (!general_operand (operands[2], E_DImode))
            return -1;
          x12 = XEXP (x10, 0);
          operands[3] = x12;
          if (!scratch_operand (operands[3], E_SImode))
            return -1;
          return 147; /* adddi3 */

        case PC:
          x6 = XEXP (x2, 0);
          if (GET_CODE (x6) != PC)
            return -1;
          x10 = XVECEXP (x1, 0, 1);
          if (GET_CODE (x10) != USE)
            return -1;
          x12 = XEXP (x10, 0);
          if (GET_CODE (x12) != LABEL_REF
              || GET_MODE (x3) != E_SImode)
            return -1;
          x18 = XEXP (x12, 0);
          operands[1] = x18;
          x4 = XEXP (x3, 1);
          switch (GET_CODE (x4))
            {
            case REG:
            case SUBREG:
              operands[0] = x4;
              if (!register_operand (operands[0], E_SImode)
                  || !
#line 5825 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_LONG_JUMP_TABLE_OFFSETS))
                return -1;
              return 383; /* *tablejump_pcrel_si */

            case SIGN_EXTEND:
              if (GET_MODE (x4) != E_SImode)
                return -1;
              x7 = XEXP (x4, 0);
              operands[0] = x7;
              if (!register_operand (operands[0], E_HImode)
                  || !
#line 5839 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_LONG_JUMP_TABLE_OFFSETS))
                return -1;
              return 384; /* *tablejump_pcrel_hi */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case MINUS:
      if (GET_MODE (x3) != E_DImode)
        return -1;
      x10 = XVECEXP (x1, 0, 1);
      if (GET_CODE (x10) != CLOBBER)
        return -1;
      x6 = XEXP (x2, 0);
      operands[0] = x6;
      if (!nonimmediate_operand (operands[0], E_DImode))
        return -1;
      x8 = XEXP (x3, 0);
      operands[1] = x8;
      if (!general_operand (operands[1], E_DImode))
        return -1;
      x12 = XEXP (x10, 0);
      operands[3] = x12;
      if (!scratch_operand (operands[3], E_SImode))
        return -1;
      x4 = XEXP (x3, 1);
      switch (GET_CODE (x4))
        {
        case ASHIFT:
          if (GET_MODE (x4) != E_DImode)
            return -1;
          x7 = XEXP (x4, 0);
          if (GET_CODE (x7) != SIGN_EXTEND
              || GET_MODE (x7) != E_DImode)
            return -1;
          x19 = XEXP (x4, 1);
          if (x19 != const_int_rtx[MAX_SAVED_CONST_INT + 32])
            return -1;
          x20 = XEXP (x7, 0);
          operands[2] = x20;
          if (!general_operand (operands[2], E_HImode)
              || !
#line 2864 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 172; /* subdi_sexthishl32 */

        case CONST_INT:
        case CONST_WIDE_INT:
        case CONST_POLY_INT:
        case CONST_FIXED:
        case CONST_DOUBLE:
        case CONST_VECTOR:
        case CONST:
        case REG:
        case SUBREG:
        case MEM:
        case LABEL_REF:
        case SYMBOL_REF:
        case HIGH:
          operands[2] = x4;
          if (!general_operand (operands[2], E_DImode))
            return -1;
          return 174; /* subdi3 */

        default:
          return -1;
        }

    case MULT:
      if (GET_MODE (x3) != E_SImode)
        return -1;
      x10 = XVECEXP (x1, 0, 1);
      if (GET_CODE (x10) != SET)
        return -1;
      x13 = XEXP (x10, 1);
      if (GET_CODE (x13) != TRUNCATE
          || GET_MODE (x13) != E_SImode)
        return -1;
      x15 = XEXP (x13, 0);
      if (GET_CODE (x15) != LSHIFTRT
          || GET_MODE (x15) != E_DImode)
        return -1;
      x21 = XEXP (x15, 0);
      if (GET_CODE (x21) != MULT
          || GET_MODE (x21) != E_DImode)
        return -1;
      x22 = XEXP (x15, 1);
      if (x22 != const_int_rtx[MAX_SAVED_CONST_INT + 32])
        return -1;
      x6 = XEXP (x2, 0);
      operands[0] = x6;
      if (!register_operand (operands[0], E_SImode))
        return -1;
      x8 = XEXP (x3, 0);
      operands[1] = x8;
      if (!register_operand (operands[1], E_SImode))
        return -1;
      x4 = XEXP (x3, 1);
      operands[2] = x4;
      x12 = XEXP (x10, 0);
      operands[3] = x12;
      if (!register_operand (operands[3], E_SImode))
        return -1;
      x23 = XEXP (x21, 0);
      if (GET_MODE (x23) != E_DImode)
        return -1;
      switch (GET_CODE (operands[2]))
        {
        case REG:
        case SUBREG:
        case MEM:
          if (!nonimmediate_operand (operands[2], E_SImode))
            return -1;
          x24 = XEXP (x21, 1);
          if (GET_MODE (x24) != E_DImode)
            return -1;
          switch (GET_CODE (x23))
            {
            case ZERO_EXTEND:
              if (GET_CODE (x24) != ZERO_EXTEND)
                return -1;
              x25 = XEXP (x23, 0);
              if (!rtx_equal_p (x25, operands[1]))
                return -1;
              x26 = XEXP (x24, 0);
              if (!rtx_equal_p (x26, operands[2])
                  || !
#line 3199 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE))
                return -1;
              return 202; /* *m68k.md:3191 */

            case SIGN_EXTEND:
              if (GET_CODE (x24) != SIGN_EXTEND)
                return -1;
              x25 = XEXP (x23, 0);
              if (!rtx_equal_p (x25, operands[1]))
                return -1;
              x26 = XEXP (x24, 0);
              if (!rtx_equal_p (x26, operands[2])
                  || !
#line 3238 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE))
                return -1;
              return 204; /* *m68k.md:3230 */

            default:
              return -1;
            }

        case CONST_INT:
          if (!const_int_operand (operands[2], E_SImode))
            return -1;
          x24 = XEXP (x21, 1);
          if (!rtx_equal_p (x24, operands[2]))
            return -1;
          switch (GET_CODE (x23))
            {
            case ZERO_EXTEND:
              x25 = XEXP (x23, 0);
              if (!rtx_equal_p (x25, operands[1])
                  || !
#line 3214 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE
   && (unsigned) INTVAL (operands[2]) <= 0x7fffffff))
                return -1;
              return 203; /* *m68k.md:3206 */

            case SIGN_EXTEND:
              x25 = XEXP (x23, 0);
              if (!rtx_equal_p (x25, operands[1])
                  || !
#line 3249 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE))
                return -1;
              return 205; /* *m68k.md:3241 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case TRUNCATE:
      if (GET_MODE (x3) != E_SImode)
        return -1;
      x8 = XEXP (x3, 0);
      if (GET_CODE (x8) != LSHIFTRT
          || GET_MODE (x8) != E_DImode)
        return -1;
      x11 = XEXP (x8, 0);
      if (GET_CODE (x11) != MULT
          || GET_MODE (x11) != E_DImode)
        return -1;
      x9 = XEXP (x8, 1);
      if (x9 != const_int_rtx[MAX_SAVED_CONST_INT + 32])
        return -1;
      x10 = XVECEXP (x1, 0, 1);
      if (GET_CODE (x10) != CLOBBER)
        return -1;
      x6 = XEXP (x2, 0);
      operands[0] = x6;
      if (!register_operand (operands[0], E_SImode))
        return -1;
      x17 = XEXP (x11, 0);
      if (GET_MODE (x17) != E_DImode)
        return -1;
      x12 = XEXP (x10, 0);
      operands[1] = x12;
      if (!register_operand (operands[1], E_SImode))
        return -1;
      switch (GET_CODE (x17))
        {
        case ZERO_EXTEND:
          x27 = XEXP (x17, 0);
          operands[2] = x27;
          if (!register_operand (operands[2], E_SImode))
            return -1;
          x16 = XEXP (x11, 1);
          switch (GET_CODE (x16))
            {
            case ZERO_EXTEND:
              if (GET_MODE (x16) != E_DImode)
                return -1;
              x28 = XEXP (x16, 0);
              operands[3] = x28;
              if (!nonimmediate_operand (operands[3], E_SImode)
                  || !
#line 3285 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE))
                return -1;
              return 206; /* *m68k.md:3277 */

            case CONST_INT:
            case CONST_DOUBLE:
              operands[3] = x16;
              if (!const_uint32_operand (operands[3], E_DImode)
                  || !
#line 3296 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE))
                return -1;
              return 207; /* const_umulsi3_highpart */

            default:
              return -1;
            }

        case SIGN_EXTEND:
          x27 = XEXP (x17, 0);
          operands[2] = x27;
          if (!register_operand (operands[2], E_SImode))
            return -1;
          x16 = XEXP (x11, 1);
          switch (GET_CODE (x16))
            {
            case SIGN_EXTEND:
              if (GET_MODE (x16) != E_DImode)
                return -1;
              x28 = XEXP (x16, 0);
              operands[3] = x28;
              if (!nonimmediate_operand (operands[3], E_SImode)
                  || !
#line 3328 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE))
                return -1;
              return 208; /* *m68k.md:3320 */

            case CONST_INT:
              operands[3] = x16;
              if (!const_sint32_operand (operands[3], E_DImode)
                  || !
#line 3339 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE))
                return -1;
              return 209; /* const_smulsi3_highpart */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case DIV:
      switch (pattern8 (x1, MOD))
        {
        case 0:
          if (
#line 3531 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_CF_HWDIV))
            return 238; /* *m68k.md:3525 */
          if (!
#line 3549 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020))
            return -1;
          return 239; /* *m68k.md:3543 */

        case 1:
          if (!
#line 3605 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE || TARGET_CF_HWDIV))
            return -1;
          return 242; /* divmodhi4 */

        default:
          return -1;
        }

    case UDIV:
      switch (pattern8 (x1, UMOD))
        {
        case 0:
          if (
#line 3573 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_CF_HWDIV))
            return 240; /* *m68k.md:3567 */
          if (!
#line 3591 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TARGET_COLDFIRE))
            return -1;
          return 241; /* *m68k.md:3585 */

        case 1:
          if (!
#line 3623 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE || TARGET_CF_HWDIV))
            return -1;
          return 243; /* udivmodhi4 */

        default:
          return -1;
        }

    case ASHIFT:
      if (GET_MODE (x3) != E_DImode)
        return -1;
      x8 = XEXP (x3, 0);
      if (GET_CODE (x8) != SIGN_EXTEND
          || GET_MODE (x8) != E_DImode)
        return -1;
      x4 = XEXP (x3, 1);
      if (x4 != const_int_rtx[MAX_SAVED_CONST_INT + 32])
        return -1;
      x10 = XVECEXP (x1, 0, 1);
      if (GET_CODE (x10) != CLOBBER)
        return -1;
      x6 = XEXP (x2, 0);
      operands[0] = x6;
      if (!nonimmediate_operand (operands[0], E_DImode))
        return -1;
      x11 = XEXP (x8, 0);
      operands[1] = x11;
      if (!general_operand (operands[1], E_HImode))
        return -1;
      x12 = XEXP (x10, 0);
      operands[2] = x12;
      if (!scratch_operand (operands[2], E_SImode))
        return -1;
      return 304; /* ashldi_sexthi */

    case ASHIFTRT:
      if (GET_MODE (x3) != E_DImode)
        return -1;
      x4 = XEXP (x3, 1);
      if (x4 != const_int_rtx[MAX_SAVED_CONST_INT + 32])
        return -1;
      x10 = XVECEXP (x1, 0, 1);
      if (GET_CODE (x10) != CLOBBER)
        return -1;
      x6 = XEXP (x2, 0);
      operands[0] = x6;
      if (!memory_operand (operands[0], E_DImode))
        return -1;
      x8 = XEXP (x3, 0);
      operands[1] = x8;
      if (!nonimmediate_src_operand (operands[1], E_DImode))
        return -1;
      x12 = XEXP (x10, 0);
      operands[2] = x12;
      if (!scratch_operand (operands[2], E_SImode))
        return -1;
      return 321; /* *ashrdi_const32_mem */

    case REG:
    case SUBREG:
      operands[0] = x3;
      if (!register_operand (operands[0], E_SImode))
        return -1;
      x6 = XEXP (x2, 0);
      if (GET_CODE (x6) != PC)
        return -1;
      x10 = XVECEXP (x1, 0, 1);
      if (GET_CODE (x10) != USE)
        return -1;
      x12 = XEXP (x10, 0);
      if (GET_CODE (x12) != LABEL_REF)
        return -1;
      x18 = XEXP (x12, 0);
      operands[1] = x18;
      return 382; /* *tablejump_internal */

    default:
      return -1;
    }
}

static int
recog_37 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11, x12, x13, x14;
  int res ATTRIBUTE_UNUSED;
  x2 = XVECEXP (x1, 0, 0);
  if (GET_CODE (x2) != SET)
    return -1;
  x3 = XVECEXP (x1, 0, 1);
  if (GET_CODE (x3) != CLOBBER)
    return -1;
  x4 = XVECEXP (x1, 0, 2);
  if (GET_CODE (x4) != CLOBBER)
    return -1;
  x5 = XEXP (x4, 0);
  if (!scratch_operand (x5, E_SImode))
    return -1;
  x6 = XEXP (x2, 1);
  switch (GET_CODE (x6))
    {
    case IF_THEN_ELSE:
      x7 = XEXP (x6, 0);
      if (!ordered_comparison_operator (x7, E_VOIDmode))
        return -1;
      operands[1] = x7;
      x8 = XEXP (x6, 1);
      if (GET_CODE (x8) != LABEL_REF)
        return -1;
      x9 = XEXP (x6, 2);
      if (GET_CODE (x9) != PC)
        return -1;
      x10 = XEXP (x2, 0);
      if (GET_CODE (x10) != PC)
        return -1;
      x11 = XEXP (x7, 0);
      operands[2] = x11;
      if (!nonimmediate_operand (operands[2], E_DImode))
        return -1;
      x12 = XEXP (x7, 1);
      operands[3] = x12;
      if (!general_operand (operands[3], E_DImode))
        return -1;
      x13 = XEXP (x8, 0);
      operands[4] = x13;
      x14 = XEXP (x3, 0);
      operands[0] = x14;
      if (!scratch_operand (operands[0], E_DImode))
        return -1;
      operands[5] = x5;
      return 5; /* cbranchdi4_insn */

    case FIX:
      x7 = XEXP (x6, 0);
      if (GET_CODE (x7) != FIX
          || GET_MODE (x7) != E_DFmode)
        return -1;
      x10 = XEXP (x2, 0);
      operands[0] = x10;
      x11 = XEXP (x7, 0);
      operands[1] = x11;
      if (!register_operand (operands[1], E_DFmode))
        return -1;
      x14 = XEXP (x3, 0);
      operands[2] = x14;
      if (!scratch_operand (operands[2], E_SImode))
        return -1;
      operands[3] = x5;
      switch (GET_MODE (operands[0]))
        {
        case E_SImode:
          if (!nonimmediate_operand (operands[0], E_SImode)
              || GET_MODE (x6) != E_SImode
              || !
#line 2172 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TUNE_68040))
            return -1;
          return 119; /* fix_truncdfsi2 */

        case E_HImode:
          if (!nonimmediate_operand (operands[0], E_HImode)
              || GET_MODE (x6) != E_HImode
              || !
#line 2182 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TUNE_68040))
            return -1;
          return 120; /* fix_truncdfhi2 */

        case E_QImode:
          if (!nonimmediate_operand (operands[0], E_QImode)
              || GET_MODE (x6) != E_QImode
              || !
#line 2192 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TUNE_68040))
            return -1;
          return 121; /* fix_truncdfqi2 */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int
recog_38 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11, x12, x13;
  int res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 1);
  switch (GET_CODE (x2))
    {
    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case PLUS:
    case MINUS:
    case MULT:
    case AND:
    case SIGN_EXTEND:
    case ZERO_EXTEND:
    case HIGH:
      return recog_32 (x1, insn, pnum_clobbers);

    case IF_THEN_ELSE:
      x3 = XEXP (x2, 0);
      switch (GET_CODE (x3))
        {
        case NE:
        case EQ:
        case GE:
        case GT:
        case LE:
        case LT:
        case LTGT:
        case GEU:
        case GTU:
        case LEU:
        case LTU:
        case UNORDERED:
        case ORDERED:
        case UNEQ:
        case UNGE:
        case UNGT:
        case UNLE:
        case UNLT:
          x4 = XEXP (x1, 0);
          if (GET_CODE (x4) != PC)
            return -1;
          switch (GET_CODE (x3))
            {
            case NE:
            case EQ:
              res = recog_19 (x1, insn, pnum_clobbers);
              if (res >= 0)
                return res;
              break;

            default:
              break;
            }
          if (ordered_comparison_operator (x3, E_VOIDmode))
            {
              res = recog_20 (x1, insn, pnum_clobbers);
              if (res >= 0)
                return res;
            }
          operands[0] = x3;
          if (!comparison_operator (operands[0], E_VOIDmode))
            return -1;
          x5 = XEXP (x3, 0);
          operands[1] = x5;
          x6 = XEXP (x3, 1);
          operands[2] = x6;
          x7 = XEXP (x2, 1);
          switch (GET_CODE (x7))
            {
            case LABEL_REF:
              x8 = XEXP (x2, 2);
              if (GET_CODE (x8) != PC)
                return -1;
              x9 = XEXP (x7, 0);
              operands[3] = x9;
              switch (pattern25 ())
                {
                case 0:
                  if (
#line 745 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], SFmode)
       || register_operand (operands[2], SFmode)
       || const0_operand (operands[2], SFmode))))
                    return 32; /* cbranchsf4_insn_68881 */
                  if (!
#line 763 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], SFmode)
       || register_operand (operands[2], SFmode)
       || const0_operand (operands[2], SFmode))))
                    return -1;
                  return 35; /* cbranchsf4_insn_cf */

                case 1:
                  if (
#line 745 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], DFmode)
       || register_operand (operands[2], DFmode)
       || const0_operand (operands[2], DFmode))))
                    return 33; /* cbranchdf4_insn_68881 */
                  if (!
#line 763 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], DFmode)
       || register_operand (operands[2], DFmode)
       || const0_operand (operands[2], DFmode))))
                    return -1;
                  return 36; /* cbranchdf4_insn_cf */

                case 2:
                  if ((
#line 745 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], XFmode)
       || register_operand (operands[2], XFmode)
       || const0_operand (operands[2], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)))
                    return 34; /* cbranchxf4_insn_68881 */
                  if (!(
#line 763 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], XFmode)
       || register_operand (operands[2], XFmode)
       || const0_operand (operands[2], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)))
                    return -1;
                  return 37; /* cbranchxf4_insn_cf */

                default:
                  return -1;
                }

            case PC:
              x8 = XEXP (x2, 2);
              if (GET_CODE (x8) != LABEL_REF)
                return -1;
              x10 = XEXP (x8, 0);
              operands[3] = x10;
              switch (pattern25 ())
                {
                case 0:
                  if (
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], SFmode)
       || register_operand (operands[2], SFmode)
       || const0_operand (operands[2], SFmode))))
                    return 38; /* cbranchsf4_insn_rev_68881 */
                  if (!
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], SFmode)
       || register_operand (operands[2], SFmode)
       || const0_operand (operands[2], SFmode))))
                    return -1;
                  return 41; /* cbranchsf4_insn_rev_cf */

                case 1:
                  if (
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], DFmode)
       || register_operand (operands[2], DFmode)
       || const0_operand (operands[2], DFmode))))
                    return 39; /* cbranchdf4_insn_rev_68881 */
                  if (!
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], DFmode)
       || register_operand (operands[2], DFmode)
       || const0_operand (operands[2], DFmode))))
                    return -1;
                  return 42; /* cbranchdf4_insn_rev_cf */

                case 2:
                  if ((
#line 781 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881
   && (register_operand (operands[1], XFmode)
       || register_operand (operands[2], XFmode)
       || const0_operand (operands[2], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)))
                    return 40; /* cbranchxf4_insn_rev_68881 */
                  if (!(
#line 799 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU
   && (register_operand (operands[1], XFmode)
       || register_operand (operands[2], XFmode)
       || const0_operand (operands[2], XFmode))) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)))
                    return -1;
                  return 43; /* cbranchxf4_insn_rev_cf */

                default:
                  return -1;
                }

            default:
              return -1;
            }

        default:
          return -1;
        }

    case NE:
    case EQ:
    case GE:
    case GT:
    case LE:
    case LT:
    case LTGT:
    case GEU:
    case GTU:
    case LEU:
    case LTU:
    case UNORDERED:
    case ORDERED:
    case UNEQ:
    case UNGE:
    case UNGT:
    case UNLE:
    case UNLT:
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      if (register_operand (operands[0], E_QImode))
        {
          res = recog_22 (x1, insn, pnum_clobbers);
          if (res >= 0)
            return res;
        }
      if (!nonimmediate_operand (operands[0], E_QImode)
          || !ordered_comparison_operator (x2, E_VOIDmode))
        return -1;
      operands[1] = x2;
      x3 = XEXP (x2, 0);
      operands[2] = x3;
      if (!general_operand (operands[2], E_DImode))
        return -1;
      x7 = XEXP (x2, 1);
      if (x7 == const_int_rtx[MAX_SAVED_CONST_INT + 0])
        {
          if (
#line 5754 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_COLDFIRE))
            return 377; /* scc0_di */
          if (
#line 5763 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
            return 378; /* scc0_di_5200 */
        }
      operands[3] = x7;
      if (!general_operand (operands[3], E_DImode))
        return -1;
      if (
#line 5773 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(! TARGET_COLDFIRE))
        return 379; /* scc_di */
      if (!
#line 5783 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
        return -1;
      return 380; /* scc_di_5200 */

    case TRUNCATE:
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      x3 = XEXP (x2, 0);
      operands[1] = x3;
      switch (GET_MODE (operands[0]))
        {
        case E_QImode:
          if (!nonimmediate_operand (operands[0], E_QImode)
              || GET_MODE (x2) != E_QImode)
            return -1;
          if (general_src_operand (operands[1], E_SImode)
              && 
#line 1631 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return 77; /* truncsiqi2 */
          if (!general_src_operand (operands[1], E_HImode)
              || !
#line 1649 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 78; /* trunchiqi2 */

        case E_HImode:
          if (!nonimmediate_operand (operands[0], E_HImode)
              || GET_MODE (x2) != E_HImode
              || !general_src_operand (operands[1], E_SImode)
              || !
#line 1672 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 79; /* truncsihi2 */

        default:
          return -1;
        }

    case FLOAT_EXTEND:
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      x3 = XEXP (x2, 0);
      operands[1] = x3;
      switch (GET_MODE (operands[0]))
        {
        case E_DFmode:
          if (!nonimmediate_operand (operands[0], E_DFmode)
              || GET_MODE (x2) != E_DFmode
              || !general_operand (operands[1], E_SFmode))
            return -1;
          if (
#line 2012 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 99; /* *m68k.md:2008 */
          if (!
#line 2038 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 100; /* extendsfdf2_cf */

        case E_XFmode:
          if (!nonimmediate_operand (operands[0], E_XFmode)
              || GET_MODE (x2) != E_XFmode)
            return -1;
          switch (GET_MODE (operands[1]))
            {
            case E_SFmode:
              if (!general_operand (operands[1], E_SFmode)
                  || !
#line 6523 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return -1;
              return 407; /* extendsfxf2 */

            case E_DFmode:
              if (!general_operand (operands[1], E_DFmode)
                  || !
#line 6552 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return -1;
              return 408; /* extenddfxf2 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case FLOAT_TRUNCATE:
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      x3 = XEXP (x2, 0);
      operands[1] = x3;
      switch (GET_MODE (operands[0]))
        {
        case E_SFmode:
          if (!nonimmediate_operand (operands[0], E_SFmode)
              || GET_MODE (x2) != E_SFmode)
            return -1;
          switch (GET_MODE (operands[1]))
            {
            case E_DFmode:
              if (!general_operand (operands[1], E_DFmode))
                return -1;
              if (
#line 2066 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && TARGET_68040))
                return 101; /* *m68k.md:2062 */
              if (
#line 2077 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
                return 102; /* truncdfsf2_cf */
              if (!
#line 2087 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return -1;
              return 103; /* *truncdfsf2_68881 */

            case E_XFmode:
              if (!general_operand (operands[1], E_XFmode)
                  || !
#line 6599 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
                return -1;
              return 410; /* truncxfsf2 */

            default:
              return -1;
            }

        case E_DFmode:
          if (!nonimmediate_operand (operands[0], E_DFmode)
              || GET_MODE (x2) != E_DFmode
              || !general_operand (operands[1], E_XFmode)
              || !
#line 6584 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 409; /* truncxfdf2 */

        default:
          return -1;
        }

    case FLOAT:
      return recog_34 (x1, insn, pnum_clobbers);

    case FIX:
      return recog_27 (x1, insn, pnum_clobbers);

    case DIV:
      return recog_35 (x1, insn, pnum_clobbers);

    case IOR:
      return recog_28 (x1, insn, pnum_clobbers);

    case XOR:
      return recog_30 (x1, insn, pnum_clobbers);

    case NEG:
      return recog_29 (x1, insn, pnum_clobbers);

    case SQRT:
      switch (pattern2 (x1))
        {
        case 0:
          if (
#line 4183 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 285; /* sqrtsf2_68881 */
          if (!
#line 4194 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 288; /* sqrtsf2_cf */

        case 1:
          if (
#line 4183 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 286; /* sqrtdf2_68881 */
          if (!
#line 4194 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 289; /* sqrtdf2_cf */

        case 2:
          if (!
#line 4183 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 287; /* sqrtxf2_68881 */

        default:
          return -1;
        }

    case ABS:
      switch (pattern2 (x1))
        {
        case 0:
          if (
#line 4298 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 290; /* abssf2_68881 */
          if (!
#line 4313 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 293; /* abssf2_cf */

        case 1:
          if (
#line 4298 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return 291; /* absdf2_68881 */
          if (!
#line 4313 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE_FPU))
            return -1;
          return 294; /* absdf2_cf */

        case 2:
          if (!
#line 4298 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881))
            return -1;
          return 292; /* absxf2_68881 */

        default:
          return -1;
        }

    case CLZ:
      if (GET_MODE (x2) != E_SImode)
        return -1;
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      if (!register_operand (operands[0], E_SImode))
        return -1;
      x3 = XEXP (x2, 0);
      operands[1] = x3;
      if (general_operand (operands[1], E_SImode)
          && 
#line 4340 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD))
        return 295; /* *clzsi2_68k */
      if (!register_operand (operands[1], E_SImode)
          || !
#line 4347 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(ISA_HAS_FF1))
        return -1;
      return 296; /* *clzsi2_cf */

    case NOT:
      switch (pattern1 (x1))
        {
        case 0:
          switch (GET_MODE (operands[0]))
            {
            case E_SImode:
              if (!nonimmediate_operand (operands[0], E_SImode)
                  || GET_MODE (x2) != E_SImode
                  || !general_operand (operands[1], E_SImode))
                return -1;
              if (
#line 4368 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return 297; /* one_cmplsi2_internal */
              if (!
#line 4375 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE))
                return -1;
              return 298; /* one_cmplsi2_5200 */

            case E_HImode:
              if (!nonimmediate_operand (operands[0], E_HImode)
                  || GET_MODE (x2) != E_HImode
                  || !general_operand (operands[1], E_HImode)
                  || !
#line 4382 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 299; /* one_cmplhi2 */

            case E_QImode:
              if (!nonimmediate_operand (operands[0], E_QImode)
                  || GET_MODE (x2) != E_QImode
                  || !general_operand (operands[1], E_QImode)
                  || !
#line 4396 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 301; /* one_cmplqi2 */

            default:
              return -1;
            }

        case 1:
          if (!
#line 4389 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 300; /* *m68k.md:4386 */

        case 2:
          if (!
#line 4403 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
            return -1;
          return 302; /* *m68k.md:4400 */

        default:
          return -1;
        }

    case ASHIFT:
      return recog_31 (x1, insn, pnum_clobbers);

    case ASHIFTRT:
      x7 = XEXP (x2, 1);
      if (GET_CODE (x7) == CONST_INT)
        {
          res = recog_24 (x1, insn, pnum_clobbers);
          if (res >= 0)
            return res;
        }
      x4 = XEXP (x1, 0);
      switch (GET_CODE (x4))
        {
        case REG:
        case SUBREG:
          switch (pattern5 (x1))
            {
            case 0:
              return 324; /* ashrsi3 */

            case 1:
              if (!
#line 4910 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 325; /* ashrhi3 */

            case 2:
              if (!
#line 4926 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 327; /* ashrqi3 */

            default:
              return -1;
            }

        case STRICT_LOW_PART:
          switch (pattern6 (x1))
            {
            case 0:
              if (!
#line 4918 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 326; /* *m68k.md:4914 */

            case 1:
              if (!
#line 4934 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 328; /* *m68k.md:4930 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case LSHIFTRT:
      return recog_33 (x1, insn, pnum_clobbers);

    case ROTATE:
      if (GET_MODE (x2) == E_SImode)
        {
          x7 = XEXP (x2, 1);
          if (x7 == const_int_rtx[MAX_SAVED_CONST_INT + 16])
            {
              x4 = XEXP (x1, 0);
              operands[0] = x4;
              if (register_operand (operands[0], E_SImode))
                {
                  x3 = XEXP (x2, 0);
                  operands[1] = x3;
                  if (register_operand (operands[1], E_SImode))
                    return 342; /* rotlsi_16 */
                }
            }
        }
      x4 = XEXP (x1, 0);
      switch (GET_CODE (x4))
        {
        case REG:
        case SUBREG:
          switch (pattern5 (x1))
            {
            case 0:
              if (!
#line 5255 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 343; /* rotlsi3 */

            case 1:
              if (!
#line 5273 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 344; /* rotlhi3 */

            case 2:
              if (!
#line 5305 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 346; /* rotlqi3 */

            default:
              return -1;
            }

        case STRICT_LOW_PART:
          switch (pattern6 (x1))
            {
            case 0:
              if (!
#line 5289 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 345; /* *rotlhi3_lowpart */

            case 1:
              if (!
#line 5321 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 347; /* *rotlqi3_lowpart */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case ROTATERT:
      x4 = XEXP (x1, 0);
      switch (GET_CODE (x4))
        {
        case REG:
        case SUBREG:
          switch (pattern5 (x1))
            {
            case 0:
              if (!
#line 5337 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 348; /* rotrsi3 */

            case 1:
              if (!
#line 5345 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 349; /* rotrhi3 */

            case 2:
              if (!
#line 5359 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 351; /* rotrqi3 */

            default:
              return -1;
            }

        case STRICT_LOW_PART:
          switch (pattern6 (x1))
            {
            case 0:
              if (!
#line 5352 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 350; /* rotrhi_lowpart */

            case 1:
              if (!
#line 5367 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!TARGET_COLDFIRE))
                return -1;
              return 352; /* *m68k.md:5363 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case ZERO_EXTRACT:
      if (GET_MODE (x2) != E_SImode)
        return -1;
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      if (nonimmediate_operand (operands[0], E_SImode))
        {
          res = recog_25 (x1, insn, pnum_clobbers);
          if (res >= 0)
            return res;
        }
      if (!register_operand (operands[0], E_SImode))
        return -1;
      x3 = XEXP (x2, 0);
      operands[1] = x3;
      if (!memory_operand (operands[1], E_QImode))
        return -1;
      x7 = XEXP (x2, 1);
      operands[2] = x7;
      if (!nonmemory_operand (operands[2], E_SImode))
        return -1;
      x8 = XEXP (x2, 2);
      operands[3] = x8;
      if (!nonmemory_operand (operands[3], E_SImode)
          || !
#line 5624 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD))
        return -1;
      return 367; /* *extzv_bfextu_mem */

    case SIGN_EXTRACT:
      if (GET_MODE (x2) != E_SImode)
        return -1;
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      if (nonimmediate_operand (operands[0], E_SImode))
        {
          res = recog_26 (x1, insn, pnum_clobbers);
          if (res >= 0)
            return res;
        }
      if (!register_operand (operands[0], E_SImode))
        return -1;
      x3 = XEXP (x2, 0);
      operands[1] = x3;
      if (!memory_operand (operands[1], E_QImode))
        return -1;
      x7 = XEXP (x2, 1);
      operands[2] = x7;
      if (!nonmemory_operand (operands[2], E_SImode))
        return -1;
      x8 = XEXP (x2, 2);
      operands[3] = x8;
      if (!nonmemory_operand (operands[3], E_SImode)
          || !
#line 5608 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD))
        return -1;
      return 366; /* *extv_bfexts_mem */

    case CALL:
      x3 = XEXP (x2, 0);
      if (GET_CODE (x3) != MEM
          || GET_MODE (x3) != E_QImode)
        return -1;
      x4 = XEXP (x1, 0);
      operands[0] = x4;
      x5 = XEXP (x3, 0);
      operands[1] = x5;
      x7 = XEXP (x2, 1);
      operands[2] = x7;
      if (!general_operand (operands[2], E_SImode))
        return -1;
      if (sibcall_operand (operands[1], E_SImode)
          && 
#line 5982 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(SIBLING_CALL_P (insn)))
        return 390; /* *sibcall_value */
      if (non_symbolic_call_operand (operands[1], E_SImode)
          && 
#line 6025 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn)))
        return 392; /* *non_symbolic_call_value */
      if (!symbolic_operand (operands[1], E_SImode))
        return -1;
      if (
#line 6035 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn) && m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_JSR))
        return 393; /* *symbolic_call_value_jsr */
      if (!
#line 6048 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn)
   && (m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_BSR_C
       || m68k_symbolic_call_var == M68K_SYMBOLIC_CALL_BSR_P)))
        return -1;
      return 394; /* *symbolic_call_value_bsr */

    case UNSPEC:
      switch (XVECLEN (x2, 0))
        {
        case 1:
          x4 = XEXP (x1, 0);
          operands[0] = x4;
          switch (XINT (x2, 1))
            {
            case 3:
              if (GET_MODE (x2) != E_SImode)
                return -1;
              x11 = XVECEXP (x2, 0, 0);
              if (x11 != const_int_rtx[MAX_SAVED_CONST_INT + 0]
                  || !register_operand (operands[0], E_SImode))
                return -1;
              return 404; /* load_got */

            case 1:
              switch (pattern18 (x2))
                {
                case 0:
                  if (!
#line 6606 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations))
                    return -1;
                  return 411; /* sinsf2 */

                case 1:
                  if (!
#line 6606 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations))
                    return -1;
                  return 412; /* sindf2 */

                case 2:
                  if (!(
#line 6606 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)))
                    return -1;
                  return 413; /* sinxf2 */

                default:
                  return -1;
                }

            case 2:
              switch (pattern18 (x2))
                {
                case 0:
                  if (!
#line 6618 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations))
                    return -1;
                  return 414; /* cossf2 */

                case 1:
                  if (!
#line 6618 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations))
                    return -1;
                  return 415; /* cosdf2 */

                case 2:
                  if (!(
#line 6618 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881 && flag_unsafe_math_optimizations) && 
#line 278 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68881)))
                    return -1;
                  return 416; /* cosxf2 */

                default:
                  return -1;
                }

            default:
              return -1;
            }

        case 2:
          if (XINT (x2, 1) != 5
              || GET_MODE (x2) != E_BLKmode)
            return -1;
          x4 = XEXP (x1, 0);
          if (GET_CODE (x4) != MEM
              || GET_MODE (x4) != E_BLKmode)
            return -1;
          x12 = XEXP (x4, 0);
          if (GET_CODE (x12) != SCRATCH)
            return -1;
          x11 = XVECEXP (x2, 0, 0);
          operands[0] = x11;
          if (!register_operand (operands[0], E_SImode))
            return -1;
          x13 = XVECEXP (x2, 0, 1);
          operands[1] = x13;
          if (!register_operand (operands[1], E_SImode))
            return -1;
          return 424; /* stack_tie */

        default:
          return -1;
        }

    default:
      return -1;
    }
}

int
recog (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pnum_clobbers ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11, x12, x13, x14, x15, x16, x17;
  rtx x18, x19, x20, x21, x22, x23, x24, x25;
  rtx x26, x27, x28, x29, x30, x31, x32;
  int res ATTRIBUTE_UNUSED;
  recog_data.insn = NULL;
  switch (GET_CODE (x1))
    {
    case SET:
      return recog_38 (x1, insn, pnum_clobbers);

    case PARALLEL:
      switch (XVECLEN (x1, 0))
        {
        case 2:
          res = recog_36 (x1, insn, pnum_clobbers);
          if (res >= 0)
            return res;
          break;

        case 3:
          res = recog_37 (x1, insn, pnum_clobbers);
          if (res >= 0)
            return res;
          break;

        default:
          break;
        }
      if (XVECLEN (x1, 0) >= 1)
        {
          operands[0] = x1;
          x2 = XVECEXP (x1, 0, 0);
          operands[1] = x2;
          if (
#line 6158 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], NULL, 0, true)))
            return 398; /* *m68k_store_multiple */
          if (GET_CODE (x2) == SET
              && pattern9 (x2) == 0
              && 
#line 6168 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], operands[1], INTVAL (operands[3]), true)))
            return 399; /* *m68k_store_multiple_automod */
          operands[1] = x2;
          if (
#line 6175 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], NULL, 0, false)))
            return 400; /* *m68k_load_multiple */
          if (GET_CODE (x2) == SET
              && pattern9 (x2) == 0
              && 
#line 6185 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_movem_pattern_p (operands[0], operands[1],
			 INTVAL (operands[3]), false)))
            return 401; /* *m68k_load_multiple_automod */
        }
      switch (XVECLEN (x1, 0))
        {
        case 3:
          x2 = XVECEXP (x1, 0, 0);
          if (GET_CODE (x2) != SET)
            return -1;
          x3 = XVECEXP (x1, 0, 1);
          if (GET_CODE (x3) != SET)
            return -1;
          x4 = XVECEXP (x1, 0, 2);
          if (GET_CODE (x4) != SET)
            return -1;
          x5 = XEXP (x2, 1);
          switch (GET_CODE (x5))
            {
            case PLUS:
              if (GET_MODE (x5) != E_SImode)
                return -1;
              x6 = XEXP (x5, 0);
              if (GET_CODE (x6) != REG
                  || REGNO (x6) != 15
                  || GET_MODE (x6) != E_SImode)
                return -1;
              x7 = XEXP (x5, 1);
              if (x7 != const_int_rtx[MAX_SAVED_CONST_INT + -4])
                return -1;
              x8 = XEXP (x3, 0);
              if (GET_CODE (x8) != MEM
                  || GET_MODE (x8) != E_SImode)
                return -1;
              x9 = XEXP (x8, 0);
              if (GET_CODE (x9) != PLUS
                  || GET_MODE (x9) != E_SImode)
                return -1;
              x10 = XEXP (x9, 0);
              if (GET_CODE (x10) != REG
                  || REGNO (x10) != 15
                  || GET_MODE (x10) != E_SImode)
                return -1;
              x11 = XEXP (x9, 1);
              if (x11 != const_int_rtx[MAX_SAVED_CONST_INT + -4])
                return -1;
              x12 = XEXP (x4, 1);
              if (GET_CODE (x12) != PLUS
                  || GET_MODE (x12) != E_SImode)
                return -1;
              x13 = XEXP (x12, 0);
              if (GET_CODE (x13) != REG
                  || REGNO (x13) != 15
                  || GET_MODE (x13) != E_SImode)
                return -1;
              x14 = XEXP (x4, 0);
              if (GET_CODE (x14) != REG
                  || REGNO (x14) != 15
                  || GET_MODE (x14) != E_SImode)
                return -1;
              x15 = XEXP (x2, 0);
              operands[0] = x15;
              if (!register_operand (operands[0], E_SImode))
                return -1;
              x16 = XEXP (x12, 1);
              operands[1] = x16;
              if (!const_int_operand (operands[1], E_SImode))
                return -1;
              x17 = XEXP (x3, 1);
              if (!rtx_equal_p (x17, operands[0])
                  || !
#line 6215 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 || INTVAL (operands[1]) >= -0x8004))
                return -1;
              return 402; /* *link */

            case UNSPEC_VOLATILE:
              if (XVECLEN (x5, 0) != 3
                  || XINT (x5, 1) != 1)
                return -1;
              x17 = XEXP (x3, 1);
              if (GET_CODE (x17) != UNSPEC_VOLATILE
                  || XVECLEN (x17, 0) != 3
                  || XINT (x17, 1) != 2)
                return -1;
              x12 = XEXP (x4, 1);
              if (GET_CODE (x12) != UNSPEC_VOLATILE
                  || XVECLEN (x12, 0) != 3
                  || XINT (x12, 1) != 2
                  || GET_MODE (x12) != E_QImode)
                return -1;
              x15 = XEXP (x2, 0);
              operands[1] = x15;
              x18 = XVECEXP (x5, 0, 0);
              operands[2] = x18;
              x19 = XVECEXP (x5, 0, 1);
              operands[3] = x19;
              x20 = XVECEXP (x5, 0, 2);
              operands[4] = x20;
              x14 = XEXP (x4, 0);
              operands[0] = x14;
              if (!register_operand (operands[0], E_QImode))
                return -1;
              x21 = XVECEXP (x17, 0, 0);
              if (!rtx_equal_p (x21, operands[2]))
                return -1;
              x22 = XVECEXP (x17, 0, 1);
              if (!rtx_equal_p (x22, operands[3]))
                return -1;
              x23 = XVECEXP (x17, 0, 2);
              if (!rtx_equal_p (x23, operands[4]))
                return -1;
              x8 = XEXP (x3, 0);
              if (!rtx_equal_p (x8, operands[2]))
                return -1;
              x24 = XVECEXP (x12, 0, 0);
              if (!rtx_equal_p (x24, operands[2]))
                return -1;
              x25 = XVECEXP (x12, 0, 1);
              if (!rtx_equal_p (x25, operands[3]))
                return -1;
              x26 = XVECEXP (x12, 0, 2);
              if (!rtx_equal_p (x26, operands[4]))
                return -1;
              switch (GET_MODE (operands[1]))
                {
                case E_QImode:
                  if (pattern27 (x1, E_QImode) != 0
                      || !
#line 54 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
(TARGET_CAS))
                    return -1;
                  return 426; /* atomic_compare_and_swapqi_1 */

                case E_HImode:
                  if (pattern27 (x1, E_HImode) != 0
                      || !
#line 54 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
(TARGET_CAS))
                    return -1;
                  return 427; /* atomic_compare_and_swaphi_1 */

                case E_SImode:
                  if (pattern27 (x1, E_SImode) != 0
                      || !
#line 54 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
(TARGET_CAS))
                    return -1;
                  return 428; /* atomic_compare_and_swapsi_1 */

                default:
                  return -1;
                }

            default:
              return -1;
            }

        case 2:
          x2 = XVECEXP (x1, 0, 0);
          if (GET_CODE (x2) != SET)
            return -1;
          x3 = XVECEXP (x1, 0, 1);
          if (GET_CODE (x3) != SET)
            return -1;
          x15 = XEXP (x2, 0);
          operands[0] = x15;
          x5 = XEXP (x2, 1);
          switch (GET_CODE (x5))
            {
            case MEM:
              if (GET_MODE (x5) != E_SImode)
                return -1;
              x17 = XEXP (x3, 1);
              if (GET_CODE (x17) != PLUS
                  || GET_MODE (x17) != E_SImode)
                return -1;
              x27 = XEXP (x17, 1);
              if (x27 != const_int_rtx[MAX_SAVED_CONST_INT + 4])
                return -1;
              x8 = XEXP (x3, 0);
              if (GET_CODE (x8) != REG
                  || REGNO (x8) != 15
                  || GET_MODE (x8) != E_SImode
                  || !register_operand (operands[0], E_SImode))
                return -1;
              x6 = XEXP (x5, 0);
              if (!rtx_equal_p (x6, operands[0]))
                return -1;
              x28 = XEXP (x17, 0);
              if (!rtx_equal_p (x28, operands[0]))
                return -1;
              return 403; /* *unlink */

            case UNSPEC_VOLATILE:
              if (XVECLEN (x5, 0) != 1
                  || XINT (x5, 1) != 3
                  || GET_MODE (x5) != E_QImode)
                return -1;
              x17 = XEXP (x3, 1);
              if (GET_CODE (x17) != UNSPEC_VOLATILE
                  || XVECLEN (x17, 0) != 1
                  || XINT (x17, 1) != 4
                  || GET_MODE (x17) != E_QImode
                  || !register_operand (operands[0], E_QImode))
                return -1;
              x18 = XVECEXP (x5, 0, 0);
              operands[1] = x18;
              if (!memory_operand (operands[1], E_QImode))
                return -1;
              x21 = XVECEXP (x17, 0, 0);
              if (!rtx_equal_p (x21, operands[1]))
                return -1;
              x8 = XEXP (x3, 0);
              if (!rtx_equal_p (x8, operands[1])
                  || !
#line 79 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
(ISA_HAS_TAS))
                return -1;
              return 429; /* atomic_test_and_set_1 */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case CALL:
      x29 = XEXP (x1, 0);
      if (GET_CODE (x29) != MEM
          || GET_MODE (x29) != E_QImode)
        return -1;
      x30 = XEXP (x29, 0);
      operands[0] = x30;
      x31 = XEXP (x1, 1);
      operands[1] = x31;
      if (!general_operand (operands[1], E_SImode))
        return -1;
      if (sibcall_operand (operands[0], E_SImode)
          && 
#line 5964 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(SIBLING_CALL_P (insn)))
        return 389; /* *sibcall */
      if (!call_operand (operands[0], E_SImode)
          || !
#line 6002 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!SIBLING_CALL_P (insn)))
        return -1;
      return 391; /* *call */

    case UNSPEC_VOLATILE:
      if (XVECLEN (x1, 0) != 1
          || XINT (x1, 1) != 0)
        return -1;
      x2 = XVECEXP (x1, 0, 0);
      if (x2 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
        return -1;
      return 395; /* blockage */

    case CONST_INT:
      if (XWINT (x1, 0) !=  HOST_WIDE_INT_C (0))
        return -1;
      return 396; /* nop */

    case RETURN:
      return 397; /* *return */

    case TRAP_IF:
      x29 = XEXP (x1, 0);
      switch (GET_CODE (x29))
        {
        case CONST_INT:
          if (XWINT (x29, 0) !=  HOST_WIDE_INT_C (-1))
            return -1;
          x31 = XEXP (x1, 1);
          if (x31 != const_int_rtx[MAX_SAVED_CONST_INT + 7])
            return -1;
          return 417; /* trap */

        case NE:
        case EQ:
        case GE:
        case GT:
        case LE:
        case LT:
        case GEU:
        case GTU:
        case LEU:
        case LTU:
          operands[0] = x29;
          if (!ordered_comparison_operator (operands[0], E_VOIDmode))
            return -1;
          x30 = XEXP (x29, 0);
          operands[1] = x30;
          x32 = XEXP (x29, 1);
          operands[2] = x32;
          x31 = XEXP (x1, 1);
          operands[3] = x31;
          if (!const1_operand (operands[3], E_SImode))
            return -1;
          switch (GET_MODE (operands[1]))
            {
            case E_QImode:
              if (!nonimmediate_operand (operands[1], E_QImode)
                  || !general_operand (operands[2], E_QImode))
                return -1;
              if (
#line 6642 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TARGET_COLDFIRE))
                return 418; /* ctrapqi4 */
              if (!
#line 6667 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_COLDFIRE))
                return -1;
              return 421; /* ctrapqi4_cf */

            case E_HImode:
              if (!nonimmediate_operand (operands[1], E_HImode)
                  || !general_operand (operands[2], E_HImode))
                return -1;
              if (
#line 6642 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TARGET_COLDFIRE))
                return 419; /* ctraphi4 */
              if (!
#line 6667 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_COLDFIRE))
                return -1;
              return 422; /* ctraphi4_cf */

            case E_SImode:
              if (!nonimmediate_operand (operands[1], E_SImode)
                  || !general_operand (operands[2], E_SImode))
                return -1;
              if (
#line 6642 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && !TARGET_COLDFIRE))
                return 420; /* ctrapsi4 */
              if (!
#line 6667 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_COLDFIRE))
                return -1;
              return 423; /* ctrapsi4_cf */

            default:
              return -1;
            }

        default:
          return -1;
        }

    case UNSPEC:
      if (XVECLEN (x1, 0) != 1
          || XINT (x1, 1) != 4)
        return -1;
      x2 = XVECEXP (x1, 0, 0);
      if (x2 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
        return -1;
      return 425; /* ib */

    default:
      return -1;
    }
}

rtx_insn *
split_insns (rtx x1 ATTRIBUTE_UNUSED, rtx_insn *insn ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6;
  rtx_insn *res ATTRIBUTE_UNUSED;
  recog_data.insn = NULL;
  if (GET_CODE (x1) != SET)
    return NULL;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  x3 = XEXP (x1, 1);
  switch (GET_CODE (x3))
    {
    case CONST_INT:
    case CONST_WIDE_INT:
    case CONST_POLY_INT:
    case CONST_FIXED:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case REG:
    case SUBREG:
    case MEM:
    case LABEL_REF:
    case SYMBOL_REF:
    case HIGH:
      operands[1] = x3;
      switch (GET_MODE (operands[0]))
        {
        case E_DFmode:
          if (!general_operand (operands[1], E_DFmode))
            return NULL;
          if (push_operand (operands[0], E_DFmode)
              && 
#line 298 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed && (extract_constrain_insn_cached (insn), which_alternative == 1)))
            return gen_split_1 (insn, operands);
          if (!nonimmediate_operand (operands[0], E_DFmode)
              || !(
#line 1396 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU) && 
#line 1398 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed)))
            return NULL;
          return gen_split_3 (insn, operands);

        case E_DImode:
          if (!push_operand (operands[0], E_DImode)
              || !general_operand (operands[1], E_DImode)
              || !
#line 311 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed))
            return NULL;
          return gen_split_2 (insn, operands);

        default:
          return NULL;
        }

    case ZERO_EXTEND:
      x4 = XEXP (x3, 0);
      operands[1] = x4;
      switch (GET_CODE (operands[0]))
        {
        case MEM:
          if (register_operand (operands[1], E_VOIDmode))
            {
              if (post_inc_operand (operands[0], E_VOIDmode))
                return gen_split_4 (insn, operands);
              if (pre_dec_operand (operands[0], E_VOIDmode))
                return gen_split_5 (insn, operands);
            }
          break;

        case REG:
        case SUBREG:
          if (register_operand (operands[0], E_DImode)
              && GET_MODE (x3) == E_DImode)
            {
              switch (GET_MODE (operands[1]))
                {
                case E_QImode:
                  if (nonimmediate_src_operand (operands[1], E_QImode))
                    return gen_split_6 (insn, operands);
                  break;

                case E_HImode:
                  if (nonimmediate_src_operand (operands[1], E_HImode))
                    return gen_split_7 (insn, operands);
                  break;

                default:
                  break;
                }
            }
          break;

        default:
          break;
        }
      if (nonimmediate_src_operand (operands[1], E_SImode)
          && nonimmediate_operand (operands[0], E_DImode)
          && GET_MODE (x3) == E_DImode)
        return gen_split_8 (insn, operands);
      if (!nonimmediate_src_operand (operands[1], E_VOIDmode)
          || !register_operand (operands[0], E_VOIDmode))
        return NULL;
      if (
#line 1820 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!ISA_HAS_MVS_MVZ
   && reload_completed
   && reg_mentioned_p (operands[0], operands[1])))
        return gen_split_9 (insn, operands);
      if (!
#line 1836 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!ISA_HAS_MVS_MVZ && reload_completed))
        return NULL;
      return gen_split_10 (insn, operands);

    case PLUS:
      if (GET_MODE (x3) != E_SImode
          || !nonimmediate_operand (operands[0], E_SImode))
        return NULL;
      x4 = XEXP (x3, 0);
      operands[1] = x4;
      if (!general_operand (operands[1], E_SImode))
        return NULL;
      x5 = XEXP (x3, 1);
      operands[2] = x5;
      if (!general_src_operand (operands[2], E_SImode)
          || !(
#line 2516 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE) && 
#line 2550 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed && (extract_constrain_insn_cached (insn), which_alternative == 5) && !operands_match_p (operands[0], operands[1]))))
        return NULL;
      return gen_split_11 (insn, operands);

    case ASHIFT:
      if (GET_MODE (x3) != E_DImode)
        return NULL;
      x5 = XEXP (x3, 1);
      if (GET_CODE (x5) != CONST_INT)
        return NULL;
      x4 = XEXP (x3, 0);
      switch (GET_CODE (x4))
        {
        case SIGN_EXTEND:
        case ZERO_EXTEND:
          operands[2] = x4;
          if (!extend_operator (operands[2], E_DImode)
              || XWINT (x5, 0) !=  HOST_WIDE_INT_C (32)
              || !nonimmediate_operand (operands[0], E_DImode))
            return NULL;
          x6 = XEXP (x4, 0);
          operands[1] = x6;
          if (!general_operand (operands[1], E_SImode)
              || !
#line 4417 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed))
            return NULL;
          return gen_split_12 (insn, operands);

        case REG:
        case SUBREG:
        case MEM:
          operands[1] = x4;
          switch (XWINT (x5, 0))
            {
            case  HOST_WIDE_INT_C (2):
              if (register_operand (operands[0], E_DImode)
                  && register_operand (operands[1], E_DImode)
                  && 
#line 4458 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
                return gen_split_13 (insn, operands);
              break;

            case  HOST_WIDE_INT_C (3):
              if (register_operand (operands[0], E_DImode)
                  && register_operand (operands[1], E_DImode)
                  && 
#line 4469 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
                return gen_split_14 (insn, operands);
              break;

            case  HOST_WIDE_INT_C (8):
              if (register_operand (operands[0], E_DImode)
                  && register_operand (operands[1], E_DImode)
                  && 
#line 4480 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
                return gen_split_15 (insn, operands);
              break;

            case  HOST_WIDE_INT_C (16):
              if (register_operand (operands[0], E_DImode)
                  && register_operand (operands[1], E_DImode)
                  && 
#line 4498 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
                return gen_split_16 (insn, operands);
              break;

            case  HOST_WIDE_INT_C (32):
              if (nonimmediate_operand (operands[1], E_DImode))
                {
                  if (pre_dec_operand (operands[0], E_DImode)
                      && 
#line 4516 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed))
                    return gen_split_17 (insn, operands);
                  if (post_inc_operand (operands[0], E_DImode)
                      && 
#line 4528 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed))
                    return gen_split_18 (insn, operands);
                  if (nonimmediate_operand (operands[0], E_DImode)
                      && 
#line 4542 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
( reload_completed))
                    return gen_split_19 (insn, operands);
                }
              break;

            default:
              break;
            }
          if (!register_operand (operands[1], E_DImode)
              || !register_operand (operands[0], E_DImode))
            return NULL;
          operands[2] = x5;
          if (const_int_operand (operands[2], E_VOIDmode)
              && 
#line 4551 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 40))
            return gen_split_20 (insn, operands);
          if (XWINT (x5, 0) ==  HOST_WIDE_INT_C (48)
              && 
#line 4566 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return gen_split_21 (insn, operands);
          operands[2] = x5;
          if (!const_int_operand (operands[2], E_VOIDmode)
              || !
#line 4582 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 40 && INTVAL (operands[2]) <= 63))
            return NULL;
          return gen_split_22 (insn, operands);

        default:
          return NULL;
        }

    case ASHIFTRT:
      if (GET_MODE (x3) != E_DImode)
        return NULL;
      x5 = XEXP (x3, 1);
      if (GET_CODE (x5) != CONST_INT
          || !register_operand (operands[0], E_DImode))
        return NULL;
      x4 = XEXP (x3, 0);
      operands[1] = x4;
      if (!register_operand (operands[1], E_DImode))
        return NULL;
      switch (XWINT (x5, 0))
        {
        case  HOST_WIDE_INT_C (2):
          if (!
#line 4751 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return NULL;
          return gen_split_23 (insn, operands);

        case  HOST_WIDE_INT_C (3):
          if (!
#line 4762 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return NULL;
          return gen_split_24 (insn, operands);

        case  HOST_WIDE_INT_C (8):
          if (!
#line 4773 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return NULL;
          return gen_split_25 (insn, operands);

        case  HOST_WIDE_INT_C (16):
          if (!
#line 4789 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return NULL;
          return gen_split_26 (insn, operands);

        case  HOST_WIDE_INT_C (63):
          if (!
#line 4835 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return NULL;
          return gen_split_27 (insn, operands);

        default:
          return NULL;
        }

    case LSHIFTRT:
      if (GET_MODE (x3) != E_DImode)
        return NULL;
      x5 = XEXP (x3, 1);
      if (GET_CODE (x5) != CONST_INT)
        return NULL;
      x4 = XEXP (x3, 0);
      operands[1] = x4;
      switch (XWINT (x5, 0))
        {
        case  HOST_WIDE_INT_C (2):
          if (register_operand (operands[0], E_DImode)
              && register_operand (operands[1], E_DImode)
              && 
#line 4990 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return gen_split_28 (insn, operands);
          break;

        case  HOST_WIDE_INT_C (3):
          if (register_operand (operands[0], E_DImode)
              && register_operand (operands[1], E_DImode)
              && 
#line 5001 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return gen_split_29 (insn, operands);
          break;

        case  HOST_WIDE_INT_C (8):
          if (register_operand (operands[0], E_DImode)
              && register_operand (operands[1], E_DImode)
              && 
#line 5012 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return gen_split_30 (insn, operands);
          break;

        case  HOST_WIDE_INT_C (16):
          if (register_operand (operands[0], E_DImode)
              && register_operand (operands[1], E_DImode)
              && 
#line 5028 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE))
            return gen_split_31 (insn, operands);
          break;

        case  HOST_WIDE_INT_C (32):
          if (nonimmediate_operand (operands[1], E_DImode))
            {
              if (pre_dec_operand (operands[0], E_DImode)
                  && 
#line 5046 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed))
                return gen_split_32 (insn, operands);
              if (post_inc_operand (operands[0], E_DImode)
                  && 
#line 5058 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed))
                return gen_split_33 (insn, operands);
              if (nonimmediate_operand (operands[0], E_DImode)
                  && 
#line 5070 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed))
                return gen_split_34 (insn, operands);
            }
          break;

        default:
          break;
        }
      if (!register_operand (operands[1], E_DImode)
          || !register_operand (operands[0], E_DImode))
        return NULL;
      operands[2] = x5;
      if (const_int_operand (operands[2], E_VOIDmode)
          && 
#line 5086 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 40))
        return gen_split_35 (insn, operands);
      if (XWINT (x5, 0) ==  HOST_WIDE_INT_C (48)
          && 
#line 5101 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed))
        return gen_split_36 (insn, operands);
      operands[2] = x5;
      if (!const_int_operand (operands[2], E_VOIDmode)
          || !
#line 5117 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(reload_completed && !TARGET_COLDFIRE
   && INTVAL (operands[2]) > 40 && INTVAL (operands[2]) <= 62))
        return NULL;
      return gen_split_37 (insn, operands);

    default:
      return NULL;
    }
}

static rtx_insn *
peephole2_1 (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pmatch_len_ ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10;
  rtx_insn *res ATTRIBUTE_UNUSED;
  x2 = XEXP (x1, 0);
  operands[0] = x2;
  if (memory_operand (operands[0], E_SImode)
      && const_int_operand (operands[1], E_SImode)
      && 
#line 6349 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(GET_CODE (XEXP (operands[0], 0)) != PRE_DEC
   && INTVAL (operands[1]) != 0
   && IN_RANGE (INTVAL (operands[1]), -0x80, 0x7f)
   && !valid_mov3q_const (INTVAL (operands[1]))))
    {
      *pmatch_len_ = 0;
      res = gen_peephole2_5 (insn, operands);
      if (res != NULL_RTX)
        return res;
    }
  if (peep2_current_count < 2
      || !register_operand (operands[0], E_SImode))
    return NULL;
  x3 = PATTERN (peep2_next_insn (1));
  if (GET_CODE (x3) != SET)
    return NULL;
  x4 = XEXP (x3, 1);
  if (GET_CODE (x4) != IF_THEN_ELSE)
    return NULL;
  x5 = XEXP (x3, 0);
  if (GET_CODE (x5) != PC)
    return NULL;
  x6 = XEXP (x4, 1);
  if (!pc_or_label_operand (x6, E_VOIDmode))
    return NULL;
  operands[3] = x6;
  if (addq_subq_operand (operands[1], E_SImode))
    {
      x7 = XEXP (x4, 0);
      if (equality_comparison_operator (x7, E_VOIDmode))
        {
          operands[5] = x7;
          x8 = XEXP (x7, 0);
          operands[2] = x8;
          if (register_operand (operands[2], E_SImode))
            {
              x9 = XEXP (x4, 2);
              operands[4] = x9;
              if (pc_or_label_operand (operands[4], E_VOIDmode))
                {
                  x10 = XEXP (x7, 1);
                  if (rtx_equal_p (x10, operands[0])
                      && 
#line 6770 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(peep2_reg_dead_p (2, operands[0])
   && peep2_reg_dead_p (2, operands[2])
   && (operands[3] == pc_rtx || operands[4] == pc_rtx)
   && DATA_REG_P (operands[2])
   && !rtx_equal_p (operands[0], operands[2])))
                    {
                      *pmatch_len_ = 1;
                      res = gen_peephole2_14 (insn, operands);
                      if (res != NULL_RTX)
                        return res;
                    }
                }
            }
        }
    }
  operands[4] = x6;
  if (!pow2_m1_operand (operands[1], E_SImode))
    return NULL;
  x9 = XEXP (x4, 2);
  operands[5] = x9;
  if (!pc_or_label_operand (operands[5], E_VOIDmode))
    return NULL;
  x7 = XEXP (x4, 0);
  switch (GET_CODE (x7))
    {
    case GTU:
      x8 = XEXP (x7, 0);
      operands[2] = x8;
      if (!register_operand (operands[2], E_SImode))
        return NULL;
      x10 = XEXP (x7, 1);
      operands[3] = x10;
      if (!register_operand (operands[3], E_SImode)
          || !
#line 6788 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   && operands[0] == operands[3]
   && peep2_reg_dead_p (2, operands[0])
   && peep2_reg_dead_p (2, operands[2])
   && (operands[4] == pc_rtx || operands[5] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[2])))
        return NULL;
      *pmatch_len_ = 1;
      return gen_peephole2_15 (insn, operands);

    case LEU:
      x8 = XEXP (x7, 0);
      operands[2] = x8;
      if (!register_operand (operands[2], E_SImode))
        return NULL;
      x10 = XEXP (x7, 1);
      operands[3] = x10;
      if (!register_operand (operands[3], E_SImode)
          || !
#line 6826 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   && operands[0] == operands[3]
   && peep2_reg_dead_p (2, operands[0])
   && peep2_reg_dead_p (2, operands[2])
   && (operands[4] == pc_rtx || operands[5] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[2])))
        return NULL;
      *pmatch_len_ = 1;
      return gen_peephole2_17 (insn, operands);

    default:
      return NULL;
    }
}

rtx_insn *
peephole2_insns (rtx x1 ATTRIBUTE_UNUSED,
	rtx_insn *insn ATTRIBUTE_UNUSED,
	int *pmatch_len_ ATTRIBUTE_UNUSED)
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x2, x3, x4, x5, x6, x7, x8, x9;
  rtx x10, x11, x12, x13, x14, x15, x16;
  rtx_insn *res ATTRIBUTE_UNUSED;
  recog_data.insn = NULL;
  if (GET_CODE (x1) != SET)
    return NULL;
  x2 = XEXP (x1, 1);
  switch (GET_CODE (x2))
    {
    case PLUS:
      if (peep2_current_count < 2
          || GET_MODE (x2) != E_SImode)
        return NULL;
      x3 = XEXP (x2, 0);
      if (GET_CODE (x3) != REG
          || REGNO (x3) != 15
          || GET_MODE (x3) != E_SImode)
        return NULL;
      x4 = XEXP (x2, 1);
      if (GET_CODE (x4) != CONST_INT)
        return NULL;
      x5 = XEXP (x1, 0);
      if (GET_CODE (x5) != REG
          || REGNO (x5) != 15
          || GET_MODE (x5) != E_SImode)
        return NULL;
      x6 = PATTERN (peep2_next_insn (1));
      if (GET_CODE (x6) != SET)
        return NULL;
      x7 = XEXP (x6, 0);
      operands[0] = x7;
      x8 = XEXP (x6, 1);
      operands[1] = x8;
      switch (XWINT (x4, 0))
        {
        case  HOST_WIDE_INT_C (4):
          switch (GET_CODE (operands[0]))
            {
            case REG:
            case SUBREG:
              if (register_operand (operands[0], E_DFmode)
                  && register_operand (operands[1], E_DFmode)
                  && 
#line 6299 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(FP_REG_P (operands[0]) && !FP_REG_P (operands[1])))
                {
                  *pmatch_len_ = 1;
                  res = gen_peephole2_1 (insn, operands);
                  if (res != NULL_RTX)
                    return res;
                }
              break;

            case MEM:
              switch (GET_MODE (operands[0]))
                {
                case E_SFmode:
                  if (push_operand (operands[0], E_SFmode)
                      && general_operand (operands[1], E_SFmode)
                      && 
#line 6313 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[0])))
                    {
                      *pmatch_len_ = 1;
                      res = gen_peephole2_2 (insn, operands);
                      if (res != NULL_RTX)
                        return res;
                    }
                  break;

                case E_SImode:
                  if (push_operand (operands[0], E_SImode)
                      && general_operand (operands[1], E_SImode)
                      && 
#line 6340 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[1])))
                    {
                      *pmatch_len_ = 1;
                      res = gen_peephole2_4 (insn, operands);
                      if (res != NULL_RTX)
                        return res;
                    }
                  break;

                default:
                  break;
                }
              break;

            default:
              break;
            }
          break;

        case  HOST_WIDE_INT_C (12):
          if (push_operand (operands[0], E_SImode)
              && const_int_operand (operands[1], E_SImode)
              && 
#line 6362 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) != 0
   && IN_RANGE (INTVAL (operands[1]), -0x80, 0x7f)
   && !valid_mov3q_const (INTVAL (operands[1]))))
            {
              *pmatch_len_ = 1;
              res = gen_peephole2_6 (insn, operands);
              if (res != NULL_RTX)
                return res;
            }
          break;

        case  HOST_WIDE_INT_C (-4):
          if (memory_operand (operands[0], E_QImode)
              && register_operand (operands[1], E_QImode)
              && 
#line 6395 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[1])
   && GET_CODE (XEXP (operands[0], 0)) == PLUS
   && rtx_equal_p (XEXP (XEXP (operands[0], 0), 0), stack_pointer_rtx)
   && CONST_INT_P (XEXP (XEXP (operands[0], 0), 1))
   && INTVAL (XEXP (XEXP (operands[0], 0), 1)) == 3))
            {
              *pmatch_len_ = 1;
              res = gen_peephole2_8 (insn, operands);
              if (res != NULL_RTX)
                return res;
            }
          break;

        default:
          break;
        }
      operands[0] = x4;
      if (!const_int_operand (operands[0], E_SImode))
        return NULL;
      operands[1] = x7;
      operands[2] = x8;
      switch (GET_MODE (operands[1]))
        {
        case E_SFmode:
          if (!push_operand (operands[1], E_SFmode)
              || !general_operand (operands[2], E_SFmode)
              || !
#line 6322 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[0]) > 4
   && !reg_mentioned_p (stack_pointer_rtx, operands[2])))
            return NULL;
          *pmatch_len_ = 1;
          return gen_peephole2_3 (insn, operands);

        case E_SImode:
          if (!push_operand (operands[1], E_SImode)
              || !general_operand (operands[2], E_SImode)
              || !
#line 6376 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[0]) > 4
   && !reg_mentioned_p (stack_pointer_rtx, operands[2])
   && !(CONST_INT_P (operands[2]) && INTVAL (operands[2]) != 0
	&& IN_RANGE (INTVAL (operands[2]), -0x8000, 0x7fff)
	&& !valid_mov3q_const (INTVAL (operands[2])))))
            return NULL;
          *pmatch_len_ = 1;
          return gen_peephole2_7 (insn, operands);

        default:
          return NULL;
        }

    case CONST_INT:
      operands[1] = x2;
      res = peephole2_1 (x1, insn, pmatch_len_);
      if (res != NULL_RTX)
        return res;
      if (peep2_current_count < 2
          || XWINT (x2, 0) !=  HOST_WIDE_INT_C (0))
        return NULL;
      x5 = XEXP (x1, 0);
      operands[0] = x5;
      if (!register_operand (operands[0], E_SImode))
        return NULL;
      x6 = PATTERN (peep2_next_insn (1));
      if (GET_CODE (x6) != SET)
        return NULL;
      x7 = XEXP (x6, 0);
      if (GET_CODE (x7) != STRICT_LOW_PART)
        return NULL;
      x9 = XEXP (x7, 0);
      operands[1] = x9;
      x8 = XEXP (x6, 1);
      operands[2] = x8;
      switch (GET_MODE (operands[1]))
        {
        case E_HImode:
          if (!register_operand (operands[1], E_HImode)
              || !general_operand (operands[2], E_HImode)
              || !
#line 6438 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(REGNO (operands[0]) == REGNO (operands[1])
   && strict_low_part_peephole_ok (HImode, insn, operands[0])))
            return NULL;
          *pmatch_len_ = 1;
          return gen_peephole2_11 (insn, operands);

        case E_QImode:
          if (!register_operand (operands[1], E_QImode)
              || !general_operand (operands[2], E_QImode)
              || !
#line 6448 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(REGNO (operands[0]) == REGNO (operands[1])
   && strict_low_part_peephole_ok (QImode, insn, operands[0])))
            return NULL;
          *pmatch_len_ = 1;
          return gen_peephole2_12 (insn, operands);

        default:
          return NULL;
        }

    case REG:
    case SUBREG:
      if (peep2_current_count < 2)
        return NULL;
      operands[1] = x2;
      x5 = XEXP (x1, 0);
      operands[0] = x5;
      x6 = PATTERN (peep2_next_insn (1));
      if (GET_CODE (x6) != SET)
        return NULL;
      x8 = XEXP (x6, 1);
      if (GET_CODE (x8) != PLUS
          || GET_MODE (x8) != E_SImode)
        return NULL;
      x10 = XEXP (x8, 0);
      if (GET_CODE (x10) != REG
          || REGNO (x10) != 15
          || GET_MODE (x10) != E_SImode)
        return NULL;
      x11 = XEXP (x8, 1);
      if (GET_CODE (x11) != CONST_INT)
        return NULL;
      x7 = XEXP (x6, 0);
      if (GET_CODE (x7) != REG
          || REGNO (x7) != 15
          || GET_MODE (x7) != E_SImode)
        return NULL;
      switch (GET_MODE (operands[0]))
        {
        case E_QImode:
          if (!push_operand (operands[0], E_QImode)
              || !register_operand (operands[1], E_QImode)
              || XWINT (x11, 0) !=  HOST_WIDE_INT_C (-3)
              || !
#line 6411 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[1])))
            return NULL;
          *pmatch_len_ = 1;
          return gen_peephole2_9 (insn, operands);

        case E_HImode:
          if (!push_operand (operands[0], E_HImode)
              || !register_operand (operands[1], E_HImode)
              || XWINT (x11, 0) !=  HOST_WIDE_INT_C (-2)
              || !
#line 6423 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(!reg_mentioned_p (stack_pointer_rtx, operands[1])))
            return NULL;
          *pmatch_len_ = 1;
          return gen_peephole2_10 (insn, operands);

        default:
          return NULL;
        }

    case MEM:
      if (peep2_current_count < 2
          || GET_MODE (x2) != E_SImode)
        return NULL;
      x3 = XEXP (x2, 0);
      if (GET_CODE (x3) != PLUS
          || GET_MODE (x3) != E_SImode)
        return NULL;
      x5 = XEXP (x1, 0);
      operands[0] = x5;
      if (!register_operand (operands[0], E_SImode))
        return NULL;
      x12 = XEXP (x3, 0);
      operands[1] = x12;
      if (!register_operand (operands[1], E_SImode))
        return NULL;
      x13 = XEXP (x3, 1);
      operands[2] = x13;
      if (!const_int_operand (operands[2], E_SImode))
        return NULL;
      x6 = PATTERN (peep2_next_insn (1));
      if (GET_CODE (x6) != SET)
        return NULL;
      x8 = XEXP (x6, 1);
      if (x8 != const_int_rtx[MAX_SAVED_CONST_INT + 0])
        return NULL;
      x7 = XEXP (x6, 0);
      if (GET_CODE (x7) != MEM
          || GET_MODE (x7) != E_QImode)
        return NULL;
      x9 = XEXP (x7, 0);
      if (GET_CODE (x9) != PLUS
          || GET_MODE (x9) != E_SImode)
        return NULL;
      x14 = XEXP (x9, 0);
      operands[3] = x14;
      if (!register_operand (operands[3], E_SImode))
        return NULL;
      x15 = XEXP (x9, 1);
      operands[4] = x15;
      if (!register_operand (operands[4], E_SImode)
          || !
#line 6729 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((optimize_size || !TUNE_68060)
   && (operands[0] == operands[3] || operands[0] == operands[4])
   && ADDRESS_REG_P (operands[1])
   && ADDRESS_REG_P ((operands[0] == operands[3]) ? operands[4] : operands[3])
   && peep2_reg_dead_p (2, operands[3])
   && peep2_reg_dead_p (2, operands[4])))
        return NULL;
      *pmatch_len_ = 1;
      return gen_peephole2_13 (insn, operands);

    case IF_THEN_ELSE:
      x3 = XEXP (x2, 0);
      switch (GET_CODE (x3))
        {
        case GT:
        case LE:
        case GTU:
        case LEU:
          x5 = XEXP (x1, 0);
          if (GET_CODE (x5) != PC)
            return NULL;
          x12 = XEXP (x3, 0);
          operands[0] = x12;
          if (!register_operand (operands[0], E_SImode))
            return NULL;
          x13 = XEXP (x3, 1);
          operands[1] = x13;
          if (pow2_m1_operand (operands[1], E_SImode))
            {
              x4 = XEXP (x2, 1);
              operands[2] = x4;
              if (pc_or_label_operand (operands[2], E_VOIDmode))
                {
                  x16 = XEXP (x2, 2);
                  operands[3] = x16;
                  if (pc_or_label_operand (operands[3], E_VOIDmode))
                    {
                      switch (GET_CODE (x3))
                        {
                        case GTU:
                          if (
#line 6809 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   && peep2_reg_dead_p (1, operands[0])
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[0])))
                            {
                              *pmatch_len_ = 0;
                              res = gen_peephole2_16 (insn, operands);
                              if (res != NULL_RTX)
                                return res;
                            }
                          break;

                        case LEU:
                          if (
#line 6846 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(INTVAL (operands[1]) <= 255
   &&  peep2_reg_dead_p (1, operands[0])
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)
   && (optimize_size || TUNE_68040_60)
   && DATA_REG_P (operands[0])))
                            {
                              *pmatch_len_ = 0;
                              res = gen_peephole2_18 (insn, operands);
                              if (res != NULL_RTX)
                                return res;
                            }
                          break;

                        default:
                          break;
                        }
                    }
                }
            }
          if (GET_CODE (x13) != CONST_INT
              || XWINT (x13, 0) !=  HOST_WIDE_INT_C (65535))
            return NULL;
          operands[1] = x3;
          if (!swap_peephole_relational_operator (operands[1], E_VOIDmode))
            return NULL;
          x4 = XEXP (x2, 1);
          operands[2] = x4;
          if (!pc_or_label_operand (operands[2], E_VOIDmode))
            return NULL;
          x16 = XEXP (x2, 2);
          operands[3] = x16;
          if (!pc_or_label_operand (operands[3], E_VOIDmode)
              || !
#line 6866 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(peep2_reg_dead_p (1, operands[0])
   && (operands[2] == pc_rtx || operands[3] == pc_rtx)
   && (optimize_size || TUNE_68000_10)
   && DATA_REG_P (operands[0])))
            return NULL;
          *pmatch_len_ = 0;
          return gen_peephole2_19 (insn, operands);

        default:
          return NULL;
        }

    default:
      return NULL;
    }
}
