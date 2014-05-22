;; Machine description for AArch64 architecture.
;; Copyright (C) 2009-2013 Free Software Foundation, Inc.
;; Contributed by ARM Ltd.
;;
;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.
;;
;; GCC is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

;; Register numbers
(define_constants
  [
    (R0_REGNUM		0)
    (R1_REGNUM		1)
    (R2_REGNUM		2)
    (R3_REGNUM		3)
    (R4_REGNUM		4)
    (R5_REGNUM		5)
    (R6_REGNUM		6)
    (R7_REGNUM		7)
    (R8_REGNUM		8)
    (R9_REGNUM		9)
    (R10_REGNUM		10)
    (R11_REGNUM		11)
    (R12_REGNUM		12)
    (R13_REGNUM		13)
    (R14_REGNUM		14)
    (R15_REGNUM		15)
    (R16_REGNUM		16)
    (IP0_REGNUM		16)
    (R17_REGNUM		17)
    (IP1_REGNUM		17)
    (R18_REGNUM		18)
    (R19_REGNUM		19)
    (R20_REGNUM		20)
    (R21_REGNUM		21)
    (R22_REGNUM		22)
    (R23_REGNUM		23)
    (R24_REGNUM		24)
    (R25_REGNUM		25)
    (R26_REGNUM		26)
    (R27_REGNUM		27)
    (R28_REGNUM		28)
    (R29_REGNUM		29)
    (R30_REGNUM		30)
    (LR_REGNUM		30)
    (SP_REGNUM		31)
    (V0_REGNUM		32)
    (V15_REGNUM		47)
    (V31_REGNUM		63)
    (SFP_REGNUM		64)
    (AP_REGNUM		65)
    (CC_REGNUM		66)
  ]
)

(define_c_enum "unspec" [
    UNSPEC_CASESI
    UNSPEC_CLS
    UNSPEC_FRINTA
    UNSPEC_FRINTI
    UNSPEC_FRINTM
    UNSPEC_FRINTP
    UNSPEC_FRINTX
    UNSPEC_FRINTZ
    UNSPEC_GOTSMALLPIC
    UNSPEC_GOTSMALLTLS
    UNSPEC_LD2
    UNSPEC_LD3
    UNSPEC_LD4
    UNSPEC_MB
    UNSPEC_NOP
    UNSPEC_PRLG_STK
    UNSPEC_RBIT
    UNSPEC_ST2
    UNSPEC_ST3
    UNSPEC_ST4
    UNSPEC_TLS
    UNSPEC_TLSDESC
    UNSPEC_VSTRUCTDUMMY
])

(define_c_enum "unspecv" [
    UNSPECV_EH_RETURN		; Represent EH_RETURN
  ]
)

;; If further include files are added the defintion of MD_INCLUDES
;; must be updated.

(include "constraints.md")
(include "predicates.md")
(include "iterators.md")

;; -------------------------------------------------------------------
;; Instruction types and attributes
;; -------------------------------------------------------------------

;; Main data types used by the insntructions

(define_attr "mode" "unknown,none,QI,HI,SI,DI,TI,SF,DF,TF"
  (const_string "unknown"))

(define_attr "mode2" "unknown,none,QI,HI,SI,DI,TI,SF,DF,TF"
  (const_string "unknown"))

; The "v8type" attribute is used to for fine grained classification of
; AArch64 instructions.  This table briefly explains the meaning of each type.

; adc              add/subtract with carry.
; adcs             add/subtract with carry (setting condition flags).
; adr              calculate address.
; alu              simple alu instruction (no memory or fp regs access).
; alu_ext          simple alu instruction (sign/zero-extended register).
; alu_shift        simple alu instruction, with a source operand shifted by a constant.
; alus             simple alu instruction (setting condition flags).
; alus_ext         simple alu instruction (sign/zero-extended register, setting condition flags).
; alus_shift       simple alu instruction, with a source operand shifted by a constant (setting condition flags).
; bfm              bitfield move operation.
; branch           branch.
; call             subroutine call.
; ccmp             conditional compare.
; clz              count leading zeros/sign bits.
; csel             conditional select.
; dmb              data memory barrier.
; extend           sign/zero-extend (specialised bitfield move).
; extr             extract register-sized bitfield encoding.
; fpsimd_load      load single floating point / simd scalar register from memory.
; fpsimd_load2     load pair of floating point / simd scalar registers from memory.
; fpsimd_store     store single floating point / simd scalar register to memory.
; fpsimd_store2    store pair floating point / simd scalar registers to memory.
; fadd             floating point add/sub.
; fccmp            floating point conditional compare.
; fcmp             floating point comparison.
; fconst           floating point load immediate.
; fcsel            floating point conditional select.
; fcvt             floating point convert (float to float).
; fcvtf2i          floating point convert (float to integer).
; fcvti2f          floating point convert (integer to float).
; fdiv             floating point division operation.
; ffarith          floating point abs, neg or cpy.
; fmadd            floating point multiply-add/sub.
; fminmax          floating point min/max.
; fmov             floating point move (float to float).
; fmovf2i          floating point move (float to integer).
; fmovi2f          floating point move (integer to float).
; fmul             floating point multiply.
; frint            floating point round to integral.
; fsqrt            floating point square root.
; load_acq         load-acquire.
; load             load single general register from memory
; load2            load pair of general registers from memory
; logic            logical operation (register).
; logic_imm        and/or/xor operation (immediate).
; logic_shift      logical operation with shift.
; logics           logical operation (register, setting condition flags).
; logics_imm       and/or/xor operation (immediate, setting condition flags).
; logics_shift     logical operation with shift (setting condition flags).
; madd             integer multiply-add/sub.
; maddl            widening integer multiply-add/sub.
; misc             miscellaneous - any type that doesn't fit into the rest.
; move             integer move operation.
; move2            double integer move operation.
; movk             move 16-bit immediate with keep.
; movz             move 16-bit immmediate with zero/one.
; mrs              system/special register move.
; mulh             64x64 to 128-bit multiply (high part).
; mull             widening multiply.
; mult             integer multiply instruction.
; prefetch         memory prefetch.
; rbit             reverse bits.
; rev              reverse bytes.
; sdiv             integer division operation (signed).
; shift            variable shift operation.
; shift_imm        immediate shift operation (specialised bitfield move).
; store_rel        store-release.
; store            store single general register to memory.
; store2           store pair of general registers to memory.
; udiv             integer division operation (unsigned).

(define_attr "v8type"
   "adc,\
   adcs,\
   adr,\
   alu,\
   alu_ext,\
   alu_shift,\
   alus,\
   alus_ext,\
   alus_shift,\
   bfm,\
   branch,\
   call,\
   ccmp,\
   clz,\
   csel,\
   dmb,\
   div,\
   div64,\
   extend,\
   extr,\
   fpsimd_load,\
   fpsimd_load2,\
   fpsimd_store2,\
   fpsimd_store,\
   fadd,\
   fccmp,\
   fcvt,\
   fcvtf2i,\
   fcvti2f,\
   fcmp,\
   fconst,\
   fcsel,\
   fdiv,\
   ffarith,\
   fmadd,\
   fminmax,\
   fmov,\
   fmovf2i,\
   fmovi2f,\
   fmul,\
   frint,\
   fsqrt,\
   load_acq,\
   load1,\
   load2,\
   logic,\
   logic_imm,\
   logic_shift,\
   logics,\
   logics_imm,\
   logics_shift,\
   madd,\
   maddl,\
   misc,\
   move,\
   move2,\
   movk,\
   movz,\
   mrs,\
   mulh,\
   mull,\
   mult,\
   prefetch,\
   rbit,\
   rev,\
   sdiv,\
   shift,\
   shift_imm,\
   store_rel,\
   store1,\
   store2,\
   udiv"
  (const_string "alu"))


; The "type" attribute is used by the AArch32 backend.  Below is a mapping
; from "v8type" to "type".

(define_attr "type"
  "alu,alu_shift,block,branch,call,f_2_r,f_cvt,f_flag,f_loads,
   f_loadd,f_stored,f_stores,faddd,fadds,fcmpd,fcmps,fconstd,fconsts,
   fcpys,fdivd,fdivs,ffarithd,ffariths,fmacd,fmacs,fmuld,fmuls,load_byte,
   load1,load2,mult,r_2_f,store1,store2"
  (cond [
	  (eq_attr "v8type" "alu_shift,alus_shift,logic_shift,logics_shift") (const_string "alu_shift")
	  (eq_attr "v8type" "branch") (const_string "branch")
	  (eq_attr "v8type" "call") (const_string "call")
	  (eq_attr "v8type" "fmovf2i") (const_string "f_2_r")
	  (eq_attr "v8type" "fcvt,fcvtf2i,fcvti2f") (const_string "f_cvt")
	  (and (eq_attr "v8type" "fpsimd_load") (eq_attr "mode" "SF")) (const_string "f_loads")
	  (and (eq_attr "v8type" "fpsimd_load") (eq_attr "mode" "DF")) (const_string "f_loadd")
	  (and (eq_attr "v8type" "fpsimd_store") (eq_attr "mode" "SF")) (const_string "f_stores")
	  (and (eq_attr "v8type" "fpsimd_store") (eq_attr "mode" "DF")) (const_string "f_stored")
	  (and (eq_attr "v8type" "fadd,fminmax") (eq_attr "mode" "DF")) (const_string "faddd")
	  (and (eq_attr "v8type" "fadd,fminmax") (eq_attr "mode" "SF")) (const_string "fadds")
	  (and (eq_attr "v8type" "fcmp,fccmp") (eq_attr "mode" "DF")) (const_string "fcmpd")
	  (and (eq_attr "v8type" "fcmp,fccmp") (eq_attr "mode" "SF")) (const_string "fcmps")
	  (and (eq_attr "v8type" "fconst") (eq_attr "mode" "DF")) (const_string "fconstd")
	  (and (eq_attr "v8type" "fconst") (eq_attr "mode" "SF")) (const_string "fconsts")
	  (and (eq_attr "v8type" "fdiv,fsqrt") (eq_attr "mode" "DF")) (const_string "fdivd")
	  (and (eq_attr "v8type" "fdiv,fsqrt") (eq_attr "mode" "SF")) (const_string "fdivs")
	  (and (eq_attr "v8type" "ffarith") (eq_attr "mode" "DF")) (const_string "ffarithd")
	  (and (eq_attr "v8type" "ffarith") (eq_attr "mode" "SF")) (const_string "ffariths")
	  (and (eq_attr "v8type" "fmadd") (eq_attr "mode" "DF")) (const_string "fmacd")
	  (and (eq_attr "v8type" "fmadd") (eq_attr "mode" "SF")) (const_string "fmacs")
	  (and (eq_attr "v8type" "fmul") (eq_attr "mode" "DF")) (const_string "fmuld")
	  (and (eq_attr "v8type" "fmul") (eq_attr "mode" "SF")) (const_string "fmuls")
	  (and (eq_attr "v8type" "load1") (eq_attr "mode" "QI,HI")) (const_string "load_byte")
	  (and (eq_attr "v8type" "load1") (eq_attr "mode" "SI,DI,TI")) (const_string "load1")
	  (eq_attr "v8type" "load2") (const_string "load2")
	  (and (eq_attr "v8type" "mulh,mult,mull,madd,sdiv,udiv") (eq_attr "mode" "SI")) (const_string "mult")
	  (eq_attr "v8type" "fmovi2f") (const_string "r_2_f")
	  (eq_attr "v8type" "store1") (const_string "store1")
	  (eq_attr "v8type" "store2") (const_string "store2")
  ]
  (const_string "alu")))

;; Attribute that specifies whether or not the instruction touches fp
;; registers.
(define_attr "fp" "no,yes" (const_string "no"))

;; Attribute that specifies whether or not the instruction touches simd
;; registers.
(define_attr "simd" "no,yes" (const_string "no"))

(define_attr "length" ""
  (const_int 4))

;; Attribute that controls whether an alternative is enabled or not.
;; Currently it is only used to disable alternatives which touch fp or simd
;; registers when -mgeneral-regs-only is specified.
(define_attr "enabled" "no,yes"
  (cond [(ior
	(and (eq_attr "fp" "yes")
	     (eq (symbol_ref "TARGET_FLOAT") (const_int 0)))
	(and (eq_attr "simd" "yes")
	     (eq (symbol_ref "TARGET_SIMD") (const_int 0))))
	     (const_string "no")
	] (const_string "yes")))

;; -------------------------------------------------------------------
;; Pipeline descriptions and scheduling
;; -------------------------------------------------------------------

;; Processor types.
(include "aarch64-tune.md")

;; Scheduling
(include "aarch64-generic.md")
(include "large.md")
(include "small.md")

;; -------------------------------------------------------------------
;; Jumps and other miscellaneous insns
;; -------------------------------------------------------------------

(define_insn "indirect_jump"
  [(set (pc) (match_operand:DI 0 "register_operand" "r"))]
  ""
  "br\\t%0"
  [(set_attr "v8type" "branch")]
)

(define_insn "jump"
  [(set (pc) (label_ref (match_operand 0 "" "")))]
  ""
  "b\\t%l0"
  [(set_attr "v8type" "branch")]
)

(define_expand "cbranch<mode>4"
  [(set (pc) (if_then_else (match_operator 0 "aarch64_comparison_operator"
			    [(match_operand:GPI 1 "register_operand" "")
			     (match_operand:GPI 2 "aarch64_plus_operand" "")])
			   (label_ref (match_operand 3 "" ""))
			   (pc)))]
  ""
  "
  operands[1] = aarch64_gen_compare_reg (GET_CODE (operands[0]), operands[1],
					 operands[2]);
  operands[2] = const0_rtx;
  "
)

(define_expand "cbranch<mode>4"
  [(set (pc) (if_then_else (match_operator 0 "aarch64_comparison_operator"
			    [(match_operand:GPF 1 "register_operand" "")
			     (match_operand:GPF 2 "aarch64_reg_or_zero" "")])
			   (label_ref (match_operand 3 "" ""))
			   (pc)))]
  ""
  "
  operands[1] = aarch64_gen_compare_reg (GET_CODE (operands[0]), operands[1],
					 operands[2]);
  operands[2] = const0_rtx;
  "
)

(define_insn "*condjump"
  [(set (pc) (if_then_else (match_operator 0 "aarch64_comparison_operator"
			    [(match_operand 1 "cc_register" "") (const_int 0)])
			   (label_ref (match_operand 2 "" ""))
			   (pc)))]
  ""
  "b%m0\\t%l2"
  [(set_attr "v8type" "branch")]
)

(define_expand "casesi"
  [(match_operand:SI 0 "register_operand" "")	; Index
   (match_operand:SI 1 "const_int_operand" "")	; Lower bound
   (match_operand:SI 2 "const_int_operand" "")	; Total range
   (match_operand:DI 3 "" "")			; Table label
   (match_operand:DI 4 "" "")]			; Out of range label
  ""
  {
    if (operands[1] != const0_rtx)
      {
	rtx reg = gen_reg_rtx (SImode);

	/* Canonical RTL says that if you have:

	   (minus (X) (CONST))

           then this should be emitted as:

           (plus (X) (-CONST))

	   The use of trunc_int_for_mode ensures that the resulting
	   constant can be represented in SImode, this is important
	   for the corner case where operand[1] is INT_MIN.  */

	operands[1] = GEN_INT (trunc_int_for_mode (-INTVAL (operands[1]), SImode));

	if (!(*insn_data[CODE_FOR_addsi3].operand[2].predicate)
	      (operands[1], SImode))
	  operands[1] = force_reg (SImode, operands[1]);
	emit_insn (gen_addsi3 (reg, operands[0], operands[1]));
	operands[0] = reg;
      }

    if (!aarch64_plus_operand (operands[2], SImode))
      operands[2] = force_reg (SImode, operands[2]);
    emit_jump_insn (gen_cbranchsi4 (gen_rtx_GTU (SImode, const0_rtx,
						 const0_rtx),
				    operands[0], operands[2], operands[4]));

    operands[2] = force_reg (DImode, gen_rtx_LABEL_REF (VOIDmode, operands[3]));
    emit_jump_insn (gen_casesi_dispatch (operands[2], operands[0],
					 operands[3]));
    DONE;
  }
)

(define_insn "casesi_dispatch"
  [(parallel
    [(set (pc)
	  (mem:DI (unspec [(match_operand:DI 0 "register_operand" "r")
			   (match_operand:SI 1 "register_operand" "r")]
			UNSPEC_CASESI)))
     (clobber (reg:CC CC_REGNUM))
     (clobber (match_scratch:DI 3 "=r"))
     (clobber (match_scratch:DI 4 "=r"))
     (use (label_ref (match_operand 2 "" "")))])]
  ""
  "*
  return aarch64_output_casesi (operands);
  "
  [(set_attr "length" "16")
   (set_attr "v8type" "branch")]
)

(define_insn "nop"
  [(unspec[(const_int 0)] UNSPEC_NOP)]
  ""
  "nop"
  [(set_attr "v8type" "misc")]
)

(define_expand "prologue"
  [(clobber (const_int 0))]
  ""
  "
  aarch64_expand_prologue ();
  DONE;
  "
)

(define_expand "epilogue"
  [(clobber (const_int 0))]
  ""
  "
  aarch64_expand_epilogue (false);
  DONE;
  "
)

(define_expand "sibcall_epilogue"
  [(clobber (const_int 0))]
  ""
  "
  aarch64_expand_epilogue (true);
  DONE;
  "
)

(define_insn "*do_return"
  [(return)]
  ""
  "ret"
  [(set_attr "v8type" "branch")]
)

(define_insn "eh_return"
  [(unspec_volatile [(match_operand:DI 0 "register_operand" "r")]
    UNSPECV_EH_RETURN)]
  ""
  "#"
  [(set_attr "v8type" "branch")]
)

(define_split
  [(unspec_volatile [(match_operand:DI 0 "register_operand" "")]
    UNSPECV_EH_RETURN)]
  "reload_completed"
  [(set (match_dup 1) (match_dup 0))]
  {
    operands[1] = aarch64_final_eh_return_addr ();
  }
)

(define_insn "*cb<optab><mode>1"
  [(set (pc) (if_then_else (EQL (match_operand:GPI 0 "register_operand" "r")
				(const_int 0))
			   (label_ref (match_operand 1 "" ""))
			   (pc)))]
  ""
  "<cbz>\\t%<w>0, %l1"
  [(set_attr "v8type" "branch")]
)

(define_insn "*tb<optab><mode>1"
  [(set (pc) (if_then_else
	      (EQL (zero_extract:DI (match_operand:GPI 0 "register_operand" "r")
				    (const_int 1)
				    (match_operand 1 "const_int_operand" "n"))
		   (const_int 0))
	     (label_ref (match_operand 2 "" ""))
	     (pc)))
   (clobber (match_scratch:DI 3 "=r"))]
  ""
  "*
  if (get_attr_length (insn) == 8)
    return \"ubfx\\t%<w>3, %<w>0, %1, #1\;<cbz>\\t%<w>3, %l2\";
  return \"<tbz>\\t%<w>0, %1, %l2\";
  "
  [(set_attr "v8type" "branch")
   (set_attr "mode" "<MODE>")
   (set (attr "length")
	(if_then_else (and (ge (minus (match_dup 2) (pc)) (const_int -32768))
			   (lt (minus (match_dup 2) (pc)) (const_int 32764)))
		      (const_int 4)
		      (const_int 8)))]
)

(define_insn "*cb<optab><mode>1"
  [(set (pc) (if_then_else (LTGE (match_operand:ALLI 0 "register_operand" "r")
				 (const_int 0))
			   (label_ref (match_operand 1 "" ""))
			   (pc)))
   (clobber (match_scratch:DI 2 "=r"))]
  ""
  "*
  if (get_attr_length (insn) == 8)
    return \"ubfx\\t%<w>2, %<w>0, <sizem1>, #1\;<cbz>\\t%<w>2, %l1\";
  return \"<tbz>\\t%<w>0, <sizem1>, %l1\";
  "
  [(set_attr "v8type" "branch")
   (set_attr "mode" "<MODE>")
   (set (attr "length")
	(if_then_else (and (ge (minus (match_dup 1) (pc)) (const_int -32768))
			   (lt (minus (match_dup 1) (pc)) (const_int 32764)))
		      (const_int 4)
		      (const_int 8)))]
)

;; -------------------------------------------------------------------
;; Subroutine calls and sibcalls
;; -------------------------------------------------------------------

(define_expand "call"
  [(parallel [(call (match_operand 0 "memory_operand" "")
		    (match_operand 1 "general_operand" ""))
	      (use (match_operand 2 "" ""))
	      (clobber (reg:DI LR_REGNUM))])]
  ""
  "
  {
    rtx callee;

    /* In an untyped call, we can get NULL for operand 2.  */
    if (operands[2] == NULL)
      operands[2] = const0_rtx;

    /* Decide if we should generate indirect calls by loading the
       64-bit address of the callee into a register before performing
       the branch-and-link.  */
    callee = XEXP (operands[0], 0);
    if (GET_CODE (callee) == SYMBOL_REF
	? aarch64_is_long_call_p (callee)
	: !REG_P (callee))
      XEXP (operands[0], 0) = force_reg (Pmode, callee);
  }"
)

(define_insn "*call_reg"
  [(call (mem:DI (match_operand:DI 0 "register_operand" "r"))
	 (match_operand 1 "" ""))
   (use (match_operand 2 "" ""))
   (clobber (reg:DI LR_REGNUM))]
  ""
  "blr\\t%0"
  [(set_attr "v8type" "call")]
)

(define_insn "*call_symbol"
  [(call (mem:DI (match_operand:DI 0 "" ""))
	 (match_operand 1 "" ""))
   (use (match_operand 2 "" ""))
   (clobber (reg:DI LR_REGNUM))]
  "GET_CODE (operands[0]) == SYMBOL_REF
   && !aarch64_is_long_call_p (operands[0])"
  "bl\\t%a0"
  [(set_attr "v8type" "call")]
)

(define_expand "call_value"
  [(parallel [(set (match_operand 0 "" "")
		   (call (match_operand 1 "memory_operand" "")
			 (match_operand 2 "general_operand" "")))
	      (use (match_operand 3 "" ""))
	      (clobber (reg:DI LR_REGNUM))])]
  ""
  "
  {
    rtx callee;

    /* In an untyped call, we can get NULL for operand 3.  */
    if (operands[3] == NULL)
      operands[3] = const0_rtx;

    /* Decide if we should generate indirect calls by loading the
       64-bit address of the callee into a register before performing
       the branch-and-link.  */
    callee = XEXP (operands[1], 0);
    if (GET_CODE (callee) == SYMBOL_REF
	? aarch64_is_long_call_p (callee)
	: !REG_P (callee))
      XEXP (operands[1], 0) = force_reg (Pmode, callee);
  }"
)

(define_insn "*call_value_reg"
  [(set (match_operand 0 "" "")
	(call (mem:DI (match_operand:DI 1 "register_operand" "r"))
		      (match_operand 2 "" "")))
   (use (match_operand 3 "" ""))
   (clobber (reg:DI LR_REGNUM))]
  ""
  "blr\\t%1"
  [(set_attr "v8type" "call")]
)

(define_insn "*call_value_symbol"
  [(set (match_operand 0 "" "")
	(call (mem:DI (match_operand:DI 1 "" ""))
	      (match_operand 2 "" "")))
   (use (match_operand 3 "" ""))
   (clobber (reg:DI LR_REGNUM))]
  "GET_CODE (operands[1]) == SYMBOL_REF
   && !aarch64_is_long_call_p (operands[1])"
  "bl\\t%a1"
  [(set_attr "v8type" "call")]
)

(define_expand "sibcall"
  [(parallel [(call (match_operand 0 "memory_operand" "")
		    (match_operand 1 "general_operand" ""))
	      (return)
	      (use (match_operand 2 "" ""))])]
  ""
  {
    if (operands[2] == NULL_RTX)
      operands[2] = const0_rtx;
  }
)

(define_expand "sibcall_value"
  [(parallel [(set (match_operand 0 "" "")
		   (call (match_operand 1 "memory_operand" "")
			 (match_operand 2 "general_operand" "")))
	      (return)
	      (use (match_operand 3 "" ""))])]
  ""
  {
    if (operands[3] == NULL_RTX)
      operands[3] = const0_rtx;
  }
)

(define_insn "*sibcall_insn"
  [(call (mem:DI (match_operand:DI 0 "" "X"))
	 (match_operand 1 "" ""))
   (return)
   (use (match_operand 2 "" ""))]
  "GET_CODE (operands[0]) == SYMBOL_REF"
  "b\\t%a0"
  [(set_attr "v8type" "branch")]
)

(define_insn "*sibcall_value_insn"
  [(set (match_operand 0 "" "")
	(call (mem:DI (match_operand 1 "" "X"))
	      (match_operand 2 "" "")))
   (return)
   (use (match_operand 3 "" ""))]
  "GET_CODE (operands[1]) == SYMBOL_REF"
  "b\\t%a1"
  [(set_attr "v8type" "branch")]
)

;; Call subroutine returning any type.

(define_expand "untyped_call"
  [(parallel [(call (match_operand 0 "")
		    (const_int 0))
	      (match_operand 1 "")
	      (match_operand 2 "")])]
  ""
{
  int i;

  emit_call_insn (GEN_CALL (operands[0], const0_rtx, NULL, const0_rtx));

  for (i = 0; i < XVECLEN (operands[2], 0); i++)
    {
      rtx set = XVECEXP (operands[2], 0, i);
      emit_move_insn (SET_DEST (set), SET_SRC (set));
    }

  /* The optimizer does not know that the call sets the function value
     registers we stored in the result block.  We avoid problems by
     claiming that all hard registers are used and clobbered at this
     point.  */
  emit_insn (gen_blockage ());
  DONE;
})

;; -------------------------------------------------------------------
;; Moves
;; -------------------------------------------------------------------

(define_expand "mov<mode>"
  [(set (match_operand:SHORT 0 "nonimmediate_operand" "")
	(match_operand:SHORT 1 "general_operand" ""))]
  ""
  "
    if (GET_CODE (operands[0]) == MEM && operands[1] != const0_rtx)
      operands[1] = force_reg (<MODE>mode, operands[1]);
  "
)

(define_insn "*mov<mode>_aarch64"
  [(set (match_operand:SHORT 0 "nonimmediate_operand" "=r,r,r,m,  r,*w")
        (match_operand:SHORT 1 "general_operand"      " r,M,m,rZ,*w,r"))]
  "(register_operand (operands[0], <MODE>mode)
    || aarch64_reg_or_zero (operands[1], <MODE>mode))"
  "@
   mov\\t%w0, %w1
   mov\\t%w0, %1
   ldr<size>\\t%w0, %1
   str<size>\\t%w1, %0
   umov\\t%w0, %1.<v>[0]
   dup\\t%0.<Vallxd>, %w1"
  [(set_attr "v8type" "move,alu,load1,store1,*,*")
   (set_attr "simd_type" "*,*,*,*,simd_movgp,simd_dupgp")
   (set_attr "mode" "<MODE>")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "mov<mode>"
  [(set (match_operand:GPI 0 "nonimmediate_operand" "")
	(match_operand:GPI 1 "general_operand" ""))]
  ""
  "
    if (GET_CODE (operands[0]) == MEM && operands[1] != const0_rtx)
      operands[1] = force_reg (<MODE>mode, operands[1]);

    if (CONSTANT_P (operands[1]))
      {
	aarch64_expand_mov_immediate (operands[0], operands[1]);
	DONE;
      }
  "
)

(define_insn "*movsi_aarch64"
  [(set (match_operand:SI 0 "nonimmediate_operand" "=r,r,r,m, *w, r,*w")
	(match_operand:SI 1 "aarch64_mov_operand"     " r,M,m,rZ,rZ,*w,*w"))]
  "(register_operand (operands[0], SImode)
    || aarch64_reg_or_zero (operands[1], SImode))"
  "@
   mov\\t%w0, %w1
   mov\\t%w0, %1
   ldr\\t%w0, %1
   str\\t%w1, %0
   fmov\\t%s0, %w1
   fmov\\t%w0, %s1
   fmov\\t%s0, %s1"
  [(set_attr "v8type" "move,alu,load1,store1,fmov,fmov,fmov")
   (set_attr "mode" "SI")
   (set_attr "fp" "*,*,*,*,yes,yes,yes")]
)

(define_insn "*movdi_aarch64"
  [(set (match_operand:DI 0 "nonimmediate_operand" "=r,k,r,r,r,m, r,  r,  *w, r,*w,w")
	(match_operand:DI 1 "aarch64_mov_operand"  " r,r,k,N,m,rZ,Usa,Ush,rZ,*w,*w,Dd"))]
  "(register_operand (operands[0], DImode)
    || aarch64_reg_or_zero (operands[1], DImode))"
  "@
   mov\\t%x0, %x1
   mov\\t%0, %x1
   mov\\t%x0, %1
   mov\\t%x0, %1
   ldr\\t%x0, %1
   str\\t%x1, %0
   adr\\t%x0, %a1
   adrp\\t%x0, %A1
   fmov\\t%d0, %x1
   fmov\\t%x0, %d1
   fmov\\t%d0, %d1
   movi\\t%d0, %1"
  [(set_attr "v8type" "move,move,move,alu,load1,store1,adr,adr,fmov,fmov,fmov,fmov")
   (set_attr "mode" "DI")
   (set_attr "fp" "*,*,*,*,*,*,*,*,yes,yes,yes,*")
   (set_attr "simd" "*,*,*,*,*,*,*,*,*,*,*,yes")]
)

(define_insn "insv_imm<mode>"
  [(set (zero_extract:GPI (match_operand:GPI 0 "register_operand" "+r")
			  (const_int 16)
			  (match_operand:GPI 1 "const_int_operand" "n"))
	(match_operand:GPI 2 "const_int_operand" "n"))]
  "INTVAL (operands[1]) < GET_MODE_BITSIZE (<MODE>mode)
   && INTVAL (operands[1]) % 16 == 0
   && UINTVAL (operands[2]) <= 0xffff"
  "movk\\t%<w>0, %X2, lsl %1"
  [(set_attr "v8type" "movk")
   (set_attr "mode" "<MODE>")]
)

(define_expand "movti"
  [(set (match_operand:TI 0 "nonimmediate_operand" "")
	(match_operand:TI 1 "general_operand" ""))]
  ""
  "
    if (GET_CODE (operands[0]) == MEM && operands[1] != const0_rtx)
      operands[1] = force_reg (TImode, operands[1]);
  "
)

(define_insn "*movti_aarch64"
  [(set (match_operand:TI 0
	 "nonimmediate_operand"  "=r, *w,r ,*w,r  ,Ump,Ump,*w,m")
	(match_operand:TI 1
	 "aarch64_movti_operand" " rn,r ,*w,*w,Ump,r  ,Z  , m,*w"))]
  "(register_operand (operands[0], TImode)
    || aarch64_reg_or_zero (operands[1], TImode))"
  "@
   #
   #
   #
   orr\\t%0.16b, %1.16b, %1.16b
   ldp\\t%0, %H0, %1
   stp\\t%1, %H1, %0
   stp\\txzr, xzr, %0
   ldr\\t%q0, %1
   str\\t%q1, %0"
  [(set_attr "v8type" "move2,fmovi2f,fmovf2i,*, \
		       load2,store2,store2,fpsimd_load,fpsimd_store")
   (set_attr "simd_type" "*,*,*,simd_move,*,*,*,*,*")
   (set_attr "mode" "DI,DI,DI,TI,DI,DI,DI,TI,TI")
   (set_attr "length" "8,8,8,4,4,4,4,4,4")
   (set_attr "fp" "*,*,*,*,*,*,*,yes,yes")
   (set_attr "simd" "*,*,*,yes,*,*,*,*,*")])

;; Split a TImode register-register or register-immediate move into
;; its component DImode pieces, taking care to handle overlapping
;; source and dest registers.
(define_split
   [(set (match_operand:TI 0 "register_operand" "")
	 (match_operand:TI 1 "aarch64_reg_or_imm" ""))]
  "reload_completed && aarch64_split_128bit_move_p (operands[0], operands[1])"
  [(const_int 0)]
{
  aarch64_split_128bit_move (operands[0], operands[1]);
  DONE;
})

(define_expand "mov<mode>"
  [(set (match_operand:GPF 0 "nonimmediate_operand" "")
	(match_operand:GPF 1 "general_operand" ""))]
  ""
  "
    if (!TARGET_FLOAT)
     {
	sorry (\"%qs and floating point code\", \"-mgeneral-regs-only\");
	FAIL;
     }

    if (GET_CODE (operands[0]) == MEM)
      operands[1] = force_reg (<MODE>mode, operands[1]);
  "
)

(define_insn "*movsf_aarch64"
  [(set (match_operand:SF 0 "nonimmediate_operand" "=w, ?r,w,w  ,w,m,r,m ,r")
	(match_operand:SF 1 "general_operand"      "?rY, w,w,Ufc,m,w,m,rY,r"))]
  "TARGET_FLOAT && (register_operand (operands[0], SFmode)
    || register_operand (operands[1], SFmode))"
  "@
   fmov\\t%s0, %w1
   fmov\\t%w0, %s1
   fmov\\t%s0, %s1
   fmov\\t%s0, %1
   ldr\\t%s0, %1
   str\\t%s1, %0
   ldr\\t%w0, %1
   str\\t%w1, %0
   mov\\t%w0, %w1"
  [(set_attr "v8type" "fmovi2f,fmovf2i,\
		       fmov,fconst,fpsimd_load,\
		       fpsimd_store,fpsimd_load,fpsimd_store,fmov")
   (set_attr "mode" "SF")]
)

(define_insn "*movdf_aarch64"
  [(set (match_operand:DF 0 "nonimmediate_operand" "=w, ?r,w,w  ,w,m,r,m ,r")
	(match_operand:DF 1 "general_operand"      "?rY, w,w,Ufc,m,w,m,rY,r"))]
  "TARGET_FLOAT && (register_operand (operands[0], DFmode)
    || register_operand (operands[1], DFmode))"
  "@
   fmov\\t%d0, %x1
   fmov\\t%x0, %d1
   fmov\\t%d0, %d1
   fmov\\t%d0, %1
   ldr\\t%d0, %1
   str\\t%d1, %0
   ldr\\t%x0, %1
   str\\t%x1, %0
   mov\\t%x0, %x1"
  [(set_attr "v8type" "fmovi2f,fmovf2i,\
		       fmov,fconst,fpsimd_load,\
		       fpsimd_store,fpsimd_load,fpsimd_store,move")
   (set_attr "mode" "DF")]
)

(define_expand "movtf"
  [(set (match_operand:TF 0 "nonimmediate_operand" "")
	(match_operand:TF 1 "general_operand" ""))]
  ""
  "
    if (!TARGET_FLOAT)
     {
	sorry (\"%qs and floating point code\", \"-mgeneral-regs-only\");
	FAIL;
     }

    if (GET_CODE (operands[0]) == MEM)
      operands[1] = force_reg (TFmode, operands[1]);
  "
)

(define_insn "*movtf_aarch64"
  [(set (match_operand:TF 0
	 "nonimmediate_operand" "=w,?&r,w ,?r,w,?w,w,m,?r ,Ump")
	(match_operand:TF 1
	 "general_operand"      " w,?r, ?r,w ,Y,Y ,m,w,Ump,?rY"))]
  "TARGET_FLOAT && (register_operand (operands[0], TFmode)
    || register_operand (operands[1], TFmode))"
  "@
   orr\\t%0.16b, %1.16b, %1.16b
   mov\\t%0, %1\;mov\\t%H0, %H1
   fmov\\t%d0, %Q1\;fmov\\t%0.d[1], %R1
   fmov\\t%Q0, %d1\;fmov\\t%R0, %1.d[1]
   movi\\t%0.2d, #0
   fmov\\t%s0, wzr
   ldr\\t%q0, %1
   str\\t%q1, %0
   ldp\\t%0, %H0, %1
   stp\\t%1, %H1, %0"
  [(set_attr "v8type" "logic,move2,fmovi2f,fmovf2i,fconst,fconst,fpsimd_load,fpsimd_store,fpsimd_load2,fpsimd_store2")
   (set_attr "mode" "DF,DF,DF,DF,DF,DF,TF,TF,DF,DF")
   (set_attr "length" "4,8,8,8,4,4,4,4,4,4")
   (set_attr "fp" "*,*,yes,yes,*,yes,yes,yes,*,*")
   (set_attr "simd" "yes,*,*,*,yes,*,*,*,*,*")]
)

;; Operands 1 and 3 are tied together by the final condition; so we allow
;; fairly lax checking on the second memory operation.
(define_insn "load_pair<mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(match_operand:GPI 1 "aarch64_mem_pair_operand" "Ump"))
   (set (match_operand:GPI 2 "register_operand" "=r")
        (match_operand:GPI 3 "memory_operand" "m"))]
  "rtx_equal_p (XEXP (operands[3], 0),
		plus_constant (Pmode,
			       XEXP (operands[1], 0),
			       GET_MODE_SIZE (<MODE>mode)))"
  "ldp\\t%<w>0, %<w>2, %1"
  [(set_attr "v8type" "load2")
   (set_attr "mode" "<MODE>")]
)

;; Operands 0 and 2 are tied together by the final condition; so we allow
;; fairly lax checking on the second memory operation.
(define_insn "store_pair<mode>"
  [(set (match_operand:GPI 0 "aarch64_mem_pair_operand" "=Ump")
	(match_operand:GPI 1 "register_operand" "r"))
   (set (match_operand:GPI 2 "memory_operand" "=m")
        (match_operand:GPI 3 "register_operand" "r"))]
  "rtx_equal_p (XEXP (operands[2], 0),
		plus_constant (Pmode,
			       XEXP (operands[0], 0),
			       GET_MODE_SIZE (<MODE>mode)))"
  "stp\\t%<w>1, %<w>3, %0"
  [(set_attr "v8type" "store2")
   (set_attr "mode" "<MODE>")]
)

;; Operands 1 and 3 are tied together by the final condition; so we allow
;; fairly lax checking on the second memory operation.
(define_insn "load_pair<mode>"
  [(set (match_operand:GPF 0 "register_operand" "=w")
	(match_operand:GPF 1 "aarch64_mem_pair_operand" "Ump"))
   (set (match_operand:GPF 2 "register_operand" "=w")
        (match_operand:GPF 3 "memory_operand" "m"))]
  "rtx_equal_p (XEXP (operands[3], 0),
		plus_constant (Pmode,
			       XEXP (operands[1], 0),
			       GET_MODE_SIZE (<MODE>mode)))"
  "ldp\\t%<w>0, %<w>2, %1"
  [(set_attr "v8type" "fpsimd_load2")
   (set_attr "mode" "<MODE>")]
)

;; Operands 0 and 2 are tied together by the final condition; so we allow
;; fairly lax checking on the second memory operation.
(define_insn "store_pair<mode>"
  [(set (match_operand:GPF 0 "aarch64_mem_pair_operand" "=Ump")
	(match_operand:GPF 1 "register_operand" "w"))
   (set (match_operand:GPF 2 "memory_operand" "=m")
        (match_operand:GPF 3 "register_operand" "w"))]
  "rtx_equal_p (XEXP (operands[2], 0),
		plus_constant (Pmode,
			       XEXP (operands[0], 0),
			       GET_MODE_SIZE (<MODE>mode)))"
  "stp\\t%<w>1, %<w>3, %0"
  [(set_attr "v8type" "fpsimd_load2")
   (set_attr "mode" "<MODE>")]
)

;; Load pair with writeback.  This is primarily used in function epilogues
;; when restoring [fp,lr]
(define_insn "loadwb_pair<GPI:mode>_<PTR:mode>"
  [(parallel
    [(set (match_operand:PTR 0 "register_operand" "=k")
          (plus:PTR (match_operand:PTR 1 "register_operand" "0")
                  (match_operand:PTR 4 "const_int_operand" "n")))
     (set (match_operand:GPI 2 "register_operand" "=r")
          (mem:GPI (plus:PTR (match_dup 1)
                   (match_dup 4))))
     (set (match_operand:GPI 3 "register_operand" "=r")
          (mem:GPI (plus:PTR (match_dup 1)
                   (match_operand:PTR 5 "const_int_operand" "n"))))])]
  "INTVAL (operands[5]) == INTVAL (operands[4]) + GET_MODE_SIZE (<GPI:MODE>mode)"
  "ldp\\t%<w>2, %<w>3, [%1], %4"
  [(set_attr "v8type" "load2")
   (set_attr "mode" "<GPI:MODE>")]
)

;; Store pair with writeback.  This is primarily used in function prologues
;; when saving [fp,lr]
(define_insn "storewb_pair<GPI:mode>_<PTR:mode>"
  [(parallel
    [(set (match_operand:PTR 0 "register_operand" "=&k")
          (plus:PTR (match_operand:PTR 1 "register_operand" "0")
                  (match_operand:PTR 4 "const_int_operand" "n")))
     (set (mem:GPI (plus:PTR (match_dup 0)
                   (match_dup 4)))
          (match_operand:GPI 2 "register_operand" "r"))
     (set (mem:GPI (plus:PTR (match_dup 0)
                   (match_operand:PTR 5 "const_int_operand" "n")))
          (match_operand:GPI 3 "register_operand" "r"))])]
  "INTVAL (operands[5]) == INTVAL (operands[4]) + GET_MODE_SIZE (<GPI:MODE>mode)"
  "stp\\t%<w>2, %<w>3, [%0, %4]!"
  [(set_attr "v8type" "store2")
   (set_attr "mode" "<GPI:MODE>")]
)

;; -------------------------------------------------------------------
;; Sign/Zero extension
;; -------------------------------------------------------------------

(define_expand "<optab>sidi2"
  [(set (match_operand:DI 0 "register_operand")
	(ANY_EXTEND:DI (match_operand:SI 1 "nonimmediate_operand")))]
  ""
)

(define_insn "*extendsidi2_aarch64"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (sign_extend:DI (match_operand:SI 1 "nonimmediate_operand" "r,m")))]
  ""
  "@
   sxtw\t%0, %w1
   ldrsw\t%0, %1"
  [(set_attr "v8type" "extend,load1")
   (set_attr "mode" "DI")]
)

(define_insn "*zero_extendsidi2_aarch64"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (zero_extend:DI (match_operand:SI 1 "nonimmediate_operand" "r,m")))]
  ""
  "@
   uxtw\t%0, %w1
   ldr\t%w0, %1"
  [(set_attr "v8type" "extend,load1")
   (set_attr "mode" "DI")]
)

(define_expand "<ANY_EXTEND:optab><SHORT:mode><GPI:mode>2"
  [(set (match_operand:GPI 0 "register_operand")
        (ANY_EXTEND:GPI (match_operand:SHORT 1 "nonimmediate_operand")))]
  ""
)

(define_insn "*extend<SHORT:mode><GPI:mode>2_aarch64"
  [(set (match_operand:GPI 0 "register_operand" "=r,r")
        (sign_extend:GPI (match_operand:SHORT 1 "nonimmediate_operand" "r,m")))]
  ""
  "@
   sxt<SHORT:size>\t%<GPI:w>0, %w1
   ldrs<SHORT:size>\t%<GPI:w>0, %1"
  [(set_attr "v8type" "extend,load1")
   (set_attr "mode" "<GPI:MODE>")]
)

(define_insn "*zero_extend<SHORT:mode><GPI:mode>2_aarch64"
  [(set (match_operand:GPI 0 "register_operand" "=r,r")
        (zero_extend:GPI (match_operand:SHORT 1 "nonimmediate_operand" "r,m")))]
  ""
  "@
   uxt<SHORT:size>\t%<GPI:w>0, %w1
   ldr<SHORT:size>\t%w0, %1"
  [(set_attr "v8type" "extend,load1")
   (set_attr "mode" "<GPI:MODE>")]
)

(define_expand "<optab>qihi2"
  [(set (match_operand:HI 0 "register_operand")
        (ANY_EXTEND:HI (match_operand:QI 1 "nonimmediate_operand")))]
  ""
)

(define_insn "*<optab>qihi2_aarch64"
  [(set (match_operand:HI 0 "register_operand" "=r,r")
        (ANY_EXTEND:HI (match_operand:QI 1 "nonimmediate_operand" "r,m")))]
  ""
  "@
   <su>xtb\t%w0, %w1
   <ldrxt>b\t%w0, %1"
  [(set_attr "v8type" "extend,load1")
   (set_attr "mode" "HI")]
)

;; -------------------------------------------------------------------
;; Simple arithmetic
;; -------------------------------------------------------------------

(define_expand "add<mode>3"
  [(set
    (match_operand:GPI 0 "register_operand" "")
    (plus:GPI (match_operand:GPI 1 "register_operand" "")
	      (match_operand:GPI 2 "aarch64_pluslong_operand" "")))]
  ""
  "
  if (! aarch64_plus_operand (operands[2], VOIDmode))
    {
      rtx subtarget = ((optimize && can_create_pseudo_p ())
		       ? gen_reg_rtx (<MODE>mode) : operands[0]);
      HOST_WIDE_INT imm = INTVAL (operands[2]);

      if (imm < 0)
	imm = -(-imm & ~0xfff);
      else
        imm &= ~0xfff;

      emit_insn (gen_add<mode>3 (subtarget, operands[1], GEN_INT (imm)));
      operands[1] = subtarget;
      operands[2] = GEN_INT (INTVAL (operands[2]) - imm);
    }
  "
)

(define_insn "*addsi3_aarch64"
  [(set
    (match_operand:SI 0 "register_operand" "=rk,rk,rk")
    (plus:SI
     (match_operand:SI 1 "register_operand" "%rk,rk,rk")
     (match_operand:SI 2 "aarch64_plus_operand" "I,r,J")))]
  ""
  "@
  add\\t%w0, %w1, %2
  add\\t%w0, %w1, %w2
  sub\\t%w0, %w1, #%n2"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "SI")]
)

;; zero_extend version of above
(define_insn "*addsi3_aarch64_uxtw"
  [(set
    (match_operand:DI 0 "register_operand" "=rk,rk,rk")
    (zero_extend:DI
     (plus:SI (match_operand:SI 1 "register_operand" "%rk,rk,rk")
              (match_operand:SI 2 "aarch64_plus_operand" "I,r,J"))))]
  ""
  "@
  add\\t%w0, %w1, %2
  add\\t%w0, %w1, %w2
  sub\\t%w0, %w1, #%n2"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "SI")]
)

(define_insn "*adddi3_aarch64"
  [(set
    (match_operand:DI 0 "register_operand" "=rk,rk,rk,!w")
    (plus:DI
     (match_operand:DI 1 "register_operand" "%rk,rk,rk,!w")
     (match_operand:DI 2 "aarch64_plus_operand" "I,r,J,!w")))]
  ""
  "@
  add\\t%x0, %x1, %2
  add\\t%x0, %x1, %x2
  sub\\t%x0, %x1, #%n2
  add\\t%d0, %d1, %d2"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "DI")
   (set_attr "simd" "*,*,*,yes")]
)

(define_insn "*add<mode>3_compare0"
  [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ
	 (plus:GPI (match_operand:GPI 1 "register_operand" "%r,r")
		   (match_operand:GPI 2 "aarch64_plus_operand" "rI,J"))
	 (const_int 0)))
   (set (match_operand:GPI 0 "register_operand" "=r,r")
	(plus:GPI (match_dup 1) (match_dup 2)))]
  ""
  "@
  adds\\t%<w>0, %<w>1, %<w>2
  subs\\t%<w>0, %<w>1, #%n2"
  [(set_attr "v8type" "alus")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*addsi3_compare0_uxtw"
  [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ
	 (plus:SI (match_operand:SI 1 "register_operand" "%r,r")
		  (match_operand:SI 2 "aarch64_plus_operand" "rI,J"))
	 (const_int 0)))
   (set (match_operand:DI 0 "register_operand" "=r,r")
	(zero_extend:DI (plus:SI (match_dup 1) (match_dup 2))))]
  ""
  "@
  adds\\t%w0, %w1, %w2
  subs\\t%w0, %w1, #%n2"
  [(set_attr "v8type" "alus")
   (set_attr "mode" "SI")]
)

(define_insn "*add<mode>3nr_compare0"
  [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ
	 (plus:GPI (match_operand:GPI 0 "register_operand" "%r,r")
		   (match_operand:GPI 1 "aarch64_plus_operand" "rI,J"))
	 (const_int 0)))]
  ""
  "@
  cmn\\t%<w>0, %<w>1
  cmp\\t%<w>0, #%n1"
  [(set_attr "v8type" "alus")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*compare_neg<mode>"
  [(set (reg:CC CC_REGNUM)
	(compare:CC
	 (match_operand:GPI 0 "register_operand" "r")
	 (neg:GPI (match_operand:GPI 1 "register_operand" "r"))))]
  ""
  "cmn\\t%<w>0, %<w>1"
  [(set_attr "v8type" "alus")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*add_<shift>_<mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(plus:GPI (ASHIFT:GPI (match_operand:GPI 1 "register_operand" "r")
			      (match_operand:QI 2 "aarch64_shift_imm_<mode>" "n"))
		  (match_operand:GPI 3 "register_operand" "r")))]
  ""
  "add\\t%<w>0, %<w>3, %<w>1, <shift> %2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*add_<shift>_si_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
	 (plus:SI (ASHIFT:SI (match_operand:SI 1 "register_operand" "r")
		             (match_operand:QI 2 "aarch64_shift_imm_si" "n"))
	          (match_operand:SI 3 "register_operand" "r"))))]
  ""
  "add\\t%w0, %w3, %w1, <shift> %2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "SI")]
)

(define_insn "*add_mul_imm_<mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(plus:GPI (mult:GPI (match_operand:GPI 1 "register_operand" "r")
			    (match_operand:QI 2 "aarch64_pwr_2_<mode>" "n"))
		  (match_operand:GPI 3 "register_operand" "r")))]
  ""
  "add\\t%<w>0, %<w>3, %<w>1, lsl %p2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*add_<optab><ALLX:mode>_<GPI:mode>"
  [(set (match_operand:GPI 0 "register_operand" "=rk")
	(plus:GPI (ANY_EXTEND:GPI (match_operand:ALLX 1 "register_operand" "r"))
		  (match_operand:GPI 2 "register_operand" "r")))]
  ""
  "add\\t%<GPI:w>0, %<GPI:w>2, %<GPI:w>1, <su>xt<ALLX:size>"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "<GPI:MODE>")]
)

;; zero_extend version of above
(define_insn "*add_<optab><SHORT:mode>_si_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI
         (plus:SI (ANY_EXTEND:SI (match_operand:SHORT 1 "register_operand" "r"))
		  (match_operand:GPI 2 "register_operand" "r"))))]
  ""
  "add\\t%w0, %w2, %w1, <su>xt<SHORT:size>"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "SI")]
)

(define_insn "*add_<optab><ALLX:mode>_shft_<GPI:mode>"
  [(set (match_operand:GPI 0 "register_operand" "=rk")
	(plus:GPI (ashift:GPI (ANY_EXTEND:GPI
			       (match_operand:ALLX 1 "register_operand" "r"))
			      (match_operand 2 "aarch64_imm3" "Ui3"))
		  (match_operand:GPI 3 "register_operand" "r")))]
  ""
  "add\\t%<GPI:w>0, %<GPI:w>3, %<GPI:w>1, <su>xt<ALLX:size> %2"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "<GPI:MODE>")]
)

;; zero_extend version of above
(define_insn "*add_<optab><SHORT:mode>_shft_si_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI
	 (plus:SI (ashift:SI (ANY_EXTEND:SI
			      (match_operand:SHORT 1 "register_operand" "r"))
			     (match_operand 2 "aarch64_imm3" "Ui3"))
		  (match_operand:SI 3 "register_operand" "r"))))]
  ""
  "add\\t%w0, %w3, %w1, <su>xt<SHORT:size> %2"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "SI")]
)

(define_insn "*add_<optab><ALLX:mode>_mult_<GPI:mode>"
  [(set (match_operand:GPI 0 "register_operand" "=rk")
	(plus:GPI (mult:GPI (ANY_EXTEND:GPI
			     (match_operand:ALLX 1 "register_operand" "r"))
			    (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		  (match_operand:GPI 3 "register_operand" "r")))]
  ""
  "add\\t%<GPI:w>0, %<GPI:w>3, %<GPI:w>1, <su>xt<ALLX:size> %p2"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "<GPI:MODE>")]
)

;; zero_extend version of above
(define_insn "*add_<optab><SHORT:mode>_mult_si_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI (plus:SI (mult:SI (ANY_EXTEND:SI
			     (match_operand:SHORT 1 "register_operand" "r"))
			    (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		  (match_operand:SI 3 "register_operand" "r"))))]
  ""
  "add\\t%w0, %w3, %w1, <su>xt<SHORT:size> %p2"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "SI")]
)

(define_insn "*add_<optab><mode>_multp2"
  [(set (match_operand:GPI 0 "register_operand" "=rk")
	(plus:GPI (ANY_EXTRACT:GPI
		   (mult:GPI (match_operand:GPI 1 "register_operand" "r")
			     (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		   (match_operand 3 "const_int_operand" "n")
		   (const_int 0))
		  (match_operand:GPI 4 "register_operand" "r")))]
  "aarch64_is_extend_from_extract (<MODE>mode, operands[2], operands[3])"
  "add\\t%<w>0, %<w>4, %<w>1, <su>xt%e3 %p2"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*add_<optab>si_multp2_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI
         (plus:SI (ANY_EXTRACT:SI
		   (mult:SI (match_operand:SI 1 "register_operand" "r")
			    (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		   (match_operand 3 "const_int_operand" "n")
                   (const_int 0))
		  (match_operand:SI 4 "register_operand" "r"))))]
  "aarch64_is_extend_from_extract (SImode, operands[2], operands[3])"
  "add\\t%w0, %w4, %w1, <su>xt%e3 %p2"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "SI")]
)

(define_insn "*add<mode>3_carryin"
  [(set
    (match_operand:GPI 0 "register_operand" "=r")
    (plus:GPI (geu:GPI (reg:CC CC_REGNUM) (const_int 0))
	      (plus:GPI
		(match_operand:GPI 1 "register_operand" "r")
		(match_operand:GPI 2 "register_operand" "r"))))]
   ""
   "adc\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "adc")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*addsi3_carryin_uxtw"
  [(set
    (match_operand:DI 0 "register_operand" "=r")
    (zero_extend:DI
     (plus:SI (geu:SI (reg:CC CC_REGNUM) (const_int 0))
	      (plus:SI
	       (match_operand:SI 1 "register_operand" "r")
	       (match_operand:SI 2 "register_operand" "r")))))]
   ""
   "adc\\t%w0, %w1, %w2"
  [(set_attr "v8type" "adc")
   (set_attr "mode" "SI")]
)

(define_insn "*add<mode>3_carryin_alt1"
  [(set
    (match_operand:GPI 0 "register_operand" "=r")
    (plus:GPI (plus:GPI
		(match_operand:GPI 1 "register_operand" "r")
		(match_operand:GPI 2 "register_operand" "r"))
              (geu:GPI (reg:CC CC_REGNUM) (const_int 0))))]
   ""
   "adc\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "adc")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*addsi3_carryin_alt1_uxtw"
  [(set
    (match_operand:DI 0 "register_operand" "=r")
    (zero_extend:DI
     (plus:SI (plus:SI
	       (match_operand:SI 1 "register_operand" "r")
	       (match_operand:SI 2 "register_operand" "r"))
              (geu:SI (reg:CC CC_REGNUM) (const_int 0)))))]
   ""
   "adc\\t%w0, %w1, %w2"
  [(set_attr "v8type" "adc")
   (set_attr "mode" "SI")]
)

(define_insn "*add<mode>3_carryin_alt2"
  [(set
    (match_operand:GPI 0 "register_operand" "=r")
    (plus:GPI (plus:GPI
                (geu:GPI (reg:CC CC_REGNUM) (const_int 0))
		(match_operand:GPI 1 "register_operand" "r"))
	      (match_operand:GPI 2 "register_operand" "r")))]
   ""
   "adc\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "adc")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*addsi3_carryin_alt2_uxtw"
  [(set
    (match_operand:DI 0 "register_operand" "=r")
    (zero_extend:DI
     (plus:SI (plus:SI
               (geu:SI (reg:CC CC_REGNUM) (const_int 0))
	       (match_operand:SI 1 "register_operand" "r"))
	      (match_operand:SI 2 "register_operand" "r"))))]
   ""
   "adc\\t%w0, %w1, %w2"
  [(set_attr "v8type" "adc")
   (set_attr "mode" "SI")]
)

(define_insn "*add<mode>3_carryin_alt3"
  [(set
    (match_operand:GPI 0 "register_operand" "=r")
    (plus:GPI (plus:GPI
                (geu:GPI (reg:CC CC_REGNUM) (const_int 0))
		(match_operand:GPI 2 "register_operand" "r"))
	      (match_operand:GPI 1 "register_operand" "r")))]
   ""
   "adc\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "adc")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*addsi3_carryin_alt3_uxtw"
  [(set
    (match_operand:DI 0 "register_operand" "=r")
    (zero_extend:DI
     (plus:SI (plus:SI
               (geu:SI (reg:CC CC_REGNUM) (const_int 0))
	       (match_operand:SI 2 "register_operand" "r"))
	      (match_operand:SI 1 "register_operand" "r"))))]
   ""
   "adc\\t%w0, %w1, %w2"
  [(set_attr "v8type" "adc")
   (set_attr "mode" "SI")]
)

(define_insn "*add_uxt<mode>_multp2"
  [(set (match_operand:GPI 0 "register_operand" "=rk")
	(plus:GPI (and:GPI
		   (mult:GPI (match_operand:GPI 1 "register_operand" "r")
			     (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		   (match_operand 3 "const_int_operand" "n"))
		  (match_operand:GPI 4 "register_operand" "r")))]
  "aarch64_uxt_size (exact_log2 (INTVAL (operands[2])), INTVAL (operands[3])) != 0"
  "*
  operands[3] = GEN_INT (aarch64_uxt_size (exact_log2 (INTVAL (operands[2])),
					   INTVAL (operands[3])));
  return \"add\t%<w>0, %<w>4, %<w>1, uxt%e3 %p2\";"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*add_uxtsi_multp2_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI
         (plus:SI (and:SI
		   (mult:SI (match_operand:SI 1 "register_operand" "r")
			    (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		   (match_operand 3 "const_int_operand" "n"))
		  (match_operand:SI 4 "register_operand" "r"))))]
  "aarch64_uxt_size (exact_log2 (INTVAL (operands[2])), INTVAL (operands[3])) != 0"
  "*
  operands[3] = GEN_INT (aarch64_uxt_size (exact_log2 (INTVAL (operands[2])),
					   INTVAL (operands[3])));
  return \"add\t%w0, %w4, %w1, uxt%e3 %p2\";"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "SI")]
)

(define_insn "subsi3"
  [(set (match_operand:SI 0 "register_operand" "=rk")
	(minus:SI (match_operand:SI 1 "register_operand" "r")
		   (match_operand:SI 2 "register_operand" "r")))]
  ""
  "sub\\t%w0, %w1, %w2"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "SI")]
)

;; zero_extend version of above
(define_insn "*subsi3_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI
         (minus:SI (match_operand:SI 1 "register_operand" "r")
		   (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "sub\\t%w0, %w1, %w2"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "SI")]
)

(define_insn "subdi3"
  [(set (match_operand:DI 0 "register_operand" "=rk,!w")
	(minus:DI (match_operand:DI 1 "register_operand" "r,!w")
		   (match_operand:DI 2 "register_operand" "r,!w")))]
  ""
  "@
   sub\\t%x0, %x1, %x2
   sub\\t%d0, %d1, %d2"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "DI")
   (set_attr "simd" "*,yes")]
)


(define_insn "*sub<mode>3_compare0"
  [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ (minus:GPI (match_operand:GPI 1 "register_operand" "r")
				  (match_operand:GPI 2 "register_operand" "r"))
		       (const_int 0)))
   (set (match_operand:GPI 0 "register_operand" "=r")
	(minus:GPI (match_dup 1) (match_dup 2)))]
  ""
  "subs\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "alus")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*subsi3_compare0_uxtw"
  [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ (minus:SI (match_operand:SI 1 "register_operand" "r")
				 (match_operand:SI 2 "register_operand" "r"))
		       (const_int 0)))
   (set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI (minus:SI (match_dup 1) (match_dup 2))))]
  ""
  "subs\\t%w0, %w1, %w2"
  [(set_attr "v8type" "alus")
   (set_attr "mode" "SI")]
)

(define_insn "*sub_<shift>_<mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(minus:GPI (match_operand:GPI 3 "register_operand" "r")
		   (ASHIFT:GPI
		    (match_operand:GPI 1 "register_operand" "r")
		    (match_operand:QI 2 "aarch64_shift_imm_<mode>" "n"))))]
  ""
  "sub\\t%<w>0, %<w>3, %<w>1, <shift> %2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*sub_<shift>_si_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
         (minus:SI (match_operand:SI 3 "register_operand" "r")
		   (ASHIFT:SI
		    (match_operand:SI 1 "register_operand" "r")
		    (match_operand:QI 2 "aarch64_shift_imm_si" "n")))))]
  ""
  "sub\\t%w0, %w3, %w1, <shift> %2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "SI")]
)

(define_insn "*sub_mul_imm_<mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(minus:GPI (match_operand:GPI 3 "register_operand" "r")
		   (mult:GPI
		    (match_operand:GPI 1 "register_operand" "r")
		    (match_operand:QI 2 "aarch64_pwr_2_<mode>" "n"))))]
  ""
  "sub\\t%<w>0, %<w>3, %<w>1, lsl %p2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*sub_mul_imm_si_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
         (minus:SI (match_operand:SI 3 "register_operand" "r")
		   (mult:SI
		    (match_operand:SI 1 "register_operand" "r")
		    (match_operand:QI 2 "aarch64_pwr_2_si" "n")))))]
  ""
  "sub\\t%w0, %w3, %w1, lsl %p2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "SI")]
)

(define_insn "*sub_<optab><ALLX:mode>_<GPI:mode>"
  [(set (match_operand:GPI 0 "register_operand" "=rk")
	(minus:GPI (match_operand:GPI 1 "register_operand" "r")
		   (ANY_EXTEND:GPI
		    (match_operand:ALLX 2 "register_operand" "r"))))]
  ""
  "sub\\t%<GPI:w>0, %<GPI:w>1, %<GPI:w>2, <su>xt<ALLX:size>"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "<GPI:MODE>")]
)

;; zero_extend version of above
(define_insn "*sub_<optab><SHORT:mode>_si_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI
         (minus:SI (match_operand:SI 1 "register_operand" "r")
		   (ANY_EXTEND:SI
		    (match_operand:SHORT 2 "register_operand" "r")))))]
  ""
  "sub\\t%w0, %w1, %w2, <su>xt<SHORT:size>"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "SI")]
)

(define_insn "*sub_<optab><ALLX:mode>_shft_<GPI:mode>"
  [(set (match_operand:GPI 0 "register_operand" "=rk")
	(minus:GPI (match_operand:GPI 1 "register_operand" "r")
		   (ashift:GPI (ANY_EXTEND:GPI
				(match_operand:ALLX 2 "register_operand" "r"))
			       (match_operand 3 "aarch64_imm3" "Ui3"))))]
  ""
  "sub\\t%<GPI:w>0, %<GPI:w>1, %<GPI:w>2, <su>xt<ALLX:size> %3"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "<GPI:MODE>")]
)

;; zero_extend version of above
(define_insn "*sub_<optab><SHORT:mode>_shft_si_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI
         (minus:SI (match_operand:SI 1 "register_operand" "r")
		   (ashift:SI (ANY_EXTEND:SI
			       (match_operand:SHORT 2 "register_operand" "r"))
			      (match_operand 3 "aarch64_imm3" "Ui3")))))]
  ""
  "sub\\t%w0, %w1, %w2, <su>xt<SHORT:size> %3"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "SI")]
)

(define_insn "*sub_<optab><mode>_multp2"
  [(set (match_operand:GPI 0 "register_operand" "=rk")
	(minus:GPI (match_operand:GPI 4 "register_operand" "r")
		   (ANY_EXTRACT:GPI
		    (mult:GPI (match_operand:GPI 1 "register_operand" "r")
			      (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		    (match_operand 3 "const_int_operand" "n")
		    (const_int 0))))]
  "aarch64_is_extend_from_extract (<MODE>mode, operands[2], operands[3])"
  "sub\\t%<w>0, %<w>4, %<w>1, <su>xt%e3 %p2"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*sub_<optab>si_multp2_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI
         (minus:SI (match_operand:SI 4 "register_operand" "r")
		   (ANY_EXTRACT:SI
		    (mult:SI (match_operand:SI 1 "register_operand" "r")
			     (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		    (match_operand 3 "const_int_operand" "n")
		    (const_int 0)))))]
  "aarch64_is_extend_from_extract (SImode, operands[2], operands[3])"
  "sub\\t%w0, %w4, %w1, <su>xt%e3 %p2"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "SI")]
)

(define_insn "*sub_uxt<mode>_multp2"
  [(set (match_operand:GPI 0 "register_operand" "=rk")
	(minus:GPI (match_operand:GPI 4 "register_operand" "r")
		   (and:GPI
		    (mult:GPI (match_operand:GPI 1 "register_operand" "r")
			      (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		    (match_operand 3 "const_int_operand" "n"))))]
  "aarch64_uxt_size (exact_log2 (INTVAL (operands[2])),INTVAL (operands[3])) != 0"
  "*
  operands[3] = GEN_INT (aarch64_uxt_size (exact_log2 (INTVAL (operands[2])),
					   INTVAL (operands[3])));
  return \"sub\t%<w>0, %<w>4, %<w>1, uxt%e3 %p2\";"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*sub_uxtsi_multp2_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=rk")
	(zero_extend:DI
         (minus:SI (match_operand:SI 4 "register_operand" "r")
		   (and:SI
		    (mult:SI (match_operand:SI 1 "register_operand" "r")
			     (match_operand 2 "aarch64_pwr_imm3" "Up3"))
		    (match_operand 3 "const_int_operand" "n")))))]
  "aarch64_uxt_size (exact_log2 (INTVAL (operands[2])),INTVAL (operands[3])) != 0"
  "*
  operands[3] = GEN_INT (aarch64_uxt_size (exact_log2 (INTVAL (operands[2])),
					   INTVAL (operands[3])));
  return \"sub\t%w0, %w4, %w1, uxt%e3 %p2\";"
  [(set_attr "v8type" "alu_ext")
   (set_attr "mode" "SI")]
)

(define_insn "neg<mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(neg:GPI (match_operand:GPI 1 "register_operand" "r")))]
  ""
  "neg\\t%<w>0, %<w>1"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*negsi2_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI (neg:SI (match_operand:SI 1 "register_operand" "r"))))]
  ""
  "neg\\t%w0, %w1"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "SI")]
)

(define_insn "*neg<mode>2_compare0"
  [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ (neg:GPI (match_operand:GPI 1 "register_operand" "r"))
		       (const_int 0)))
   (set (match_operand:GPI 0 "register_operand" "=r")
	(neg:GPI (match_dup 1)))]
  ""
  "negs\\t%<w>0, %<w>1"
  [(set_attr "v8type" "alus")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*negsi2_compare0_uxtw"
  [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ (neg:SI (match_operand:SI 1 "register_operand" "r"))
		       (const_int 0)))
   (set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI (neg:SI (match_dup 1))))]
  ""
  "negs\\t%w0, %w1"
  [(set_attr "v8type" "alus")
   (set_attr "mode" "SI")]
)

(define_insn "*neg_<shift>_<mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(neg:GPI (ASHIFT:GPI
		  (match_operand:GPI 1 "register_operand" "r")
		  (match_operand:QI 2 "aarch64_shift_imm_<mode>" "n"))))]
  ""
  "neg\\t%<w>0, %<w>1, <shift> %2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*neg_<shift>_si2_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
         (neg:SI (ASHIFT:SI
		  (match_operand:SI 1 "register_operand" "r")
		  (match_operand:QI 2 "aarch64_shift_imm_si" "n")))))]
  ""
  "neg\\t%w0, %w1, <shift> %2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "SI")]
)

(define_insn "*neg_mul_imm_<mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(neg:GPI (mult:GPI
		  (match_operand:GPI 1 "register_operand" "r")
		  (match_operand:QI 2 "aarch64_pwr_2_<mode>" "n"))))]
  ""
  "neg\\t%<w>0, %<w>1, lsl %p2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*neg_mul_imm_si2_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
         (neg:SI (mult:SI
		  (match_operand:SI 1 "register_operand" "r")
		  (match_operand:QI 2 "aarch64_pwr_2_si" "n")))))]
  ""
  "neg\\t%w0, %w1, lsl %p2"
  [(set_attr "v8type" "alu_shift")
   (set_attr "mode" "SI")]
)

(define_insn "mul<mode>3"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(mult:GPI (match_operand:GPI 1 "register_operand" "r")
		  (match_operand:GPI 2 "register_operand" "r")))]
  ""
  "mul\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "mult")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*mulsi3_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
         (mult:SI (match_operand:SI 1 "register_operand" "r")
		  (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "mul\\t%w0, %w1, %w2"
  [(set_attr "v8type" "mult")
   (set_attr "mode" "SI")]
)

(define_insn "*madd<mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(plus:GPI (mult:GPI (match_operand:GPI 1 "register_operand" "r")
			    (match_operand:GPI 2 "register_operand" "r"))
		  (match_operand:GPI 3 "register_operand" "r")))]
  ""
  "madd\\t%<w>0, %<w>1, %<w>2, %<w>3"
  [(set_attr "v8type" "madd")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*maddsi_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
         (plus:SI (mult:SI (match_operand:SI 1 "register_operand" "r")
			   (match_operand:SI 2 "register_operand" "r"))
		  (match_operand:SI 3 "register_operand" "r"))))]
  ""
  "madd\\t%w0, %w1, %w2, %w3"
  [(set_attr "v8type" "madd")
   (set_attr "mode" "SI")]
)

(define_insn "*msub<mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(minus:GPI (match_operand:GPI 3 "register_operand" "r")
		   (mult:GPI (match_operand:GPI 1 "register_operand" "r")
			     (match_operand:GPI 2 "register_operand" "r"))))]

  ""
  "msub\\t%<w>0, %<w>1, %<w>2, %<w>3"
  [(set_attr "v8type" "madd")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*msubsi_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
         (minus:SI (match_operand:SI 3 "register_operand" "r")
		   (mult:SI (match_operand:SI 1 "register_operand" "r")
			    (match_operand:SI 2 "register_operand" "r")))))]

  ""
  "msub\\t%w0, %w1, %w2, %w3"
  [(set_attr "v8type" "madd")
   (set_attr "mode" "SI")]
)

(define_insn "*mul<mode>_neg"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(mult:GPI (neg:GPI (match_operand:GPI 1 "register_operand" "r"))
		  (match_operand:GPI 2 "register_operand" "r")))]

  ""
  "mneg\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "mult")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*mulsi_neg_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
         (mult:SI (neg:SI (match_operand:SI 1 "register_operand" "r"))
		  (match_operand:SI 2 "register_operand" "r"))))]

  ""
  "mneg\\t%w0, %w1, %w2"
  [(set_attr "v8type" "mult")
   (set_attr "mode" "SI")]
)

(define_insn "<su_optab>mulsidi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(mult:DI (ANY_EXTEND:DI (match_operand:SI 1 "register_operand" "r"))
		 (ANY_EXTEND:DI (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "<su>mull\\t%0, %w1, %w2"
  [(set_attr "v8type" "mull")
   (set_attr "mode" "DI")]
)

(define_insn "<su_optab>maddsidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(plus:DI (mult:DI
		  (ANY_EXTEND:DI (match_operand:SI 1 "register_operand" "r"))
		  (ANY_EXTEND:DI (match_operand:SI 2 "register_operand" "r")))
		 (match_operand:DI 3 "register_operand" "r")))]
  ""
  "<su>maddl\\t%0, %w1, %w2, %3"
  [(set_attr "v8type" "maddl")
   (set_attr "mode" "DI")]
)

(define_insn "<su_optab>msubsidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(minus:DI
	 (match_operand:DI 3 "register_operand" "r")
	 (mult:DI (ANY_EXTEND:DI (match_operand:SI 1 "register_operand" "r"))
		  (ANY_EXTEND:DI
		   (match_operand:SI 2 "register_operand" "r")))))]
  ""
  "<su>msubl\\t%0, %w1, %w2, %3"
  [(set_attr "v8type" "maddl")
   (set_attr "mode" "DI")]
)

(define_insn "*<su_optab>mulsidi_neg"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(mult:DI (neg:DI
		  (ANY_EXTEND:DI (match_operand:SI 1 "register_operand" "r")))
		  (ANY_EXTEND:DI (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "<su>mnegl\\t%0, %w1, %w2"
  [(set_attr "v8type" "mull")
   (set_attr "mode" "DI")]
)

(define_insn "<su>muldi3_highpart"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(truncate:DI
	 (lshiftrt:TI
	  (mult:TI
	   (ANY_EXTEND:TI (match_operand:DI 1 "register_operand" "r"))
	   (ANY_EXTEND:TI (match_operand:DI 2 "register_operand" "r")))
	  (const_int 64))))]
  ""
  "<su>mulh\\t%0, %1, %2"
  [(set_attr "v8type" "mulh")
   (set_attr "mode" "DI")]
)

(define_insn "<su_optab>div<mode>3"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(ANY_DIV:GPI (match_operand:GPI 1 "register_operand" "r")
		     (match_operand:GPI 2 "register_operand" "r")))]
  ""
  "<su>div\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "<su>div")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*<su_optab>divsi3_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
         (ANY_DIV:SI (match_operand:SI 1 "register_operand" "r")
		     (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "<su>div\\t%w0, %w1, %w2"
  [(set_attr "v8type" "<su>div")
   (set_attr "mode" "SI")]
)

;; -------------------------------------------------------------------
;; Comparison insns
;; -------------------------------------------------------------------

(define_insn "*cmp<mode>"
  [(set (reg:CC CC_REGNUM)
	(compare:CC (match_operand:GPI 0 "register_operand" "r,r")
		    (match_operand:GPI 1 "aarch64_plus_operand" "rI,J")))]
  ""
  "@
   cmp\\t%<w>0, %<w>1
   cmn\\t%<w>0, #%n1"
  [(set_attr "v8type" "alus")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*cmp<mode>"
  [(set (reg:CCFP CC_REGNUM)
        (compare:CCFP (match_operand:GPF 0 "register_operand" "w,w")
		      (match_operand:GPF 1 "aarch64_fp_compare_operand" "Y,w")))]
   "TARGET_FLOAT"
   "@
    fcmp\\t%<s>0, #0.0
    fcmp\\t%<s>0, %<s>1"
  [(set_attr "v8type" "fcmp")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*cmpe<mode>"
  [(set (reg:CCFPE CC_REGNUM)
        (compare:CCFPE (match_operand:GPF 0 "register_operand" "w,w")
		       (match_operand:GPF 1 "aarch64_fp_compare_operand" "Y,w")))]
   "TARGET_FLOAT"
   "@
    fcmpe\\t%<s>0, #0.0
    fcmpe\\t%<s>0, %<s>1"
  [(set_attr "v8type" "fcmp")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*cmp_swp_<shift>_reg<mode>"
  [(set (reg:CC_SWP CC_REGNUM)
	(compare:CC_SWP (ASHIFT:GPI
			 (match_operand:GPI 0 "register_operand" "r")
			 (match_operand:QI 1 "aarch64_shift_imm_<mode>" "n"))
			(match_operand:GPI 2 "aarch64_reg_or_zero" "rZ")))]
  ""
  "cmp\\t%<w>2, %<w>0, <shift> %1"
  [(set_attr "v8type" "alus_shift")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*cmp_swp_<optab><ALLX:mode>_reg<GPI:mode>"
  [(set (reg:CC_SWP CC_REGNUM)
	(compare:CC_SWP (ANY_EXTEND:GPI
			 (match_operand:ALLX 0 "register_operand" "r"))
			(match_operand:GPI 1 "register_operand" "r")))]
  ""
  "cmp\\t%<GPI:w>1, %<GPI:w>0, <su>xt<ALLX:size>"
  [(set_attr "v8type" "alus_ext")
   (set_attr "mode" "<GPI:MODE>")]
)


;; -------------------------------------------------------------------
;; Store-flag and conditional select insns
;; -------------------------------------------------------------------

(define_expand "cstore<mode>4"
  [(set (match_operand:SI 0 "register_operand" "")
	(match_operator:SI 1 "aarch64_comparison_operator"
	 [(match_operand:GPI 2 "register_operand" "")
	  (match_operand:GPI 3 "aarch64_plus_operand" "")]))]
  ""
  "
  operands[2] = aarch64_gen_compare_reg (GET_CODE (operands[1]), operands[2],
				      operands[3]);
  operands[3] = const0_rtx;
  "
)

(define_expand "cstore<mode>4"
  [(set (match_operand:SI 0 "register_operand" "")
	(match_operator:SI 1 "aarch64_comparison_operator"
	 [(match_operand:GPF 2 "register_operand" "")
	  (match_operand:GPF 3 "register_operand" "")]))]
  ""
  "
  operands[2] = aarch64_gen_compare_reg (GET_CODE (operands[1]), operands[2],
				      operands[3]);
  operands[3] = const0_rtx;
  "
)

(define_insn "*cstore<mode>_insn"
  [(set (match_operand:ALLI 0 "register_operand" "=r")
	(match_operator:ALLI 1 "aarch64_comparison_operator"
	 [(match_operand 2 "cc_register" "") (const_int 0)]))]
  ""
  "cset\\t%<w>0, %m1"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of the above
(define_insn "*cstoresi_insn_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
	 (match_operator:SI 1 "aarch64_comparison_operator"
	  [(match_operand 2 "cc_register" "") (const_int 0)])))]
  ""
  "cset\\t%w0, %m1"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "SI")]
)

(define_insn "cstore<mode>_neg"
  [(set (match_operand:ALLI 0 "register_operand" "=r")
	(neg:ALLI (match_operator:ALLI 1 "aarch64_comparison_operator"
		  [(match_operand 2 "cc_register" "") (const_int 0)])))]
  ""
  "csetm\\t%<w>0, %m1"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of the above
(define_insn "*cstoresi_neg_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
	 (neg:SI (match_operator:SI 1 "aarch64_comparison_operator"
		  [(match_operand 2 "cc_register" "") (const_int 0)]))))]
  ""
  "csetm\\t%w0, %m1"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "SI")]
)

(define_expand "cmov<mode>6"
  [(set (match_operand:GPI 0 "register_operand" "")
	(if_then_else:GPI
	 (match_operator 1 "aarch64_comparison_operator"
	  [(match_operand:GPI 2 "register_operand" "")
	   (match_operand:GPI 3 "aarch64_plus_operand" "")])
	 (match_operand:GPI 4 "register_operand" "")
	 (match_operand:GPI 5 "register_operand" "")))]
  ""
  "
  operands[2] = aarch64_gen_compare_reg (GET_CODE (operands[1]), operands[2],
				      operands[3]);
  operands[3] = const0_rtx;
  "
)

(define_expand "cmov<mode>6"
  [(set (match_operand:GPF 0 "register_operand" "")
	(if_then_else:GPF
	 (match_operator 1 "aarch64_comparison_operator"
	  [(match_operand:GPF 2 "register_operand" "")
	   (match_operand:GPF 3 "register_operand" "")])
	 (match_operand:GPF 4 "register_operand" "")
	 (match_operand:GPF 5 "register_operand" "")))]
  ""
  "
  operands[2] = aarch64_gen_compare_reg (GET_CODE (operands[1]), operands[2],
				      operands[3]);
  operands[3] = const0_rtx;
  "
)

(define_insn "*cmov<mode>_insn"
  [(set (match_operand:ALLI 0 "register_operand" "=r,r,r,r,r,r,r")
	(if_then_else:ALLI
	 (match_operator 1 "aarch64_comparison_operator"
	  [(match_operand 2 "cc_register" "") (const_int 0)])
	 (match_operand:ALLI 3 "aarch64_reg_zero_or_m1_or_1" "rZ,rZ,UsM,rZ,Ui1,UsM,Ui1")
	 (match_operand:ALLI 4 "aarch64_reg_zero_or_m1_or_1" "rZ,UsM,rZ,Ui1,rZ,UsM,Ui1")))]
  "!((operands[3] == const1_rtx && operands[4] == constm1_rtx)
     || (operands[3] == constm1_rtx && operands[4] == const1_rtx))"
  ;; Final two alternatives should be unreachable, but included for completeness
  "@
   csel\\t%<w>0, %<w>3, %<w>4, %m1
   csinv\\t%<w>0, %<w>3, <w>zr, %m1
   csinv\\t%<w>0, %<w>4, <w>zr, %M1
   csinc\\t%<w>0, %<w>3, <w>zr, %m1
   csinc\\t%<w>0, %<w>4, <w>zr, %M1
   mov\\t%<w>0, -1
   mov\\t%<w>0, 1"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*cmovsi_insn_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r,r,r,r")
	(zero_extend:DI
	 (if_then_else:SI
	  (match_operator 1 "aarch64_comparison_operator"
	   [(match_operand 2 "cc_register" "") (const_int 0)])
	  (match_operand:SI 3 "aarch64_reg_zero_or_m1_or_1" "rZ,rZ,UsM,rZ,Ui1,UsM,Ui1")
	  (match_operand:SI 4 "aarch64_reg_zero_or_m1_or_1" "rZ,UsM,rZ,Ui1,rZ,UsM,Ui1"))))]
  "!((operands[3] == const1_rtx && operands[4] == constm1_rtx)
     || (operands[3] == constm1_rtx && operands[4] == const1_rtx))"
  ;; Final two alternatives should be unreachable, but included for completeness
  "@
   csel\\t%w0, %w3, %w4, %m1
   csinv\\t%w0, %w3, wzr, %m1
   csinv\\t%w0, %w4, wzr, %M1
   csinc\\t%w0, %w3, wzr, %m1
   csinc\\t%w0, %w4, wzr, %M1
   mov\\t%w0, -1
   mov\\t%w0, 1"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "SI")]
)

(define_insn "*cmov<mode>_insn"
  [(set (match_operand:GPF 0 "register_operand" "=w")
	(if_then_else:GPF
	 (match_operator 1 "aarch64_comparison_operator"
	  [(match_operand 2 "cc_register" "") (const_int 0)])
	 (match_operand:GPF 3 "register_operand" "w")
	 (match_operand:GPF 4 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fcsel\\t%<s>0, %<s>3, %<s>4, %m1"
  [(set_attr "v8type" "fcsel")
   (set_attr "mode" "<MODE>")]
)

(define_expand "mov<mode>cc"
  [(set (match_operand:ALLI 0 "register_operand" "")
	(if_then_else:ALLI (match_operand 1 "aarch64_comparison_operator" "")
			   (match_operand:ALLI 2 "register_operand" "")
			   (match_operand:ALLI 3 "register_operand" "")))]
  ""
  {
    rtx ccreg;
    enum rtx_code code = GET_CODE (operands[1]);

    if (code == UNEQ || code == LTGT)
      FAIL;

    ccreg = aarch64_gen_compare_reg (code, XEXP (operands[1], 0),
				  XEXP (operands[1], 1));
    operands[1] = gen_rtx_fmt_ee (code, VOIDmode, ccreg, const0_rtx);
  }
)

(define_expand "mov<GPF:mode><GPI:mode>cc"
  [(set (match_operand:GPI 0 "register_operand" "")
	(if_then_else:GPI (match_operand 1 "aarch64_comparison_operator" "")
			  (match_operand:GPF 2 "register_operand" "")
			  (match_operand:GPF 3 "register_operand" "")))]
  ""
  {
    rtx ccreg;
    enum rtx_code code = GET_CODE (operands[1]);

    if (code == UNEQ || code == LTGT)
      FAIL;

    ccreg = aarch64_gen_compare_reg (code, XEXP (operands[1], 0),
				  XEXP (operands[1], 1));
    operands[1] = gen_rtx_fmt_ee (code, VOIDmode, ccreg, const0_rtx);
  }
)

(define_insn "*csinc2<mode>_insn"
  [(set (match_operand:GPI 0 "register_operand" "=r")
        (plus:GPI (match_operator:GPI 2 "aarch64_comparison_operator"
		  [(match_operand:CC 3 "cc_register" "") (const_int 0)])
		 (match_operand:GPI 1 "register_operand" "r")))]
  ""
  "csinc\\t%<w>0, %<w>1, %<w>1, %M2"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "<MODE>")])

(define_insn "csinc3<mode>_insn"
  [(set (match_operand:GPI 0 "register_operand" "=r")
        (if_then_else:GPI
	  (match_operator:GPI 1 "aarch64_comparison_operator"
	   [(match_operand:CC 2 "cc_register" "") (const_int 0)])
	  (plus:GPI (match_operand:GPI 3 "register_operand" "r")
		    (const_int 1))
	  (match_operand:GPI 4 "aarch64_reg_or_zero" "rZ")))]
  ""
  "csinc\\t%<w>0, %<w>4, %<w>3, %M1"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*csinv3<mode>_insn"
  [(set (match_operand:GPI 0 "register_operand" "=r")
        (if_then_else:GPI
	  (match_operator:GPI 1 "aarch64_comparison_operator"
	   [(match_operand:CC 2 "cc_register" "") (const_int 0)])
	  (not:GPI (match_operand:GPI 3 "register_operand" "r"))
	  (match_operand:GPI 4 "aarch64_reg_or_zero" "rZ")))]
  ""
  "csinv\\t%<w>0, %<w>4, %<w>3, %M1"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "<MODE>")])

(define_insn "*csneg3<mode>_insn"
  [(set (match_operand:GPI 0 "register_operand" "=r")
        (if_then_else:GPI
	  (match_operator:GPI 1 "aarch64_comparison_operator"
	   [(match_operand:CC 2 "cc_register" "") (const_int 0)])
	  (neg:GPI (match_operand:GPI 3 "register_operand" "r"))
	  (match_operand:GPI 4 "aarch64_reg_or_zero" "rZ")))]
  ""
  "csneg\\t%<w>0, %<w>4, %<w>3, %M1"
  [(set_attr "v8type" "csel")
   (set_attr "mode" "<MODE>")])

;; -------------------------------------------------------------------
;; Logical operations
;; -------------------------------------------------------------------

(define_insn "<optab><mode>3"
  [(set (match_operand:GPI 0 "register_operand" "=r,rk")
	(LOGICAL:GPI (match_operand:GPI 1 "register_operand" "%r,r")
		     (match_operand:GPI 2 "aarch64_logical_operand" "r,<lconst>")))]
  ""
  "<logical>\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "logic,logic_imm")
   (set_attr "mode" "<MODE>")])

;; zero_extend version of above
(define_insn "*<optab>si3_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r,rk")
	(zero_extend:DI
         (LOGICAL:SI (match_operand:SI 1 "register_operand" "%r,r")
		     (match_operand:SI 2 "aarch64_logical_operand" "r,K"))))]
  ""
  "<logical>\\t%w0, %w1, %w2"
  [(set_attr "v8type" "logic,logic_imm")
   (set_attr "mode" "SI")])

(define_insn "*<LOGICAL:optab>_<SHIFT:optab><mode>3"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(LOGICAL:GPI (SHIFT:GPI
		      (match_operand:GPI 1 "register_operand" "r")
		      (match_operand:QI 2 "aarch64_shift_imm_<mode>" "n"))
		     (match_operand:GPI 3 "register_operand" "r")))]
  ""
  "<LOGICAL:logical>\\t%<w>0, %<w>3, %<w>1, <SHIFT:shift> %2"
  [(set_attr "v8type" "logic_shift")
   (set_attr "mode" "<MODE>")])

;; zero_extend version of above
(define_insn "*<LOGICAL:optab>_<SHIFT:optab>si3_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
	 (LOGICAL:SI (SHIFT:SI
		      (match_operand:SI 1 "register_operand" "r")
		      (match_operand:QI 2 "aarch64_shift_imm_si" "n"))
		     (match_operand:SI 3 "register_operand" "r"))))]
  ""
  "<LOGICAL:logical>\\t%w0, %w3, %w1, <SHIFT:shift> %2"
  [(set_attr "v8type" "logic_shift")
   (set_attr "mode" "SI")])

(define_insn "one_cmpl<mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(not:GPI (match_operand:GPI 1 "register_operand" "r")))]
  ""
  "mvn\\t%<w>0, %<w>1"
  [(set_attr "v8type" "logic")
   (set_attr "mode" "<MODE>")])

(define_insn "*one_cmpl_<optab><mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(not:GPI (SHIFT:GPI (match_operand:GPI 1 "register_operand" "r")
			    (match_operand:QI 2 "aarch64_shift_imm_<mode>" "n"))))]
  ""
  "mvn\\t%<w>0, %<w>1, <shift> %2"
  [(set_attr "v8type" "logic_shift")
   (set_attr "mode" "<MODE>")])

(define_insn "*<LOGICAL:optab>_one_cmpl<mode>3"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(LOGICAL:GPI (not:GPI
		      (match_operand:GPI 1 "register_operand" "r"))
		     (match_operand:GPI 2 "register_operand" "r")))]
  ""
  "<LOGICAL:nlogical>\\t%<w>0, %<w>2, %<w>1"
  [(set_attr "v8type" "logic")
   (set_attr "mode" "<MODE>")])

(define_insn "*<LOGICAL:optab>_one_cmpl_<SHIFT:optab><mode>3"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(LOGICAL:GPI (not:GPI
		      (SHIFT:GPI
		       (match_operand:GPI 1 "register_operand" "r")
		       (match_operand:QI 2 "aarch64_shift_imm_<mode>" "n")))
		     (match_operand:GPI 3 "register_operand" "r")))]
  ""
  "<LOGICAL:nlogical>\\t%<w>0, %<w>3, %<w>1, <SHIFT:shift> %2"
  [(set_attr "v8type" "logic_shift")
   (set_attr "mode" "<MODE>")])

(define_insn "clz<mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(clz:GPI (match_operand:GPI 1 "register_operand" "r")))]
  ""
  "clz\\t%<w>0, %<w>1"
  [(set_attr "v8type" "clz")
   (set_attr "mode" "<MODE>")])

(define_expand "ffs<mode>2"
  [(match_operand:GPI 0 "register_operand")
   (match_operand:GPI 1 "register_operand")]
  ""
  {
    rtx ccreg = aarch64_gen_compare_reg (EQ, operands[1], const0_rtx);
    rtx x = gen_rtx_NE (VOIDmode, ccreg, const0_rtx);

    emit_insn (gen_rbit<mode>2 (operands[0], operands[1]));
    emit_insn (gen_clz<mode>2 (operands[0], operands[0]));
    emit_insn (gen_csinc3<mode>_insn (operands[0], x, ccreg, operands[0], const0_rtx));
    DONE;
  }
)

(define_insn "clrsb<mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(unspec:GPI [(match_operand:GPI 1 "register_operand" "r")] UNSPEC_CLS))]
  ""
  "cls\\t%<w>0, %<w>1"
  [(set_attr "v8type" "clz")
   (set_attr "mode" "<MODE>")])

(define_insn "rbit<mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(unspec:GPI [(match_operand:GPI 1 "register_operand" "r")] UNSPEC_RBIT))]
  ""
  "rbit\\t%<w>0, %<w>1"
  [(set_attr "v8type" "rbit")
   (set_attr "mode" "<MODE>")])

(define_expand "ctz<mode>2"
  [(match_operand:GPI 0 "register_operand")
   (match_operand:GPI 1 "register_operand")]
  ""
  {
    emit_insn (gen_rbit<mode>2 (operands[0], operands[1]));
    emit_insn (gen_clz<mode>2 (operands[0], operands[0]));
    DONE;
  }
)

(define_insn "*and<mode>3nr_compare0"
  [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ
	 (and:GPI (match_operand:GPI 0 "register_operand" "%r,r")
		  (match_operand:GPI 1 "aarch64_logical_operand" "r,<lconst>"))
	 (const_int 0)))]
  ""
  "tst\\t%<w>0, %<w>1"
  [(set_attr "v8type" "logics")
   (set_attr "mode" "<MODE>")])

(define_insn "*and_<SHIFT:optab><mode>3nr_compare0"
  [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ
	 (and:GPI (SHIFT:GPI
		   (match_operand:GPI 0 "register_operand" "r")
		   (match_operand:QI 1 "aarch64_shift_imm_<mode>" "n"))
		  (match_operand:GPI 2 "register_operand" "r"))
	(const_int 0)))]
  ""
  "tst\\t%<w>2, %<w>0, <SHIFT:shift> %1"
  [(set_attr "v8type" "logics_shift")
   (set_attr "mode" "<MODE>")])

;; -------------------------------------------------------------------
;; Shifts
;; -------------------------------------------------------------------

(define_expand "<optab><mode>3"
  [(set (match_operand:GPI 0 "register_operand")
	(ASHIFT:GPI (match_operand:GPI 1 "register_operand")
		    (match_operand:QI 2 "nonmemory_operand")))]
  ""
  {
    if (CONST_INT_P (operands[2]))
      {
        operands[2] = GEN_INT (INTVAL (operands[2])
                               & (GET_MODE_BITSIZE (<MODE>mode) - 1));

        if (operands[2] == const0_rtx)
          {
	    emit_insn (gen_mov<mode> (operands[0], operands[1]));
	    DONE;
          }
      }
  }
)

(define_expand "ashl<mode>3"
  [(set (match_operand:SHORT 0 "register_operand")
	(ashift:SHORT (match_operand:SHORT 1 "register_operand")
		      (match_operand:QI 2 "nonmemory_operand")))]
  ""
  {
    if (CONST_INT_P (operands[2]))
      {
        operands[2] = GEN_INT (INTVAL (operands[2])
                               & (GET_MODE_BITSIZE (<MODE>mode) - 1));

        if (operands[2] == const0_rtx)
          {
	    emit_insn (gen_mov<mode> (operands[0], operands[1]));
	    DONE;
          }
      }
  }
)

(define_expand "rotr<mode>3"
  [(set (match_operand:GPI 0 "register_operand")
	(rotatert:GPI (match_operand:GPI 1 "register_operand")
		      (match_operand:QI 2 "nonmemory_operand")))]
  ""
  {
    if (CONST_INT_P (operands[2]))
      {
        operands[2] = GEN_INT (INTVAL (operands[2])
                               & (GET_MODE_BITSIZE (<MODE>mode) - 1));

        if (operands[2] == const0_rtx)
          {
	    emit_insn (gen_mov<mode> (operands[0], operands[1]));
	    DONE;
          }
      }
  }
)

(define_expand "rotl<mode>3"
  [(set (match_operand:GPI 0 "register_operand")
	(rotatert:GPI (match_operand:GPI 1 "register_operand")
		      (match_operand:QI 2 "nonmemory_operand")))]
  ""
  {
    /* (SZ - cnt) % SZ == -cnt % SZ */
    if (CONST_INT_P (operands[2]))
      {
        operands[2] = GEN_INT ((-INTVAL (operands[2]))
			       & (GET_MODE_BITSIZE (<MODE>mode) - 1));
        if (operands[2] == const0_rtx)
          {
	    emit_insn (gen_mov<mode> (operands[0], operands[1]));
	    DONE;
          }
      }
    else
      operands[2] = expand_simple_unop (QImode, NEG, operands[2],
					NULL_RTX, 1);
  }
)

(define_insn "*<optab><mode>3_insn"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(SHIFT:GPI
	 (match_operand:GPI 1 "register_operand" "r")
	 (match_operand:QI 2 "aarch64_reg_or_shift_imm_<mode>" "rUs<cmode>")))]
  ""
  "<shift>\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "shift")
   (set_attr "mode" "<MODE>")]
)

;; zero_extend version of above
(define_insn "*<optab>si3_insn_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI (SHIFT:SI
	 (match_operand:SI 1 "register_operand" "r")
	 (match_operand:QI 2 "aarch64_reg_or_shift_imm_si" "rUss"))))]
  ""
  "<shift>\\t%w0, %w1, %w2"
  [(set_attr "v8type" "shift")
   (set_attr "mode" "SI")]
)

(define_insn "*ashl<mode>3_insn"
  [(set (match_operand:SHORT 0 "register_operand" "=r")
	(ashift:SHORT (match_operand:SHORT 1 "register_operand" "r")
		      (match_operand:QI 2 "aarch64_reg_or_shift_imm_si" "rUss")))]
  ""
  "lsl\\t%<w>0, %<w>1, %<w>2"
  [(set_attr "v8type" "shift")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*<optab><mode>3_insn"
  [(set (match_operand:SHORT 0 "register_operand" "=r")
	(ASHIFT:SHORT (match_operand:SHORT 1 "register_operand" "r")
		      (match_operand 2 "const_int_operand" "n")))]
  "UINTVAL (operands[2]) < GET_MODE_BITSIZE (<MODE>mode)"
{
  operands[3] = GEN_INT (<sizen> - UINTVAL (operands[2]));
  return "<bfshift>\t%w0, %w1, %2, %3";
}
  [(set_attr "v8type" "bfm")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*<ANY_EXTEND:optab><GPI:mode>_ashl<SHORT:mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(ANY_EXTEND:GPI
	 (ashift:SHORT (match_operand:SHORT 1 "register_operand" "r")
		       (match_operand 2 "const_int_operand" "n"))))]
  "UINTVAL (operands[2]) < GET_MODE_BITSIZE (<SHORT:MODE>mode)"
{
  operands[3] = GEN_INT (<SHORT:sizen> - UINTVAL (operands[2]));
  return "<su>bfiz\t%<GPI:w>0, %<GPI:w>1, %2, %3";
}
  [(set_attr "v8type" "bfm")
   (set_attr "mode" "<GPI:MODE>")]
)

(define_insn "*zero_extend<GPI:mode>_lshr<SHORT:mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(zero_extend:GPI
	 (lshiftrt:SHORT (match_operand:SHORT 1 "register_operand" "r")
			 (match_operand 2 "const_int_operand" "n"))))]
  "UINTVAL (operands[2]) < GET_MODE_BITSIZE (<SHORT:MODE>mode)"
{
  operands[3] = GEN_INT (<SHORT:sizen> - UINTVAL (operands[2]));
  return "ubfx\t%<GPI:w>0, %<GPI:w>1, %2, %3";
}
  [(set_attr "v8type" "bfm")
   (set_attr "mode" "<GPI:MODE>")]
)

(define_insn "*extend<GPI:mode>_ashr<SHORT:mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(sign_extend:GPI
	 (ashiftrt:SHORT (match_operand:SHORT 1 "register_operand" "r")
			 (match_operand 2 "const_int_operand" "n"))))]
  "UINTVAL (operands[2]) < GET_MODE_BITSIZE (<SHORT:MODE>mode)"
{
  operands[3] = GEN_INT (<SHORT:sizen> - UINTVAL (operands[2]));
  return "sbfx\\t%<GPI:w>0, %<GPI:w>1, %2, %3";
}
  [(set_attr "v8type" "bfm")
   (set_attr "mode" "<GPI:MODE>")]
)

;; -------------------------------------------------------------------
;; Bitfields
;; -------------------------------------------------------------------

(define_expand "<optab>"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(ANY_EXTRACT:DI (match_operand:DI 1 "register_operand" "r")
			(match_operand 2 "const_int_operand" "n")
			(match_operand 3 "const_int_operand" "n")))]
  ""
  ""
)

(define_insn "*<optab><mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(ANY_EXTRACT:GPI (match_operand:GPI 1 "register_operand" "r")
			 (match_operand 2 "const_int_operand" "n")
			 (match_operand 3 "const_int_operand" "n")))]
  ""
  "<su>bfx\\t%<w>0, %<w>1, %3, %2"
  [(set_attr "v8type" "bfm")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*<optab><ALLX:mode>_shft_<GPI:mode>"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(ashift:GPI (ANY_EXTEND:GPI
		     (match_operand:ALLX 1 "register_operand" "r"))
		    (match_operand 2 "const_int_operand" "n")))]
  "UINTVAL (operands[2]) < <GPI:sizen>"
{
  operands[3] = (<ALLX:sizen> <= (<GPI:sizen> - UINTVAL (operands[2])))
	      ? GEN_INT (<ALLX:sizen>)
	      : GEN_INT (<GPI:sizen> - UINTVAL (operands[2]));
  return "<su>bfiz\t%<GPI:w>0, %<GPI:w>1, %2, %3";
}
  [(set_attr "v8type" "bfm")
   (set_attr "mode" "<GPI:MODE>")]
)

;; XXX We should match (any_extend (ashift)) here, like (and (ashift)) below

(define_insn "*andim_ashift<mode>_bfiz"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(and:GPI (ashift:GPI (match_operand:GPI 1 "register_operand" "r")
			     (match_operand 2 "const_int_operand" "n"))
		 (match_operand 3 "const_int_operand" "n")))]
  "exact_log2 ((INTVAL (operands[3]) >> INTVAL (operands[2])) + 1) >= 0
   && (INTVAL (operands[3]) & ((1 << INTVAL (operands[2])) - 1)) == 0"
  "ubfiz\\t%<w>0, %<w>1, %2, %P3"
  [(set_attr "v8type" "bfm")
   (set_attr "mode" "<MODE>")]
)

(define_insn "bswap<mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
        (bswap:GPI (match_operand:GPI 1 "register_operand" "r")))]
  ""
  "rev\\t%<w>0, %<w>1"
  [(set_attr "v8type" "rev")
   (set_attr "mode" "<MODE>")]
)

(define_insn "bswaphi2"
  [(set (match_operand:HI 0 "register_operand" "=r")
        (bswap:HI (match_operand:HI 1 "register_operand" "r")))]
  ""
  "rev16\\t%w0, %w1"
  [(set_attr "v8type" "rev")
   (set_attr "mode" "HI")]
)

;; zero_extend version of above
(define_insn "*bswapsi2_uxtw"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (zero_extend:DI (bswap:SI (match_operand:SI 1 "register_operand" "r"))))]
  ""
  "rev\\t%w0, %w1"
  [(set_attr "v8type" "rev")
   (set_attr "mode" "SI")]
)

;; -------------------------------------------------------------------
;; Floating-point intrinsics
;; -------------------------------------------------------------------

;; frint floating-point round to integral standard patterns.
;; Expands to btrunc, ceil, floor, nearbyint, rint, round.

(define_insn "<frint_pattern><mode>2"
  [(set (match_operand:GPF 0 "register_operand" "=w")
	(unspec:GPF [(match_operand:GPF 1 "register_operand" "w")]
	 FRINT))]
  "TARGET_FLOAT"
  "frint<frint_suffix>\\t%<s>0, %<s>1"
  [(set_attr "v8type" "frint")
   (set_attr "mode" "<MODE>")]
)

;; frcvt floating-point round to integer and convert standard patterns.
;; Expands to lbtrunc, lceil, lfloor, lround.
(define_insn "l<fcvt_pattern><su_optab><GPF:mode><GPI:mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
	(FIXUORS:GPI (unspec:GPF [(match_operand:GPF 1 "register_operand" "w")]
		      FCVT)))]
  "TARGET_FLOAT"
  "fcvt<frint_suffix><su>\\t%<GPI:w>0, %<GPF:s>1"
  [(set_attr "v8type" "fcvtf2i")
   (set_attr "mode" "<GPF:MODE>")
   (set_attr "mode2" "<GPI:MODE>")]
)

;; fma - no throw

(define_insn "fma<mode>4"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (fma:GPF (match_operand:GPF 1 "register_operand" "w")
		 (match_operand:GPF 2 "register_operand" "w")
		 (match_operand:GPF 3 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fmadd\\t%<s>0, %<s>1, %<s>2, %<s>3"
  [(set_attr "v8type" "fmadd")
   (set_attr "mode" "<MODE>")]
)

(define_insn "fnma<mode>4"
  [(set (match_operand:GPF 0 "register_operand" "=w")
	(fma:GPF (neg:GPF (match_operand:GPF 1 "register_operand" "w"))
		 (match_operand:GPF 2 "register_operand" "w")
		 (match_operand:GPF 3 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fmsub\\t%<s>0, %<s>1, %<s>2, %<s>3"
  [(set_attr "v8type" "fmadd")
   (set_attr "mode" "<MODE>")]
)

(define_insn "fms<mode>4"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (fma:GPF (match_operand:GPF 1 "register_operand" "w")
		 (match_operand:GPF 2 "register_operand" "w")
		 (neg:GPF (match_operand:GPF 3 "register_operand" "w"))))]
  "TARGET_FLOAT"
  "fnmsub\\t%<s>0, %<s>1, %<s>2, %<s>3"
  [(set_attr "v8type" "fmadd")
   (set_attr "mode" "<MODE>")]
)

(define_insn "fnms<mode>4"
  [(set (match_operand:GPF 0 "register_operand" "=w")
	(fma:GPF (neg:GPF (match_operand:GPF 1 "register_operand" "w"))
		 (match_operand:GPF 2 "register_operand" "w")
		 (neg:GPF (match_operand:GPF 3 "register_operand" "w"))))]
  "TARGET_FLOAT"
  "fnmadd\\t%<s>0, %<s>1, %<s>2, %<s>3"
  [(set_attr "v8type" "fmadd")
   (set_attr "mode" "<MODE>")]
)

;; If signed zeros are ignored, -(a * b + c) = -a * b - c.
(define_insn "*fnmadd<mode>4"
  [(set (match_operand:GPF 0 "register_operand" "=w")
	(neg:GPF (fma:GPF (match_operand:GPF 1 "register_operand" "w")
			  (match_operand:GPF 2 "register_operand" "w")
			  (match_operand:GPF 3 "register_operand" "w"))))]
  "!HONOR_SIGNED_ZEROS (<MODE>mode) && TARGET_FLOAT"
  "fnmadd\\t%<s>0, %<s>1, %<s>2, %<s>3"
  [(set_attr "v8type" "fmadd")
   (set_attr "mode" "<MODE>")]
)

;; -------------------------------------------------------------------
;; Floating-point conversions
;; -------------------------------------------------------------------

(define_insn "extendsfdf2"
  [(set (match_operand:DF 0 "register_operand" "=w")
        (float_extend:DF (match_operand:SF 1 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fcvt\\t%d0, %s1"
  [(set_attr "v8type" "fcvt")
   (set_attr "mode" "DF")
   (set_attr "mode2" "SF")]
)

(define_insn "truncdfsf2"
  [(set (match_operand:SF 0 "register_operand" "=w")
        (float_truncate:SF (match_operand:DF 1 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fcvt\\t%s0, %d1"
  [(set_attr "v8type" "fcvt")
   (set_attr "mode" "SF")
   (set_attr "mode2" "DF")]
)

(define_insn "fix_trunc<GPF:mode><GPI:mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
        (fix:GPI (match_operand:GPF 1 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fcvtzs\\t%<GPI:w>0, %<GPF:s>1"
  [(set_attr "v8type" "fcvtf2i")
   (set_attr "mode" "<GPF:MODE>")
   (set_attr "mode2" "<GPI:MODE>")]
)

(define_insn "fixuns_trunc<GPF:mode><GPI:mode>2"
  [(set (match_operand:GPI 0 "register_operand" "=r")
        (unsigned_fix:GPI (match_operand:GPF 1 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fcvtzu\\t%<GPI:w>0, %<GPF:s>1"
  [(set_attr "v8type" "fcvtf2i")
   (set_attr "mode" "<GPF:MODE>")
   (set_attr "mode2" "<GPI:MODE>")]
)

(define_insn "float<GPI:mode><GPF:mode>2"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (float:GPF (match_operand:GPI 1 "register_operand" "r")))]
  "TARGET_FLOAT"
  "scvtf\\t%<GPF:s>0, %<GPI:w>1"
  [(set_attr "v8type" "fcvti2f")
   (set_attr "mode" "<GPF:MODE>")
   (set_attr "mode2" "<GPI:MODE>")]
)

(define_insn "floatuns<GPI:mode><GPF:mode>2"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (unsigned_float:GPF (match_operand:GPI 1 "register_operand" "r")))]
  "TARGET_FLOAT"
  "ucvtf\\t%<GPF:s>0, %<GPI:w>1"
  [(set_attr "v8type" "fcvt")
   (set_attr "mode" "<GPF:MODE>")
   (set_attr "mode2" "<GPI:MODE>")]
)

;; -------------------------------------------------------------------
;; Floating-point arithmetic
;; -------------------------------------------------------------------

(define_insn "add<mode>3"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (plus:GPF
         (match_operand:GPF 1 "register_operand" "w")
         (match_operand:GPF 2 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fadd\\t%<s>0, %<s>1, %<s>2"
  [(set_attr "v8type" "fadd")
   (set_attr "mode" "<MODE>")]
)

(define_insn "sub<mode>3"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (minus:GPF
         (match_operand:GPF 1 "register_operand" "w")
         (match_operand:GPF 2 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fsub\\t%<s>0, %<s>1, %<s>2"
  [(set_attr "v8type" "fadd")
   (set_attr "mode" "<MODE>")]
)

(define_insn "mul<mode>3"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (mult:GPF
         (match_operand:GPF 1 "register_operand" "w")
         (match_operand:GPF 2 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fmul\\t%<s>0, %<s>1, %<s>2"
  [(set_attr "v8type" "fmul")
   (set_attr "mode" "<MODE>")]
)

(define_insn "*fnmul<mode>3"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (mult:GPF
		 (neg:GPF (match_operand:GPF 1 "register_operand" "w"))
		 (match_operand:GPF 2 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fnmul\\t%<s>0, %<s>1, %<s>2"
  [(set_attr "v8type" "fmul")
   (set_attr "mode" "<MODE>")]
)

(define_insn "div<mode>3"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (div:GPF
         (match_operand:GPF 1 "register_operand" "w")
         (match_operand:GPF 2 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fdiv\\t%<s>0, %<s>1, %<s>2"
  [(set_attr "v8type" "fdiv")
   (set_attr "mode" "<MODE>")]
)

(define_insn "neg<mode>2"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (neg:GPF (match_operand:GPF 1 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fneg\\t%<s>0, %<s>1"
  [(set_attr "v8type" "ffarith")
   (set_attr "mode" "<MODE>")]
)

(define_insn "sqrt<mode>2"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (sqrt:GPF (match_operand:GPF 1 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fsqrt\\t%<s>0, %<s>1"
  [(set_attr "v8type" "fsqrt")
   (set_attr "mode" "<MODE>")]
)

(define_insn "abs<mode>2"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (abs:GPF (match_operand:GPF 1 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fabs\\t%<s>0, %<s>1"
  [(set_attr "v8type" "ffarith")
   (set_attr "mode" "<MODE>")]
)

;; Given that smax/smin do not specify the result when either input is NaN,
;; we could use either FMAXNM or FMAX for smax, and either FMINNM or FMIN
;; for smin.

(define_insn "smax<mode>3"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (smax:GPF (match_operand:GPF 1 "register_operand" "w")
		  (match_operand:GPF 2 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fmaxnm\\t%<s>0, %<s>1, %<s>2"
  [(set_attr "v8type" "fminmax")
   (set_attr "mode" "<MODE>")]
)

(define_insn "smin<mode>3"
  [(set (match_operand:GPF 0 "register_operand" "=w")
        (smin:GPF (match_operand:GPF 1 "register_operand" "w")
		  (match_operand:GPF 2 "register_operand" "w")))]
  "TARGET_FLOAT"
  "fminnm\\t%<s>0, %<s>1, %<s>2"
  [(set_attr "v8type" "fminmax")
   (set_attr "mode" "<MODE>")]
)

;; -------------------------------------------------------------------
;; Reload support
;; -------------------------------------------------------------------

;; Reload SP+imm where imm cannot be handled by a single ADD instruction.  
;; Must load imm into a scratch register and copy SP to the dest reg before
;; adding, since SP cannot be used as a source register in an ADD
;; instruction.
(define_expand "reload_sp_immediate"
  [(parallel [(set (match_operand:DI 0 "register_operand" "=r")
		   (match_operand:DI 1 "" ""))
	     (clobber (match_operand:TI 2 "register_operand" "=&r"))])]
  ""
  {
    rtx sp = XEXP (operands[1], 0);
    rtx val = XEXP (operands[1], 1);
    unsigned regno = REGNO (operands[2]);
    rtx scratch = operands[1];
    gcc_assert (GET_CODE (operands[1]) == PLUS);
    gcc_assert (sp == stack_pointer_rtx);
    gcc_assert (CONST_INT_P (val));

    /* It is possible that one of the registers we got for operands[2]
       might coincide with that of operands[0] (which is why we made
       it TImode).  Pick the other one to use as our scratch.  */
    if (regno == REGNO (operands[0]))
      regno++;
    scratch = gen_rtx_REG (DImode, regno);

    emit_move_insn (scratch, val);
    emit_move_insn (operands[0], sp);
    emit_insn (gen_adddi3 (operands[0], operands[0], scratch));
    DONE;
  }
)

(define_expand "aarch64_reload_mov<mode>"
  [(set (match_operand:TX 0 "register_operand" "=w")
        (match_operand:TX 1 "register_operand" "w"))
   (clobber (match_operand:DI 2 "register_operand" "=&r"))
  ]
  ""
  {
    rtx op0 = simplify_gen_subreg (TImode, operands[0], <MODE>mode, 0);
    rtx op1 = simplify_gen_subreg (TImode, operands[1], <MODE>mode, 0);
    gen_aarch64_movtilow_tilow (op0, op1);
    gen_aarch64_movdi_tihigh (operands[2], op1);
    gen_aarch64_movtihigh_di (op0, operands[2]);
    DONE;
  }
)

;; The following secondary reload helpers patterns are invoked
;; after or during reload as we don't want these patterns to start
;; kicking in during the combiner.
 
(define_insn "aarch64_movdi_tilow"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (truncate:DI (match_operand:TI 1 "register_operand" "w")))]
  "reload_completed || reload_in_progress"
  "fmov\\t%x0, %d1"
  [(set_attr "v8type" "fmovf2i")
   (set_attr "mode"   "DI")
   (set_attr "length" "4")
  ])

(define_insn "aarch64_movdi_tihigh"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (truncate:DI
	  (lshiftrt:TI (match_operand:TI 1 "register_operand" "w")
		       (const_int 64))))]
  "reload_completed || reload_in_progress"
  "fmov\\t%x0, %1.d[1]"
  [(set_attr "v8type" "fmovf2i")
   (set_attr "mode"   "DI")
   (set_attr "length" "4")
  ])

(define_insn "aarch64_movtihigh_di"
  [(set (zero_extract:TI (match_operand:TI 0 "register_operand" "+w")
                         (const_int 64) (const_int 64))
        (zero_extend:TI (match_operand:DI 1 "register_operand" "r")))]
  "reload_completed || reload_in_progress"
  "fmov\\t%0.d[1], %x1"

  [(set_attr "v8type" "fmovi2f")
   (set_attr "mode"   "DI")
   (set_attr "length" "4")
  ])

(define_insn "aarch64_movtilow_di"
  [(set (match_operand:TI 0 "register_operand" "=w")
        (zero_extend:TI (match_operand:DI 1 "register_operand" "r")))]
  "reload_completed || reload_in_progress"
  "fmov\\t%d0, %x1"

  [(set_attr "v8type" "fmovi2f")
   (set_attr "mode"   "DI")
   (set_attr "length" "4")
  ])

(define_insn "aarch64_movtilow_tilow"
  [(set (match_operand:TI 0 "register_operand" "=w")
        (zero_extend:TI 
	  (truncate:DI (match_operand:TI 1 "register_operand" "w"))))]
  "reload_completed || reload_in_progress"
  "fmov\\t%d0, %d1"

  [(set_attr "v8type" "fmovi2f")
   (set_attr "mode"   "DI")
   (set_attr "length" "4")
  ])

;; There is a deliberate reason why the parameters of high and lo_sum's
;; don't have modes for ADRP and ADD instructions.  This is to allow high
;; and lo_sum's to be used with the labels defining the jump tables in
;; rodata section.

(define_insn "add_losym"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(lo_sum:DI (match_operand:DI 1 "register_operand" "r")
		   (match_operand 2 "aarch64_valid_symref" "S")))]
  ""
  "add\\t%0, %1, :lo12:%a2"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "DI")]

)

(define_insn "ldr_got_small"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(unspec:DI [(mem:DI (lo_sum:DI
			      (match_operand:DI 1 "register_operand" "r")
			      (match_operand:DI 2 "aarch64_valid_symref" "S")))]
		   UNSPEC_GOTSMALLPIC))]
  ""
  "ldr\\t%0, [%1, #:got_lo12:%a2]"
  [(set_attr "v8type" "load1")
   (set_attr "mode" "DI")]
)

(define_insn "aarch64_load_tp_hard"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(unspec:DI [(const_int 0)] UNSPEC_TLS))]
  ""
  "mrs\\t%0, tpidr_el0"
  [(set_attr "v8type" "mrs")
   (set_attr "mode" "DI")]
)

;; The TLS ABI specifically requires that the compiler does not schedule
;; instructions in the TLS stubs, in order to enable linker relaxation.
;; Therefore we treat the stubs as an atomic sequence.
(define_expand "tlsgd_small"
 [(parallel [(set (match_operand 0 "register_operand" "")
                  (call (mem:DI (match_dup 2)) (const_int 1)))
	     (unspec:DI [(match_operand:DI 1 "aarch64_valid_symref" "")] UNSPEC_GOTSMALLTLS)
	     (clobber (reg:DI LR_REGNUM))])]
 ""
{
  operands[2] = aarch64_tls_get_addr ();
})

(define_insn "*tlsgd_small"
  [(set (match_operand 0 "register_operand" "")
	(call (mem:DI (match_operand:DI 2 "" "")) (const_int 1)))
   (unspec:DI [(match_operand:DI 1 "aarch64_valid_symref" "S")] UNSPEC_GOTSMALLTLS)
   (clobber (reg:DI LR_REGNUM))
  ]
  ""
  "adrp\\tx0, %A1\;add\\tx0, x0, %L1\;bl\\t%2\;nop"
  [(set_attr "v8type" "call")
   (set_attr "length" "16")])

(define_insn "tlsie_small"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:DI 1 "aarch64_tls_ie_symref" "S")]
		   UNSPEC_GOTSMALLTLS))]
  ""
  "adrp\\t%0, %A1\;ldr\\t%0, [%0, #%L1]"
  [(set_attr "v8type" "load1")
   (set_attr "mode" "DI")
   (set_attr "length" "8")]
)

(define_insn "tlsle_small"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:DI 1 "register_operand" "r")
                   (match_operand:DI 2 "aarch64_tls_le_symref" "S")]
		   UNSPEC_GOTSMALLTLS))]
  ""
  "add\\t%0, %1, #%G2\;add\\t%0, %0, #%L2"
  [(set_attr "v8type" "alu")
   (set_attr "mode" "DI")
   (set_attr "length" "8")]
)

(define_insn "tlsdesc_small"
  [(set (reg:DI R0_REGNUM)
        (unspec:DI [(match_operand:DI 0 "aarch64_valid_symref" "S")]
		   UNSPEC_TLSDESC))
   (clobber (reg:DI LR_REGNUM))
   (clobber (match_scratch:DI 1 "=r"))]
  "TARGET_TLS_DESC"
  "adrp\\tx0, %A0\;ldr\\t%1, [x0, #%L0]\;add\\tx0, x0, %L0\;.tlsdesccall\\t%0\;blr\\t%1"
  [(set_attr "v8type" "call")
   (set_attr "length" "16")])

(define_insn "stack_tie"
  [(set (mem:BLK (scratch))
	(unspec:BLK [(match_operand:DI 0 "register_operand" "rk")
		     (match_operand:DI 1 "register_operand" "rk")]
		    UNSPEC_PRLG_STK))]
  ""
  ""
  [(set_attr "length" "0")]
)

;; Named pattern for expanding thread pointer reference.
(define_expand "get_thread_pointerdi"
  [(match_operand:DI 0 "register_operand" "=r")]
  ""
{
  rtx tmp = aarch64_load_tp (operands[0]);
  if (tmp != operands[0])
    emit_move_insn (operands[0], tmp);
  DONE;
})

;; AdvSIMD Stuff
(include "aarch64-simd.md")

;; Atomic Operations
(include "atomics.md")
