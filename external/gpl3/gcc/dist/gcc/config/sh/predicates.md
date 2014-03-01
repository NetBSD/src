;; Predicate definitions for Renesas / SuperH SH.
;; Copyright (C) 2005-2013 Free Software Foundation, Inc.
;;
;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.
;;
;; GCC is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

;; TODO: Add a comment here.
(define_predicate "trapping_target_operand"
  (match_code "if_then_else")
{
  rtx cond, mem, res, tar, and_expr;

  if (GET_MODE (op) != PDImode)
    return 0;
  cond = XEXP (op, 0);
  mem = XEXP (op, 1);
  res = XEXP (op, 2);
  if (!MEM_P (mem)
      || (GET_CODE (res) != SIGN_EXTEND && GET_CODE (res) != TRUNCATE))
    return 0;
  tar = XEXP (res, 0);
  if (!rtx_equal_p (XEXP (mem, 0), tar)
      || GET_MODE (tar) != Pmode)
    return 0;
  if (GET_CODE (cond) == CONST)
    {
      cond = XEXP (cond, 0);
      if (!satisfies_constraint_Csy (tar))
	return 0;
      if (GET_CODE (tar) == CONST)
	tar = XEXP (tar, 0);
    }
  else if (!arith_reg_operand (tar, VOIDmode)
	   && ! satisfies_constraint_Csy (tar))
    return 0;
  if (GET_CODE (cond) != EQ)
    return 0;
  and_expr = XEXP (cond, 0);
  return (GET_CODE (and_expr) == AND
	  && rtx_equal_p (XEXP (and_expr, 0), tar)
	  && CONST_INT_P (XEXP (and_expr, 1))
	  && CONST_INT_P (XEXP (cond, 1))
	  && INTVAL (XEXP (and_expr, 1)) == 3
	  && INTVAL (XEXP (cond, 1)) == 3);
})

;; A logical operand that can be used in an shmedia and insn.
(define_predicate "and_operand"
  (match_code "subreg,reg,const_int")
{
  if (logical_operand (op, mode))
    return 1;

  /* Check mshflo.l / mshflhi.l opportunities.  */
  if (TARGET_SHMEDIA
      && mode == DImode
      && satisfies_constraint_J16 (op))
    return 1;

  return 0;
})

;; Like arith_reg_dest, but this predicate is defined with
;; define_special_predicate, not define_predicate.
(define_special_predicate "any_arith_reg_dest"
  (match_code "subreg,reg")
{
  return arith_reg_dest (op, mode);
})

;; Like register_operand, but this predicate is defined with
;; define_special_predicate, not define_predicate.
(define_special_predicate "any_register_operand"
  (match_code "subreg,reg")
{
  return register_operand (op, mode);
})

;; Returns 1 if OP is a valid source operand for an arithmetic insn.
(define_predicate "arith_operand"
  (match_code "subreg,reg,const_int,truncate")
{
  if (arith_reg_operand (op, mode))
    return 1;

  if (TARGET_SHMEDIA)
    {
      /* FIXME: We should be checking whether the CONST_INT fits in a
	 signed 16-bit here, but this causes reload_cse to crash when
	 attempting to transform a sequence of two 64-bit sets of the
	 same register from literal constants into a set and an add,
	 when the difference is too wide for an add.  */
      if (CONST_INT_P (op)
	  || satisfies_constraint_Css (op))
	return 1;
      else if (GET_CODE (op) == TRUNCATE
	       && REG_P (XEXP (op, 0))
	       && ! system_reg_operand (XEXP (op, 0), VOIDmode)
	       && (mode == VOIDmode || mode == GET_MODE (op))
	       && (GET_MODE_SIZE (GET_MODE (op))
		   < GET_MODE_SIZE (GET_MODE (XEXP (op, 0))))
	       && (! FP_REGISTER_P (REGNO (XEXP (op, 0)))
		   || GET_MODE_SIZE (GET_MODE (op)) == 4))
	return register_operand (XEXP (op, 0), VOIDmode);
      else
	return 0;
    }
  else if (satisfies_constraint_I08 (op))
    return 1;

  return 0;
})

;; Like above, but for DImode destinations: forbid paradoxical DImode
;; subregs, because this would lead to missing sign extensions when
;; truncating from DImode to SImode.
(define_predicate "arith_reg_dest"
  (match_code "subreg,reg")
{
  if (mode == DImode && GET_CODE (op) == SUBREG
      && GET_MODE_SIZE (GET_MODE (SUBREG_REG (op))) < 8
      && TARGET_SHMEDIA)
    return 0;
  return arith_reg_operand (op, mode);
})

;; Returns 1 if OP is a normal arithmetic register.
(define_predicate "arith_reg_operand"
  (match_code "subreg,reg,sign_extend")
{
  if (register_operand (op, mode))
    {
      int regno;

      if (REG_P (op))
	regno = REGNO (op);
      else if (GET_CODE (op) == SUBREG && REG_P (SUBREG_REG (op)))
	regno = REGNO (SUBREG_REG (op));
      else
	return 1;

      return (regno != T_REG && regno != PR_REG
	      && ! TARGET_REGISTER_P (regno)
	      && (regno != FPUL_REG || TARGET_SH4)
	      && regno != MACH_REG && regno != MACL_REG);
    }
  /* Allow a no-op sign extension - compare LOAD_EXTEND_OP.
     We allow SImode here, as not using an FP register is just a matter of
     proper register allocation.  */
  if (TARGET_SHMEDIA
      && GET_MODE (op) == DImode && GET_CODE (op) == SIGN_EXTEND
      && GET_MODE (XEXP (op, 0)) == SImode
      && GET_CODE (XEXP (op, 0)) != SUBREG)
    return register_operand (XEXP (op, 0), VOIDmode);
#if 0 /* Can't do this because of PROMOTE_MODE for unsigned vars.  */
  if (GET_MODE (op) == SImode && GET_CODE (op) == SIGN_EXTEND
      && GET_MODE (XEXP (op, 0)) == HImode
      && REG_P (XEXP (op, 0))
      && REGNO (XEXP (op, 0)) <= LAST_GENERAL_REG)
    return register_operand (XEXP (op, 0), VOIDmode);
#endif
  if (GET_MODE_CLASS (GET_MODE (op)) == MODE_VECTOR_INT
      && GET_CODE (op) == SUBREG
      && GET_MODE (SUBREG_REG (op)) == DImode
      && GET_CODE (SUBREG_REG (op)) == SIGN_EXTEND
      && GET_MODE (XEXP (SUBREG_REG (op), 0)) == SImode
      && GET_CODE (XEXP (SUBREG_REG (op), 0)) != SUBREG)
    return register_operand (XEXP (SUBREG_REG (op), 0), VOIDmode);
  return 0;
})

;; Returns 1 if OP is a valid source operand for a compare insn.
(define_predicate "arith_reg_or_0_operand"
  (match_code "subreg,reg,const_int,const_vector")
{
  if (arith_reg_operand (op, mode))
    return 1;

  if (satisfies_constraint_Z (op))
    return 1;

  return 0;
})

;; Returns 1 if OP is a floating point operator with two operands.
(define_predicate "binary_float_operator"
  (and (match_code "plus,minus,mult,div")
       (match_test "GET_MODE (op) == mode")))

;; Returns 1 if OP is a logical operator with two operands.
(define_predicate "binary_logical_operator"
  (and (match_code "and,ior,xor")
       (match_test "GET_MODE (op) == mode")))

;; Return 1 if OP is an address suitable for a cache manipulation operation.
;; MODE has the meaning as in address_operand.
(define_special_predicate "cache_address_operand"
  (match_code "plus,reg")
{
  if (GET_CODE (op) == PLUS)
    {
      if (!REG_P (XEXP (op, 0)))
	return 0;
      if (!CONST_INT_P (XEXP (op, 1))
	  || (INTVAL (XEXP (op, 1)) & 31))
	return 0;
    }
  else if (!REG_P (op))
    return 0;
  return address_operand (op, mode);
})

;; Returns 1 if OP is a valid source operand for shmedia cmpgt / cmpgtu.
(define_predicate "cmp_operand"
  (match_code "subreg,reg,const_int")
{
  if (satisfies_constraint_N (op))
    return 1;
  if (TARGET_SHMEDIA
      && mode != DImode && GET_CODE (op) == SUBREG
      && GET_MODE_SIZE (GET_MODE (SUBREG_REG (op))) > 4)
    return 0;
  return arith_reg_operand (op, mode);
})

;; Returns true if OP is an operand that can be used as the first operand in
;; the cstoresi4 expander pattern.
(define_predicate "cmpsi_operand"
  (match_code "subreg,reg,const_int")
{
  if (REG_P (op) && REGNO (op) == T_REG
      && GET_MODE (op) == SImode
      && TARGET_SH1)
    return 1;
  return arith_operand (op, mode);
})

;; Returns true if OP is a comutative float operator.
;; This predicate is currently unused.
;;(define_predicate "commutative_float_operator"
;;  (and (match_code "plus,mult")
;;       (match_test "GET_MODE (op) == mode")))

;; Returns true if OP is a equal or not equal operator.
(define_predicate "equality_comparison_operator"
  (match_code "eq,ne"))

;; Returns true if OP is an arithmetic operand that is zero extended during
;; an operation.
(define_predicate "extend_reg_operand"
  (match_code "subreg,reg,truncate")
{
  return (GET_CODE (op) == TRUNCATE
	  ? arith_operand
	  : arith_reg_operand) (op, mode);
})

;; Like extend_reg_operand, but also allow a constant 0.
(define_predicate "extend_reg_or_0_operand"
  (match_code "subreg,reg,truncate,const_int")
{
  return (GET_CODE (op) == TRUNCATE
	  ? arith_operand
	  : arith_reg_or_0_operand) (op, mode);
})

;; Like arith_reg_operand, but this predicate does not accept SIGN_EXTEND.
(define_predicate "ext_dest_operand"
  (match_code "subreg,reg")
{
  return arith_reg_operand (op, mode);
})

;; Returns true if OP can be used as a destination register for shmedia floating
;; point to integer conversions.
(define_predicate "fp_arith_reg_dest"
  (match_code "subreg,reg")
{
  if (mode == DImode && GET_CODE (op) == SUBREG
      && GET_MODE_SIZE (GET_MODE (SUBREG_REG (op))) < 8)
    return 0;
  return fp_arith_reg_operand (op, mode);
})

;; Returns true if OP is a floating point register that can be used in floating
;; point arithmetic operations.
(define_predicate "fp_arith_reg_operand"
  (match_code "subreg,reg")
{
  if (register_operand (op, mode))
    {
      int regno;

      if (REG_P (op))
	regno = REGNO (op);
      else if (GET_CODE (op) == SUBREG && REG_P (SUBREG_REG (op)))
	regno = REGNO (SUBREG_REG (op));
      else
	return 1;

      return (regno >= FIRST_PSEUDO_REGISTER
	      || FP_REGISTER_P (regno));
    }
  return 0;
})

;; Returns true if OP is the FPSCR.
(define_predicate "fpscr_operand"
  (match_code "reg")
{
  return (REG_P (op)
	  && (REGNO (op) == FPSCR_REG
	      || (REGNO (op) >= FIRST_PSEUDO_REGISTER
		  && !(reload_in_progress || reload_completed)))
	  && GET_MODE (op) == PSImode);
})

;; Returns true if OP is an operand that is either the fpul hard reg or
;; a pseudo.  This prevents combine from propagating function arguments
;; in hard regs into insns that need the operand in fpul.  If it's a pseudo
;; reload can fix it up.
(define_predicate "fpul_operand"
  (match_code "reg")
{
  if (TARGET_SHMEDIA)
    return fp_arith_reg_operand (op, mode);

  return (REG_P (op)
	  && (REGNO (op) == FPUL_REG || REGNO (op) >= FIRST_PSEUDO_REGISTER)
	  && GET_MODE (op) == mode);
})

;; Returns true if OP is a valid fpul input operand for the fsca insn.
;; The value in fpul is a fixed-point value and its scaling is described
;; in the fsca insn by a mult:SF.  To allow pre-scaled fixed-point inputs
;; in fpul we have to permit things like
;;   (reg:SI)
;;   (fix:SF (float:SF (reg:SI)))
(define_predicate "fpul_fsca_operand"
  (match_code "fix,reg")
{
  if (fpul_operand (op, SImode))
    return true;
  if (GET_CODE (op) == FIX && GET_MODE (op) == SImode
      && GET_CODE (XEXP (op, 0)) == FLOAT && GET_MODE (XEXP (op, 0)) == SFmode)
    return fpul_fsca_operand (XEXP (XEXP (op, 0), 0),
			      GET_MODE (XEXP (XEXP (op, 0), 0)));
  return false;
})

;; Returns true if OP is a valid constant scale factor for the fsca insn.
(define_predicate "fsca_scale_factor"
  (and (match_code "const_double")
       (match_test "op == sh_fsca_int2sf ()")))

;; Returns true if OP is an operand that is zero extended during an operation.
(define_predicate "general_extend_operand"
  (match_code "subreg,reg,mem,truncate")
{
  return (GET_CODE (op) == TRUNCATE
	  ? arith_operand
	  : nonimmediate_operand) (op, mode);
})

;; Returns 1 if OP is a simple register address.
(define_predicate "simple_mem_operand"
  (and (match_code "mem")
       (match_test "arith_reg_operand (XEXP (op, 0), SImode)")))

;; Returns 1 if OP is a valid displacement address.
(define_predicate "displacement_mem_operand"
  (and (match_code "mem")
       (match_test "GET_CODE (XEXP (op, 0)) == PLUS")
       (match_test "arith_reg_operand (XEXP (XEXP (op, 0), 0), SImode)")
       (match_test "sh_legitimate_index_p (GET_MODE (op),
					   XEXP (XEXP (op, 0), 1),
					   TARGET_SH2A, true)")))

;; Returns true if OP is a displacement address that can fit into a
;; 16 bit (non-SH2A) memory load / store insn.
(define_predicate "short_displacement_mem_operand"
  (match_test "sh_disp_addr_displacement (op)
	       <= sh_max_mov_insn_displacement (GET_MODE (op), false)"))

;; Returns 1 if the operand can be used in an SH2A movu.{b|w} insn.
(define_predicate "zero_extend_movu_operand"
  (and (match_operand 0 "displacement_mem_operand")
       (match_test "GET_MODE (op) == QImode || GET_MODE (op) == HImode")))

;; Returns 1 if the operand can be used in a zero_extend.
(define_predicate "zero_extend_operand"
  (ior (and (match_test "TARGET_SHMEDIA")
	    (match_operand 0 "general_extend_operand"))
       (and (match_test "! TARGET_SHMEDIA")
	    (match_operand 0 "arith_reg_operand"))
       (and (match_test "TARGET_SH2A")
	    (match_operand 0 "zero_extend_movu_operand"))))

;; Returns 1 if OP can be source of a simple move operation. Same as
;; general_operand, but a LABEL_REF is valid, PRE_DEC is invalid as
;; are subregs of system registers.
(define_predicate "general_movsrc_operand"
  (match_code "subreg,reg,const_int,const_double,mem,symbol_ref,label_ref,
	       const,const_vector")
{
  if (t_reg_operand (op, mode))
    return 0;

  /* Disallow PC relative QImode loads, since these is no insn to do that
     and an imm8 load should be used instead.  */
  if (IS_PC_RELATIVE_LOAD_ADDR_P (op) && GET_MODE (op) == QImode)
    return false;

  if (MEM_P (op))
    {
      rtx inside = XEXP (op, 0);

      /* Disallow mems with GBR address here.  They have to go through
	 separate special patterns.  */
      if ((REG_P (inside) && REGNO (inside) == GBR_REG)
	  || (GET_CODE (inside) == PLUS && REG_P (XEXP (inside, 0))
	      && REGNO (XEXP (inside, 0)) == GBR_REG))
	return 0;

      if (GET_CODE (inside) == CONST)
	inside = XEXP (inside, 0);

      if (GET_CODE (inside) == LABEL_REF)
	return 1;

      if (GET_CODE (inside) == PLUS
	  && GET_CODE (XEXP (inside, 0)) == LABEL_REF
	  && CONST_INT_P (XEXP (inside, 1)))
	return 1;

      /* Only post inc allowed.  */
      if (GET_CODE (inside) == PRE_DEC)
	return 0;
    }

  if ((mode == QImode || mode == HImode)
      && mode == GET_MODE (op)
      && (MEM_P (op)
	  || (GET_CODE (op) == SUBREG && MEM_P (SUBREG_REG (op)))))
    {
      rtx x = XEXP ((MEM_P (op) ? op : SUBREG_REG (op)), 0);

      if (GET_CODE (x) == PLUS
	  && REG_P (XEXP (x, 0))
	  && CONST_INT_P (XEXP (x, 1)))
	return sh_legitimate_index_p (mode, XEXP (x, 1), TARGET_SH2A, false);
    }

  if (TARGET_SHMEDIA
      && (GET_CODE (op) == PARALLEL || GET_CODE (op) == CONST_VECTOR)
      && sh_rep_vec (op, mode))
    return 1;
  if (TARGET_SHMEDIA && 1
      && GET_CODE (op) == SUBREG && GET_MODE (op) == mode
      && SUBREG_REG (op) == const0_rtx && subreg_lowpart_p (op))
    /* FIXME */ abort (); /* return 1; */
  return general_operand (op, mode);
})

;; Returns 1 if OP is a MEM that does not use displacement addressing.
(define_predicate "movsrc_no_disp_mem_operand"
  (match_code "mem")
{
  return general_movsrc_operand (op, mode) && satisfies_constraint_Snd (op);
})

;; Returns 1 if OP can be a destination of a move. Same as
;; general_operand, but no preinc allowed.
(define_predicate "general_movdst_operand"
  (match_code "subreg,reg,mem")
{
  if (t_reg_operand (op, mode))
    return 0;

  if (MEM_P (op))
    {
      rtx inside = XEXP (op, 0);
      /* Disallow mems with GBR address here.  They have to go through
	 separate special patterns.  */
      if ((REG_P (inside) && REGNO (inside) == GBR_REG)
	  || (GET_CODE (inside) == PLUS && REG_P (XEXP (inside, 0))
	      && REGNO (XEXP (inside, 0)) == GBR_REG))
	return 0;
    }

  /* Only pre dec allowed.  */
  if (MEM_P (op) && GET_CODE (XEXP (op, 0)) == POST_INC)
    return 0;
  if (mode == DImode && TARGET_SHMEDIA && GET_CODE (op) == SUBREG
      && GET_MODE_SIZE (GET_MODE (SUBREG_REG (op))) < 8
      && ! (reload_in_progress || reload_completed))
    return 0;

  if ((mode == QImode || mode == HImode)
      && mode == GET_MODE (op)
      && (MEM_P (op)
	  || (GET_CODE (op) == SUBREG && MEM_P (SUBREG_REG (op)))))
    {
      rtx x = XEXP ((MEM_P (op) ? op : SUBREG_REG (op)), 0);

      if (GET_CODE (x) == PLUS
	  && REG_P (XEXP (x, 0))
	  && CONST_INT_P (XEXP (x, 1)))
	return sh_legitimate_index_p (mode, XEXP (x, 1), TARGET_SH2A, false);
    }

  return general_operand (op, mode);
})

;; Returns 1 if OP is a POST_INC on stack pointer register.
(define_predicate "sh_no_delay_pop_operand"
  (match_code "mem")
{
  rtx inside;
  inside = XEXP (op, 0);

  if (GET_CODE (op) == MEM && GET_MODE (op) == SImode 
      && GET_CODE (inside) == POST_INC 
      && GET_CODE (XEXP (inside, 0)) == REG
      && REGNO (XEXP (inside, 0)) == SP_REG)
    return 1;

  return 0;
})

;; Returns 1 if OP is a MEM that can be source of a simple move operation.
(define_predicate "unaligned_load_operand"
  (match_code "mem")
{
  rtx inside;

  if (!MEM_P (op) || GET_MODE (op) != mode)
    return 0;

  inside = XEXP (op, 0);

  if (GET_CODE (inside) == POST_INC)
    inside = XEXP (inside, 0);

  if (REG_P (inside))
    return 1;

  return 0;
})

;; Returns 1 if OP is a MEM that can be used in "index_disp" combiner
;; patterns.
(define_predicate "mem_index_disp_operand"
  (match_code "mem")
{
  rtx plus0_rtx, plus1_rtx, mult_rtx;

  plus0_rtx = XEXP (op, 0);
  if (GET_CODE (plus0_rtx) != PLUS)
    return 0;

  plus1_rtx = XEXP (plus0_rtx, 0);
  if (GET_CODE (plus1_rtx) != PLUS)
    return 0;
  if (! arith_reg_operand (XEXP (plus1_rtx, 1), GET_MODE (XEXP (plus1_rtx, 1))))
    return 0;

  mult_rtx = XEXP (plus1_rtx, 0);
  if (GET_CODE (mult_rtx) != MULT)
    return 0;
  if (! arith_reg_operand (XEXP (mult_rtx, 0), GET_MODE (XEXP (mult_rtx, 0)))
      || ! CONST_INT_P (XEXP (mult_rtx, 1)))
    return 0;

  return exact_log2 (INTVAL (XEXP (mult_rtx, 1))) > 0
	 && sh_legitimate_index_p (mode, XEXP (plus0_rtx, 1), TARGET_SH2A, true);
})

;; Returns true if OP is some kind of greater comparision.
(define_predicate "greater_comparison_operator"
  (match_code "gt,ge,gtu,geu"))

;; Returns true if OP is an operand suitable for shmedia reload_inqi and
;; reload_inhi insns.
(define_predicate "inqhi_operand"
  (match_code "truncate")
{
  if (GET_CODE (op) != TRUNCATE || mode != GET_MODE (op))
    return 0;
  op = XEXP (op, 0);
  /* Can't use true_regnum here because copy_cost wants to know about
     SECONDARY_INPUT_RELOAD_CLASS.  */
  return REG_P (op) && FP_REGISTER_P (REGNO (op));
})

;; Returns true if OP is a general purpose integer register.
;; This predicate is currently unused.
;;(define_special_predicate "int_gpr_dest"
;;  (match_code "subreg,reg")
;;{
;;  enum machine_mode op_mode = GET_MODE (op);
;;
;;  if (GET_MODE_CLASS (op_mode) != MODE_INT
;;      || GET_MODE_SIZE (op_mode) >= UNITS_PER_WORD)
;;    return 0;
;;  if (! reload_completed)
;;    return 0;
;;  return true_regnum (op) <= LAST_GENERAL_REG;
;;})

;; Returns true if OP is some kind of less comparison.
(define_predicate "less_comparison_operator"
  (match_code "lt,le,ltu,leu"))

;; Returns 1 if OP is a valid source operand for a logical operation.
(define_predicate "logical_operand"
  (match_code "subreg,reg,const_int")
{
  if (TARGET_SHMEDIA
      && mode != DImode && GET_CODE (op) == SUBREG
      && GET_MODE_SIZE (GET_MODE (SUBREG_REG (op))) > 4)
    return 0;

  if (arith_reg_operand (op, mode))
    return 1;

  if (TARGET_SHMEDIA)
    {
      if (satisfies_constraint_I10 (op))
	return 1;
      else
	return 0;
    }
  else if (satisfies_constraint_K08 (op))
    return 1;

  return 0;
})

;; Like logical_operand but allows additional constant values which can be
;; done with zero extensions.  Used for the second operand of and insns.
(define_predicate "logical_and_operand"
  (match_code "subreg,reg,const_int")
{
  if (logical_operand (op, mode))
    return 1;

  if (! TARGET_SHMEDIA
      && (satisfies_constraint_Jmb (op) || satisfies_constraint_Jmw (op)))
    return 1;

  return 0;
})

;; Returns true if OP is a logical operator.
(define_predicate "logical_operator"
  (match_code "and,ior,xor"))

;; Like arith_reg_operand, but for register source operands of narrow
;; logical SHMEDIA operations: forbid subregs of DImode / TImode regs.
(define_predicate "logical_reg_operand"
  (match_code "subreg,reg")
{
  if (TARGET_SHMEDIA
      && GET_CODE (op) == SUBREG
      && GET_MODE_SIZE (GET_MODE (SUBREG_REG (op))) > 4
      && mode != DImode)
    return 0;
  return arith_reg_operand (op, mode);
})

;; Returns true if OP is a valid bit offset value for the shmedia mextr insns.
(define_predicate "mextr_bit_offset"
  (match_code "const_int")
{
  HOST_WIDE_INT i;

  if (!CONST_INT_P (op))
    return 0;
  i = INTVAL (op);
  return i >= 1 * 8 && i <= 7 * 8 && (i & 7) == 0;
})

;; Returns true if OP is a constant -1, 0 or an zero extended register that
;; can be used as an operator in the *subsi3_media insn.
(define_predicate "minuend_operand"
  (match_code "subreg,reg,truncate,const_int")
{
  return op == constm1_rtx || extend_reg_or_0_operand (op, mode);
})

;; Returns true if OP is a noncommutative floating point operator.
;; This predicate is currently unused.
;;(define_predicate "noncommutative_float_operator"
;;  (and (match_code "minus,div")
;;       (match_test "GET_MODE (op) == mode")))

;; UNORDERED is only supported on SHMEDIA.

(define_predicate "sh_float_comparison_operator"
  (ior (match_operand 0 "ordered_comparison_operator")
       (and (match_test "TARGET_SHMEDIA")
	    (match_code "unordered"))))

(define_predicate "shmedia_cbranch_comparison_operator"
  (ior (match_operand 0 "equality_comparison_operator")
       (match_operand 0 "greater_comparison_operator")))

;; Returns true if OP is a constant vector.
(define_predicate "sh_const_vec"
  (match_code "const_vector")
{
  int i;

  if (GET_CODE (op) != CONST_VECTOR
      || (GET_MODE (op) != mode && mode != VOIDmode))
    return 0;
  i = XVECLEN (op, 0) - 1;
  for (; i >= 0; i--)
    if (!CONST_INT_P (XVECEXP (op, 0, i)))
      return 0;
  return 1;
})

;; Determine if OP is a constant vector matching MODE with only one
;; element that is not a sign extension.  Two byte-sized elements
;; count as one.
(define_predicate "sh_1el_vec"
  (match_code "const_vector")
{
  int unit_size;
  int i, last, least, sign_ix;
  rtx sign;

  if (GET_CODE (op) != CONST_VECTOR
      || (GET_MODE (op) != mode && mode != VOIDmode))
    return 0;
  /* Determine numbers of last and of least significant elements.  */
  last = XVECLEN (op, 0) - 1;
  least = TARGET_LITTLE_ENDIAN ? 0 : last;
  if (!CONST_INT_P (XVECEXP (op, 0, least)))
    return 0;
  sign_ix = least;
  if (GET_MODE_UNIT_SIZE (mode) == 1)
    sign_ix = TARGET_LITTLE_ENDIAN ? 1 : last - 1;
  if (!CONST_INT_P (XVECEXP (op, 0, sign_ix)))
    return 0;
  unit_size = GET_MODE_UNIT_SIZE (GET_MODE (op));
  sign = (INTVAL (XVECEXP (op, 0, sign_ix)) >> (unit_size * BITS_PER_UNIT - 1)
	  ? constm1_rtx : const0_rtx);
  i = XVECLEN (op, 0) - 1;
  do
    if (i != least && i != sign_ix && XVECEXP (op, 0, i) != sign)
      return 0;
  while (--i);
  return 1;
})

;; Like register_operand, but take into account that SHMEDIA can use
;; the constant zero like a general register.
(define_predicate "sh_register_operand"
  (match_code "reg,subreg,const_int,const_double")
{
  if (op == CONST0_RTX (mode) && TARGET_SHMEDIA)
    return 1;
  return register_operand (op, mode);
})

;; Returns true if OP is a vector which is composed of one element that is
;; repeated.
(define_predicate "sh_rep_vec"
  (match_code "const_vector,parallel")
{
  int i;
  rtx x, y;

  if ((GET_CODE (op) != CONST_VECTOR && GET_CODE (op) != PARALLEL)
      || (GET_MODE (op) != mode && mode != VOIDmode))
    return 0;
  i = XVECLEN (op, 0) - 2;
  x = XVECEXP (op, 0, i + 1);
  if (GET_MODE_UNIT_SIZE (mode) == 1)
    {
      y = XVECEXP (op, 0, i);
      for (i -= 2; i >= 0; i -= 2)
	if (! rtx_equal_p (XVECEXP (op, 0, i + 1), x)
	    || ! rtx_equal_p (XVECEXP (op, 0, i), y))
	  return 0;
    }
  else
    for (; i >= 0; i--)
      if (XVECEXP (op, 0, i) != x)
	return 0;
  return 1;
})

;; Returns true if OP is a valid shift count operand for shift operations.
(define_predicate "shift_count_operand"
  (match_code "const_int,const_double,const,symbol_ref,label_ref,subreg,reg,
	       zero_extend,sign_extend")
{
  /* Allow T_REG as shift count for dynamic shifts, although it is not
     really possible.  It will then be copied to a general purpose reg.  */
  if (! TARGET_SHMEDIA)
    return const_int_operand (op, mode) || arith_reg_operand (op, mode)
	   || (TARGET_DYNSHIFT && t_reg_operand (op, mode));

  return (CONSTANT_P (op)
	  ? (CONST_INT_P (op)
	     ? (unsigned) INTVAL (op) < GET_MODE_BITSIZE (mode)
	     : nonmemory_operand (op, mode))
	  : shift_count_reg_operand (op, mode));
})

;; Returns true if OP is a valid shift count operand in a register which can
;; be used by shmedia shift insns.
(define_predicate "shift_count_reg_operand"
  (match_code "subreg,reg,zero_extend,sign_extend")
{
  if ((GET_CODE (op) == ZERO_EXTEND || GET_CODE (op) == SIGN_EXTEND
       || (GET_CODE (op) == SUBREG && SUBREG_BYTE (op) == 0))
      && (mode == VOIDmode || mode == GET_MODE (op))
      && GET_MODE_BITSIZE (GET_MODE (XEXP (op, 0))) >= 6
      && GET_MODE_CLASS (GET_MODE (XEXP (op, 0))) == MODE_INT)
    {
      mode = VOIDmode;
      do
	op = XEXP (op, 0);
      while ((GET_CODE (op) == ZERO_EXTEND || GET_CODE (op) == SIGN_EXTEND
	      || GET_CODE (op) == TRUNCATE)
	     && GET_MODE_BITSIZE (GET_MODE (XEXP (op, 0))) >= 6
	     && GET_MODE_CLASS (GET_MODE (XEXP (op, 0))) == MODE_INT);

    }
  return arith_reg_operand (op, mode);
})

;; Predicates for matching operands that are constant shift
;; amounts 1, 2, 8, 16.
(define_predicate "p27_shift_count_operand"
  (and (match_code "const_int")
       (match_test "satisfies_constraint_P27 (op)")))

(define_predicate "not_p27_shift_count_operand"
  (and (match_code "const_int")
       (match_test "! satisfies_constraint_P27 (op)")))

;; For right shifts the constant 1 is a special case because the shlr insn
;; clobbers the T_REG and is handled by the T_REG clobbering version of the
;; insn, which is also used for non-P27 shift sequences.
(define_predicate "p27_rshift_count_operand"
  (and (match_code "const_int")
       (match_test "satisfies_constraint_P27 (op)")
       (match_test "! satisfies_constraint_M (op)")))

(define_predicate "not_p27_rshift_count_operand"
  (and (match_code "const_int")
       (ior (match_test "! satisfies_constraint_P27 (op)")
	    (match_test "satisfies_constraint_M (op)"))))

;; Returns true if OP is some kind of a shift operator.
(define_predicate "shift_operator"
  (match_code "ashift,ashiftrt,lshiftrt"))

;; Returns true if OP is a symbol reference.
(define_predicate "symbol_ref_operand"
  (match_code "symbol_ref"))

;; Same as target_reg_operand, except that label_refs and symbol_refs
;; are accepted before reload.
(define_special_predicate "target_operand"
  (match_code "subreg,reg,label_ref,symbol_ref,const,unspec")
{
  if (mode != VOIDmode && mode != Pmode)
    return 0;

  if ((GET_MODE (op) == Pmode || GET_MODE (op) == VOIDmode)
      && satisfies_constraint_Csy (op))
    return ! reload_completed;

  return target_reg_operand (op, mode);
})

;; A predicate that accepts pseudos and branch target registers.
(define_special_predicate "target_reg_operand"
  (match_code "subreg,reg")
{
  if (mode == VOIDmode
     ? GET_MODE (op) != Pmode && GET_MODE (op) != PDImode
     : mode != GET_MODE (op))
    return 0;

  if (GET_CODE (op) == SUBREG)
    op = XEXP (op, 0);

  if (!REG_P (op))
    return 0;

  /* We must protect ourselves from matching pseudos that are virtual
     register, because they will eventually be replaced with hardware
     registers that aren't branch-target registers.  */
  if (REGNO (op) > LAST_VIRTUAL_REGISTER
      || TARGET_REGISTER_P (REGNO (op)))
    return 1;

  return 0;
})

;; Returns true if OP is a valid operand for the shmedia mperm.w insn.
(define_special_predicate "trunc_hi_operand"
  (match_code "subreg,reg,truncate")
{
  enum machine_mode op_mode = GET_MODE (op);

  if (op_mode != SImode && op_mode != DImode
      && op_mode != V4HImode && op_mode != V2SImode)
    return 0;
  return extend_reg_operand (op, mode);
})

;; Returns true if OP is an address suitable for an unaligned access
;; instruction.
(define_special_predicate "ua_address_operand"
  (match_code "subreg,reg,plus")
{
  if (GET_CODE (op) == PLUS
      && (! satisfies_constraint_I06 (XEXP (op, 1))))
    return 0;
  return address_operand (op, QImode);
})

;; Returns true if OP is a valid offset for an unaligned memory address.
(define_predicate "ua_offset"
  (match_code "const_int")
{
  return satisfies_constraint_I06 (op);
})

;; Returns true if OP is a floating point operator with one operand.
(define_predicate "unary_float_operator"
  (and (match_code "abs,neg,sqrt")
       (match_test "GET_MODE (op) == mode")))

;; Return 1 if OP is a valid source operand for xor.
(define_predicate "xor_operand"
  (match_code "subreg,reg,const_int")
{
  if (CONST_INT_P (op))
    return (TARGET_SHMEDIA
	    ? (satisfies_constraint_I06 (op)
	       || (!can_create_pseudo_p () && INTVAL (op) == 0xff))
	    : satisfies_constraint_K08 (op));
  if (TARGET_SHMEDIA
      && mode != DImode && GET_CODE (op) == SUBREG
      && GET_MODE_SIZE (GET_MODE (SUBREG_REG (op))) > 4)
    return 0;
  return arith_reg_operand (op, mode);
})

(define_predicate "bitwise_memory_operand"
  (match_code "mem")
{
  if (MEM_P (op))
    {
      if (REG_P (XEXP (op, 0)))
	return 1;

      if (GET_CODE (XEXP (op, 0)) == PLUS
	  && REG_P (XEXP (XEXP (op, 0), 0))
	  && satisfies_constraint_K12 (XEXP (XEXP (op, 0), 1)))
        return 1;
    }
  return 0;
})

;; The atomic_* operand predicates are used for the atomic patterns.
;; Depending on the particular pattern some operands can be immediate
;; values.  Using these predicates avoids the usage of 'force_reg' in the
;; expanders.
(define_predicate "atomic_arith_operand"
  (ior (match_code "subreg,reg")
       (and (match_test "satisfies_constraint_I08 (op)")
	    (match_test "mode != QImode")
	    (match_test "mode != HImode")
	    (match_test "TARGET_SH4A_ARCH"))))

(define_predicate "atomic_logical_operand"
  (ior (match_code "subreg,reg")
       (and (match_test "satisfies_constraint_K08 (op)")
	    (match_test "mode != QImode")
	    (match_test "mode != HImode")
	    (match_test "TARGET_SH4A_ARCH"))))

;; A predicate describing the T bit register in any form.
(define_predicate "t_reg_operand"
  (match_code "reg,subreg,sign_extend,zero_extend")
{
  switch (GET_CODE (op))
    {
      case REG:
	return REGNO (op) == T_REG;

      case SUBREG:
	return REG_P (SUBREG_REG (op)) && REGNO (SUBREG_REG (op)) == T_REG;

      case ZERO_EXTEND:
      case SIGN_EXTEND:
	return GET_CODE (XEXP (op, 0)) == SUBREG
	       && REG_P (SUBREG_REG (XEXP (op, 0)))
	       && REGNO (SUBREG_REG (XEXP (op, 0))) == T_REG;

      default:
	return 0;
    }
})

;; A predicate describing a negated T bit register.
(define_predicate "negt_reg_operand"
  (match_code "subreg,xor")
{
  switch (GET_CODE (op))
    {
      case XOR:
	return t_reg_operand (XEXP (op, 0), GET_MODE (XEXP (op, 0)))
	       && satisfies_constraint_M (XEXP (op, 1));

      case SUBREG:
	return negt_reg_operand (XEXP (op, 0), GET_MODE (XEXP (op, 0)));

      default:
	return 0;
    }
})

;; A predicate that returns true if OP is a valid construct around the T bit
;; that can be used as an operand for conditional branches.
(define_predicate "cbranch_treg_value"
  (match_code "eq,ne,reg,subreg,xor,sign_extend,zero_extend")
{
  return sh_eval_treg_value (op) >= 0;
})

;; Returns true if OP is arith_reg_operand or t_reg_operand.
(define_predicate "arith_reg_or_t_reg_operand"
  (ior (match_operand 0 "arith_reg_operand")
       (match_operand 0 "t_reg_operand")))

;; A predicate describing the negated value of the T bit register shifted
;; left by 31.
(define_predicate "negt_reg_shl31_operand"
  (match_code "plus,minus,if_then_else")
{
  /* (plus:SI (mult:SI (match_operand:SI 1 "t_reg_operand")
		       (const_int -2147483648))  ;; 0xffffffff80000000
	      (const_int -2147483648))
  */
  if (GET_CODE (op) == PLUS && satisfies_constraint_Jhb (XEXP (op, 1))
      && GET_CODE (XEXP (op, 0)) == MULT
      && t_reg_operand (XEXP (XEXP (op, 0), 0), SImode)
      && satisfies_constraint_Jhb (XEXP (XEXP (op, 0), 1)))
    return true;

  /* (minus:SI (const_int -2147483648)  ;; 0xffffffff80000000
	       (mult:SI (match_operand:SI 1 "t_reg_operand")
			(const_int -2147483648)))
  */
  if (GET_CODE (op) == MINUS
      && satisfies_constraint_Jhb (XEXP (op, 0))
      && GET_CODE (XEXP (op, 1)) == MULT
      && t_reg_operand (XEXP (XEXP (op, 1), 0), SImode)
      && satisfies_constraint_Jhb (XEXP (XEXP (op, 1), 1)))
    return true;

  /*  (if_then_else:SI (match_operand:SI 1 "t_reg_operand")
		       (const_int 0)
		       (const_int -2147483648))  ;; 0xffffffff80000000
  */
  if (GET_CODE (op) == IF_THEN_ELSE && t_reg_operand (XEXP (op, 0), SImode)
      && satisfies_constraint_Z (XEXP (op, 1))
      && satisfies_constraint_Jhb (XEXP (op, 2)))
    return true;

  return false;
})

;; A predicate that determines whether a given constant is a valid
;; displacement for a GBR load/store of the specified mode.
(define_predicate "gbr_displacement"
  (match_code "const_int")
{
  const int mode_sz = GET_MODE_SIZE (mode);
  const int move_sz = mode_sz > GET_MODE_SIZE (SImode)
				? GET_MODE_SIZE (SImode)
				: mode_sz;
  int max_disp = 255 * move_sz;
  if (mode_sz > move_sz)
    max_disp -= mode_sz - move_sz;

  return INTVAL (op) >= 0 && INTVAL (op) <= max_disp;
})

;; A predicate that determines whether OP is a valid GBR addressing mode
;; memory reference.
(define_predicate "gbr_address_mem"
  (match_code "mem")
{
  rtx addr = XEXP (op, 0);

  if (REG_P (addr) && REGNO (addr) == GBR_REG)
    return true;
  if (GET_CODE (addr) == PLUS
      && REG_P (XEXP (addr, 0)) && REGNO (XEXP (addr, 0)) == GBR_REG
      && gbr_displacement (XEXP (addr, 1), mode))
    return true;

  return false;
})
