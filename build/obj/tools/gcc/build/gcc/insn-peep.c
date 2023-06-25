/* Generated automatically by the program `genpeep'
from the machine description file `md'.  */

#define IN_TARGET_CODE 1
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "tree.h"
#include "rtl.h"
#include "insn-config.h"
#include "alias.h"
#include "varasm.h"
#include "stor-layout.h"
#include "calls.h"
#include "memmodel.h"
#include "tm_p.h"
#include "regs.h"
#include "output.h"
#include "recog.h"
#include "except.h"
#include "diagnostic-core.h"
#include "flags.h"
#include "tm-constrs.h"

extern rtx peep_operand[];

#define operands peep_operand

rtx_insn *
peephole (rtx_insn *ins1)
{
  rtx_insn *insn ATTRIBUTE_UNUSED;
  rtx x ATTRIBUTE_UNUSED, pat ATTRIBUTE_UNUSED;

  if (NEXT_INSN (ins1)
      && BARRIER_P (NEXT_INSN (ins1)))
    return 0;

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L541;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L541;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L541;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L541;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, QImode)) goto L541;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, QImode)) goto L541;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L541;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L541;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L541; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L541;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L541;
  if (XVECLEN (x, 0) != 2) goto L541;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L541;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L541;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L541;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != NE) goto L541;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  operands[0] = x;
  if (! register_operand (x, HImode)) goto L541;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L541;
  if (XWINT (x, 0) != 0) goto L541;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L541;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L541;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L541;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L541;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L541;
  if (GET_MODE (x) != HImode) goto L541;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L541;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L541;
  if (XWINT (x, 0) != -1) goto L541;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L541;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 541;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L541:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L542;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L542;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L542;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L542;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, QImode)) goto L542;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, QImode)) goto L542;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L542;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L542;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L542; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L542;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L542;
  if (XVECLEN (x, 0) != 2) goto L542;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L542;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L542;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L542;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != NE) goto L542;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  operands[0] = x;
  if (! register_operand (x, SImode)) goto L542;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L542;
  if (XWINT (x, 0) != 0) goto L542;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L542;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L542;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L542;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L542;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L542;
  if (GET_MODE (x) != SImode) goto L542;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L542;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L542;
  if (XWINT (x, 0) != -1) goto L542;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L542;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 542;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L542:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L543;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L543;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L543;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L543;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, HImode)) goto L543;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, HImode)) goto L543;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L543;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L543;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L543; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L543;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L543;
  if (XVECLEN (x, 0) != 2) goto L543;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L543;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L543;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L543;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != NE) goto L543;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  operands[0] = x;
  if (! register_operand (x, HImode)) goto L543;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L543;
  if (XWINT (x, 0) != 0) goto L543;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L543;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L543;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L543;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L543;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L543;
  if (GET_MODE (x) != HImode) goto L543;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L543;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L543;
  if (XWINT (x, 0) != -1) goto L543;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L543;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 543;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L543:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L544;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L544;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L544;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L544;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, HImode)) goto L544;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, HImode)) goto L544;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L544;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L544;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L544; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L544;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L544;
  if (XVECLEN (x, 0) != 2) goto L544;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L544;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L544;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L544;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != NE) goto L544;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  operands[0] = x;
  if (! register_operand (x, SImode)) goto L544;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L544;
  if (XWINT (x, 0) != 0) goto L544;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L544;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L544;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L544;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L544;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L544;
  if (GET_MODE (x) != SImode) goto L544;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L544;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L544;
  if (XWINT (x, 0) != -1) goto L544;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L544;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 544;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L544:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L545;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L545;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L545;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L545;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, SImode)) goto L545;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, SImode)) goto L545;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L545;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L545;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L545; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L545;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L545;
  if (XVECLEN (x, 0) != 2) goto L545;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L545;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L545;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L545;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != NE) goto L545;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  operands[0] = x;
  if (! register_operand (x, HImode)) goto L545;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L545;
  if (XWINT (x, 0) != 0) goto L545;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L545;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L545;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L545;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L545;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L545;
  if (GET_MODE (x) != HImode) goto L545;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L545;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L545;
  if (XWINT (x, 0) != -1) goto L545;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L545;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 545;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L545:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L546;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L546;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L546;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L546;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, SImode)) goto L546;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, SImode)) goto L546;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L546;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L546;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L546; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L546;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L546;
  if (XVECLEN (x, 0) != 2) goto L546;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L546;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L546;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L546;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != NE) goto L546;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  operands[0] = x;
  if (! register_operand (x, SImode)) goto L546;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L546;
  if (XWINT (x, 0) != 0) goto L546;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L546;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L546;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L546;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L546;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L546;
  if (GET_MODE (x) != SImode) goto L546;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L546;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L546;
  if (XWINT (x, 0) != -1) goto L546;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L546;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 546;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L546:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L547;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L547;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L547;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L547;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, QImode)) goto L547;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, QImode)) goto L547;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L547;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L547;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L547; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L547;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L547;
  if (XVECLEN (x, 0) != 2) goto L547;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L547;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L547;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L547;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != GE) goto L547;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  if (GET_CODE (x) != PLUS) goto L547;
  if (GET_MODE (x) != HImode) goto L547;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0);
  operands[0] = x;
  if (! register_operand (x, HImode)) goto L547;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L547;
  if (XWINT (x, 0) != -1) goto L547;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L547;
  if (XWINT (x, 0) != 0) goto L547;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L547;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L547;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L547;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L547;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L547;
  if (GET_MODE (x) != HImode) goto L547;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L547;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L547;
  if (XWINT (x, 0) != -1) goto L547;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L547;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 547;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L547:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L548;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L548;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L548;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L548;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, QImode)) goto L548;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, QImode)) goto L548;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L548;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L548;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L548; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L548;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L548;
  if (XVECLEN (x, 0) != 2) goto L548;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L548;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L548;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L548;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != GE) goto L548;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  if (GET_CODE (x) != PLUS) goto L548;
  if (GET_MODE (x) != SImode) goto L548;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0);
  operands[0] = x;
  if (! register_operand (x, SImode)) goto L548;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L548;
  if (XWINT (x, 0) != -1) goto L548;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L548;
  if (XWINT (x, 0) != 0) goto L548;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L548;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L548;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L548;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L548;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L548;
  if (GET_MODE (x) != SImode) goto L548;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L548;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L548;
  if (XWINT (x, 0) != -1) goto L548;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L548;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 548;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L548:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L549;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L549;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L549;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L549;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, HImode)) goto L549;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, HImode)) goto L549;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L549;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L549;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L549; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L549;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L549;
  if (XVECLEN (x, 0) != 2) goto L549;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L549;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L549;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L549;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != GE) goto L549;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  if (GET_CODE (x) != PLUS) goto L549;
  if (GET_MODE (x) != HImode) goto L549;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0);
  operands[0] = x;
  if (! register_operand (x, HImode)) goto L549;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L549;
  if (XWINT (x, 0) != -1) goto L549;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L549;
  if (XWINT (x, 0) != 0) goto L549;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L549;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L549;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L549;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L549;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L549;
  if (GET_MODE (x) != HImode) goto L549;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L549;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L549;
  if (XWINT (x, 0) != -1) goto L549;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L549;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 549;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L549:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L550;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L550;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L550;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L550;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, HImode)) goto L550;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, HImode)) goto L550;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L550;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L550;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L550; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L550;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L550;
  if (XVECLEN (x, 0) != 2) goto L550;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L550;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L550;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L550;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != GE) goto L550;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  if (GET_CODE (x) != PLUS) goto L550;
  if (GET_MODE (x) != SImode) goto L550;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0);
  operands[0] = x;
  if (! register_operand (x, SImode)) goto L550;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L550;
  if (XWINT (x, 0) != -1) goto L550;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L550;
  if (XWINT (x, 0) != 0) goto L550;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L550;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L550;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L550;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L550;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L550;
  if (GET_MODE (x) != SImode) goto L550;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L550;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L550;
  if (XWINT (x, 0) != -1) goto L550;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L550;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 550;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L550:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L551;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L551;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L551;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L551;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, SImode)) goto L551;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, SImode)) goto L551;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L551;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L551;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L551; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L551;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L551;
  if (XVECLEN (x, 0) != 2) goto L551;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L551;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L551;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L551;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != GE) goto L551;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  if (GET_CODE (x) != PLUS) goto L551;
  if (GET_MODE (x) != HImode) goto L551;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0);
  operands[0] = x;
  if (! register_operand (x, HImode)) goto L551;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L551;
  if (XWINT (x, 0) != -1) goto L551;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L551;
  if (XWINT (x, 0) != 0) goto L551;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L551;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L551;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L551;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L551;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L551;
  if (GET_MODE (x) != HImode) goto L551;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L551;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L551;
  if (XWINT (x, 0) != -1) goto L551;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L551;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 551;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L551:

  insn = ins1;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != SET) goto L552;
  x = XEXP (pat, 0);
  if (GET_CODE (x) != PC) goto L552;
  x = XEXP (pat, 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L552;
  x = XEXP (XEXP (pat, 1), 0);
  operands[3] = x;
  if (! ordered_comparison_operator (x, VOIDmode)) goto L552;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 0);
  operands[4] = x;
  if (! general_operand (x, SImode)) goto L552;
  x = XEXP (XEXP (XEXP (pat, 1), 0), 1);
  operands[5] = x;
  if (! general_operand (x, SImode)) goto L552;
  x = XEXP (XEXP (pat, 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L552;
  x = XEXP (XEXP (XEXP (pat, 1), 1), 0);
  operands[2] = x;
  x = XEXP (XEXP (pat, 1), 2);
  if (GET_CODE (x) != PC) goto L552;
  do { insn = NEXT_INSN (insn);
       if (insn == 0) goto L552; }
  while (NOTE_P (insn)
	 || (NONJUMP_INSN_P (insn)
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)));
  if (LABEL_P (insn)
      || BARRIER_P (insn))
    goto L552;
  pat = PATTERN (insn);
  x = pat;
  if (GET_CODE (x) != PARALLEL) goto L552;
  if (XVECLEN (x, 0) != 2) goto L552;
  x = XVECEXP (pat, 0, 0);
  if (GET_CODE (x) != SET) goto L552;
  x = XEXP (XVECEXP (pat, 0, 0), 0);
  if (GET_CODE (x) != PC) goto L552;
  x = XEXP (XVECEXP (pat, 0, 0), 1);
  if (GET_CODE (x) != IF_THEN_ELSE) goto L552;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
  if (GET_CODE (x) != GE) goto L552;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
  if (GET_CODE (x) != PLUS) goto L552;
  if (GET_MODE (x) != SImode) goto L552;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0);
  operands[0] = x;
  if (! register_operand (x, SImode)) goto L552;
  x = XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L552;
  if (XWINT (x, 0) != -1) goto L552;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1);
  if (GET_CODE (x) != CONST_INT) goto L552;
  if (XWINT (x, 0) != 0) goto L552;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
  if (GET_CODE (x) != LABEL_REF) goto L552;
  x = XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
  operands[1] = x;
  x = XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2);
  if (GET_CODE (x) != PC) goto L552;
  x = XVECEXP (pat, 0, 1);
  if (GET_CODE (x) != SET) goto L552;
  x = XEXP (XVECEXP (pat, 0, 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L552;
  x = XEXP (XVECEXP (pat, 0, 1), 1);
  if (GET_CODE (x) != PLUS) goto L552;
  if (GET_MODE (x) != SImode) goto L552;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
  if (!rtx_equal_p (operands[0], x)) goto L552;
  x = XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
  if (GET_CODE (x) != CONST_INT) goto L552;
  if (XWINT (x, 0) != -1) goto L552;
  if (! (!TARGET_COLDFIRE && DATA_REG_P (operands[0]))) goto L552;
  PATTERN (ins1) = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (6, operands));
  INSN_CODE (ins1) = 552;
  delete_for_peephole (NEXT_INSN (ins1), insn);
  return NEXT_INSN (insn);
 L552:

  return 0;
}

rtx peep_operand[6];
