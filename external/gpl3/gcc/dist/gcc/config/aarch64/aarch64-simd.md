;; Machine description for AArch64 AdvSIMD architecture.
;; Copyright (C) 2011-2013 Free Software Foundation, Inc.
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


; Main data types used by the insntructions

(define_attr "simd_mode" "unknown,none,V8QI,V16QI,V4HI,V8HI,V2SI,V4SI,V2DI,V2SF,V4SF,V2DF,OI,CI,XI,DI,DF,SI,SF,HI,QI"
  (const_string "unknown"))


; Classification of AdvSIMD instructions for scheduling purposes.
; Do not set this attribute and the "v8type" attribute together in
; any instruction pattern.

; simd_abd              integer absolute difference and accumulate.
; simd_abdl             integer absolute difference and accumulate (long).
; simd_adal             integer add and accumulate (long).
; simd_add              integer addition/subtraction.
; simd_addl             integer addition/subtraction (long).
; simd_addlv            across lanes integer sum (long).
; simd_addn             integer addition/subtraction (narrow).
; simd_addn2            integer addition/subtraction (narrow, high).
; simd_addv             across lanes integer sum.
; simd_cls              count leading sign/zero bits.
; simd_cmp              compare / create mask.
; simd_cnt              population count.
; simd_dup              duplicate element.
; simd_dupgp            duplicate general purpose register.
; simd_ext              bitwise extract from pair.
; simd_fadd             floating point add/sub.
; simd_fcmp             floating point compare.
; simd_fcvti            floating point convert to integer.
; simd_fcvtl            floating-point convert upsize.
; simd_fcvtn            floating-point convert downsize (narrow).
; simd_fcvtn2           floating-point convert downsize (narrow, high).
; simd_fdiv             floating point division.
; simd_fminmax          floating point min/max.
; simd_fminmaxv         across lanes floating point min/max.
; simd_fmla             floating point multiply-add.
; simd_fmla_elt         floating point multiply-add (by element).
; simd_fmul             floating point multiply.
; simd_fmul_elt         floating point multiply (by element).
; simd_fnegabs          floating point neg/abs.
; simd_frcpe            floating point reciprocal estimate.
; simd_frcps            floating point reciprocal step.
; simd_frecx            floating point reciprocal exponent.
; simd_frint            floating point round to integer.
; simd_fsqrt            floating point square root.
; simd_icvtf            integer convert to floating point.
; simd_ins              insert element.
; simd_insgp            insert general purpose register.
; simd_load1            load multiple structures to one register (LD1).
; simd_load1r           load single structure to all lanes of one register (LD1R).
; simd_load1s           load single structure to one lane of one register (LD1 [index]).
; simd_load2            load multiple structures to two registers (LD1, LD2).
; simd_load2r           load single structure to all lanes of two registers (LD1R, LD2R).
; simd_load2s           load single structure to one lane of two registers (LD2 [index]).
; simd_load3            load multiple structures to three registers (LD1, LD3).
; simd_load3r           load single structure to all lanes of three registers (LD3R).
; simd_load3s           load single structure to one lane of three registers (LD3 [index]).
; simd_load4            load multiple structures to four registers (LD1, LD2, LD4).
; simd_load4r           load single structure to all lanes of four registers (LD4R).
; simd_load4s           load single structure to one lane of four registers (LD4 [index]).
; simd_logic            logical operation.
; simd_logic_imm        logcial operation (immediate).
; simd_minmax           integer min/max.
; simd_minmaxv          across lanes integer min/max,
; simd_mla              integer multiply-accumulate.
; simd_mla_elt          integer multiply-accumulate (by element).
; simd_mlal             integer multiply-accumulate (long).
; simd_mlal_elt         integer multiply-accumulate (by element, long).
; simd_move             move register.
; simd_move_imm         move immediate.
; simd_movgp            move element to general purpose register.
; simd_mul              integer multiply.
; simd_mul_elt          integer multiply (by element).
; simd_mull             integer multiply (long).
; simd_mull_elt         integer multiply (by element, long).
; simd_negabs           integer negate/absolute.
; simd_rbit             bitwise reverse.
; simd_rcpe             integer reciprocal estimate.
; simd_rcps             integer reciprocal square root.
; simd_rev              element reverse.
; simd_sat_add          integer saturating addition/subtraction.
; simd_sat_mlal         integer saturating multiply-accumulate (long).
; simd_sat_mlal_elt     integer saturating multiply-accumulate (by element, long).
; simd_sat_mul          integer saturating multiply.
; simd_sat_mul_elt      integer saturating multiply (by element).
; simd_sat_mull         integer saturating multiply (long).
; simd_sat_mull_elt     integer saturating multiply (by element, long).
; simd_sat_negabs       integer saturating negate/absolute.
; simd_sat_shift        integer saturating shift.
; simd_sat_shift_imm    integer saturating shift (immediate).
; simd_sat_shiftn_imm   integer saturating shift (narrow, immediate).
; simd_sat_shiftn2_imm  integer saturating shift (narrow, high, immediate).
; simd_shift            shift register/vector.
; simd_shift_acc        shift accumulate.
; simd_shift_imm        shift immediate.
; simd_shift_imm_acc    shift immediate and accumualte.
; simd_shiftl           shift register/vector (long).
; simd_shiftl_imm       shift register/vector (long, immediate).
; simd_shiftn_imm       shift register/vector (narrow, immediate).
; simd_shiftn2_imm      shift register/vector (narrow, high, immediate).
; simd_store1           store multiple structures from one register (ST1).
; simd_store1s          store single structure from one lane of one register (ST1 [index]).
; simd_store2           store multiple structures from two registers (ST1, ST2).
; simd_store2s          store single structure from one lane of two registers (ST2 [index]).
; simd_store3           store multiple structures from three registers (ST1, ST3).
; simd_store3s          store single structure from one lane of three register (ST3 [index]).
; simd_store4           store multiple structures from four registers (ST1, ST2, ST4).
; simd_store4s          store single structure from one lane for four registers (ST4 [index]).
; simd_tbl              table lookup.
; simd_trn              transpose.
; simd_uzp              unzip.
; simd_zip              zip.

(define_attr "simd_type"
   "simd_abd,\
   simd_abdl,\
   simd_adal,\
   simd_add,\
   simd_addl,\
   simd_addlv,\
   simd_addn,\
   simd_addn2,\
   simd_addv,\
   simd_cls,\
   simd_cmp,\
   simd_cnt,\
   simd_dup,\
   simd_dupgp,\
   simd_ext,\
   simd_fadd,\
   simd_fcmp,\
   simd_fcvti,\
   simd_fcvtl,\
   simd_fcvtn,\
   simd_fcvtn2,\
   simd_fdiv,\
   simd_fminmax,\
   simd_fminmaxv,\
   simd_fmla,\
   simd_fmla_elt,\
   simd_fmul,\
   simd_fmul_elt,\
   simd_fnegabs,\
   simd_frcpe,\
   simd_frcps,\
   simd_frecx,\
   simd_frint,\
   simd_fsqrt,\
   simd_icvtf,\
   simd_ins,\
   simd_insgp,\
   simd_load1,\
   simd_load1r,\
   simd_load1s,\
   simd_load2,\
   simd_load2r,\
   simd_load2s,\
   simd_load3,\
   simd_load3r,\
   simd_load3s,\
   simd_load4,\
   simd_load4r,\
   simd_load4s,\
   simd_logic,\
   simd_logic_imm,\
   simd_minmax,\
   simd_minmaxv,\
   simd_mla,\
   simd_mla_elt,\
   simd_mlal,\
   simd_mlal_elt,\
   simd_movgp,\
   simd_move,\
   simd_move_imm,\
   simd_mul,\
   simd_mul_elt,\
   simd_mull,\
   simd_mull_elt,\
   simd_negabs,\
   simd_rbit,\
   simd_rcpe,\
   simd_rcps,\
   simd_rev,\
   simd_sat_add,\
   simd_sat_mlal,\
   simd_sat_mlal_elt,\
   simd_sat_mul,\
   simd_sat_mul_elt,\
   simd_sat_mull,\
   simd_sat_mull_elt,\
   simd_sat_negabs,\
   simd_sat_shift,\
   simd_sat_shift_imm,\
   simd_sat_shiftn_imm,\
   simd_sat_shiftn2_imm,\
   simd_shift,\
   simd_shift_acc,\
   simd_shift_imm,\
   simd_shift_imm_acc,\
   simd_shiftl,\
   simd_shiftl_imm,\
   simd_shiftn_imm,\
   simd_shiftn2_imm,\
   simd_store1,\
   simd_store1s,\
   simd_store2,\
   simd_store2s,\
   simd_store3,\
   simd_store3s,\
   simd_store4,\
   simd_store4s,\
   simd_tbl,\
   simd_trn,\
   simd_uzp,\
   simd_zip,\
   none"
  (const_string "none"))


; The "neon_type" attribute is used by the AArch32 backend.  Below is a mapping
; from "simd_type" to "neon_type".

(define_attr "neon_type"
   "neon_int_1,neon_int_2,neon_int_3,neon_int_4,neon_int_5,neon_vqneg_vqabs,
   neon_vmov,neon_vaba,neon_vsma,neon_vaba_qqq,
   neon_mul_ddd_8_16_qdd_16_8_long_32_16_long,neon_mul_qqq_8_16_32_ddd_32,
   neon_mul_qdd_64_32_long_qqd_16_ddd_32_scalar_64_32_long_scalar,
   neon_mla_ddd_8_16_qdd_16_8_long_32_16_long,neon_mla_qqq_8_16,
   neon_mla_ddd_32_qqd_16_ddd_32_scalar_qdd_64_32_long_scalar_qdd_64_32_long,
   neon_mla_qqq_32_qqd_32_scalar,neon_mul_ddd_16_scalar_32_16_long_scalar,
   neon_mul_qqd_32_scalar,neon_mla_ddd_16_scalar_qdd_32_16_long_scalar,
   neon_shift_1,neon_shift_2,neon_shift_3,neon_vshl_ddd,
   neon_vqshl_vrshl_vqrshl_qqq,neon_vsra_vrsra,neon_fp_vadd_ddd_vabs_dd,
   neon_fp_vadd_qqq_vabs_qq,neon_fp_vsum,neon_fp_vmul_ddd,neon_fp_vmul_qqd,
   neon_fp_vmla_ddd,neon_fp_vmla_qqq,neon_fp_vmla_ddd_scalar,
   neon_fp_vmla_qqq_scalar,neon_fp_vrecps_vrsqrts_ddd,
   neon_fp_vrecps_vrsqrts_qqq,neon_bp_simple,neon_bp_2cycle,neon_bp_3cycle,
   neon_ldr,neon_str,neon_vld1_1_2_regs,neon_vld1_3_4_regs,
   neon_vld2_2_regs_vld1_vld2_all_lanes,neon_vld2_4_regs,neon_vld3_vld4,
   neon_vst1_1_2_regs_vst2_2_regs,neon_vst1_3_4_regs,
   neon_vst2_4_regs_vst3_vst4,neon_vst3_vst4,neon_vld1_vld2_lane,
   neon_vld3_vld4_lane,neon_vst1_vst2_lane,neon_vst3_vst4_lane,
   neon_vld3_vld4_all_lanes,neon_mcr,neon_mcr_2_mcrr,neon_mrc,neon_mrrc,
   neon_ldm_2,neon_stm_2,none,unknown"
  (cond [
	  (eq_attr "simd_type" "simd_dup") (const_string "neon_bp_simple")
	  (eq_attr "simd_type" "simd_movgp") (const_string "neon_bp_simple")
	  (eq_attr "simd_type" "simd_add,simd_logic,simd_logic_imm") (const_string "neon_int_1")
	  (eq_attr "simd_type" "simd_negabs,simd_addlv") (const_string "neon_int_3")
	  (eq_attr "simd_type" "simd_addn,simd_addn2,simd_addl,simd_sat_add,simd_sat_negabs") (const_string "neon_int_4")
	  (eq_attr "simd_type" "simd_move") (const_string "neon_vmov")
	  (eq_attr "simd_type" "simd_ins") (const_string "neon_mcr")
	  (and (eq_attr "simd_type" "simd_mul,simd_sat_mul") (eq_attr "simd_mode" "V8QI,V4HI")) (const_string "neon_mul_ddd_8_16_qdd_16_8_long_32_16_long")
	  (and (eq_attr "simd_type" "simd_mul,simd_sat_mul") (eq_attr "simd_mode" "V2SI,V8QI,V16QI,V2SI")) (const_string "neon_mul_qqq_8_16_32_ddd_32")
	  (and (eq_attr "simd_type" "simd_mull,simd_sat_mull") (eq_attr "simd_mode" "V8QI,V16QI,V4HI,V8HI")) (const_string "neon_mul_ddd_8_16_qdd_16_8_long_32_16_long")
	  (and (eq_attr "simd_type" "simd_mull,simd_sat_mull") (eq_attr "simd_mode" "V2SI,V4SI,V2DI")) (const_string "neon_mul_qdd_64_32_long_qqd_16_ddd_32_scalar_64_32_long_scalar")
	  (and (eq_attr "simd_type" "simd_mla,simd_sat_mlal") (eq_attr "simd_mode" "V8QI,V4HI")) (const_string "neon_mla_ddd_8_16_qdd_16_8_long_32_16_long")
	  (and (eq_attr "simd_type" "simd_mla,simd_sat_mlal") (eq_attr "simd_mode" "V2SI")) (const_string "neon_mla_ddd_32_qqd_16_ddd_32_scalar_qdd_64_32_long_scalar_qdd_64_32_long")
	  (and (eq_attr "simd_type" "simd_mla,simd_sat_mlal") (eq_attr "simd_mode" "V16QI,V8HI")) (const_string "neon_mla_qqq_8_16")
	  (and (eq_attr "simd_type" "simd_mla,simd_sat_mlal") (eq_attr "simd_mode" "V4SI")) (const_string "neon_mla_qqq_32_qqd_32_scalar")
	  (and (eq_attr "simd_type" "simd_mlal") (eq_attr "simd_mode" "V8QI,V16QI,V4HI,V8HI")) (const_string "neon_mla_ddd_8_16_qdd_16_8_long_32_16_long")
	  (and (eq_attr "simd_type" "simd_mlal") (eq_attr "simd_mode" "V2SI,V4SI,V2DI")) (const_string "neon_mla_ddd_32_qqd_16_ddd_32_scalar_qdd_64_32_long_scalar_qdd_64_32_long")
	  (and (eq_attr "simd_type" "simd_fmla") (eq_attr "simd_mode" "V2SF")) (const_string "neon_fp_vmla_ddd")
	  (and (eq_attr "simd_type" "simd_fmla") (eq_attr "simd_mode" "V4SF,V2DF")) (const_string "neon_fp_vmla_qqq")
	  (and (eq_attr "simd_type" "simd_fmla_elt") (eq_attr "simd_mode" "V2SF")) (const_string "neon_fp_vmla_ddd_scalar")
	  (and (eq_attr "simd_type" "simd_fmla_elt") (eq_attr "simd_mode" "V4SF,V2DF")) (const_string "neon_fp_vmla_qqq_scalar")
	  (and (eq_attr "simd_type" "simd_fmul,simd_fmul_elt,simd_fdiv,simd_fsqrt") (eq_attr "simd_mode" "V2SF")) (const_string "neon_fp_vmul_ddd")
	  (and (eq_attr "simd_type" "simd_fmul,simd_fmul_elt,simd_fdiv,simd_fsqrt") (eq_attr "simd_mode" "V4SF,V2DF")) (const_string "neon_fp_vmul_qqd")
	  (and (eq_attr "simd_type" "simd_fadd") (eq_attr "simd_mode" "V2SF")) (const_string "neon_fp_vadd_ddd_vabs_dd")
	  (and (eq_attr "simd_type" "simd_fadd") (eq_attr "simd_mode" "V4SF,V2DF")) (const_string "neon_fp_vadd_qqq_vabs_qq")
	  (and (eq_attr "simd_type" "simd_fnegabs,simd_fminmax,simd_fminmaxv") (eq_attr "simd_mode" "V2SF")) (const_string "neon_fp_vadd_ddd_vabs_dd")
	  (and (eq_attr "simd_type" "simd_fnegabs,simd_fminmax,simd_fminmaxv") (eq_attr "simd_mode" "V4SF,V2DF")) (const_string "neon_fp_vadd_qqq_vabs_qq")
	  (and (eq_attr "simd_type" "simd_shift,simd_shift_acc") (eq_attr "simd_mode" "V8QI,V4HI,V2SI")) (const_string "neon_vshl_ddd")
	  (and (eq_attr "simd_type" "simd_shift,simd_shift_acc") (eq_attr "simd_mode" "V16QI,V8HI,V4SI,V2DI")) (const_string "neon_shift_3")
	  (eq_attr "simd_type" "simd_minmax,simd_minmaxv") (const_string "neon_int_5")
	  (eq_attr "simd_type" "simd_shiftn_imm,simd_shiftn2_imm,simd_shiftl_imm,") (const_string "neon_shift_1")
	  (eq_attr "simd_type" "simd_load1,simd_load2") (const_string "neon_vld1_1_2_regs")
	  (eq_attr "simd_type" "simd_load3,simd_load3") (const_string "neon_vld1_3_4_regs")
	  (eq_attr "simd_type" "simd_load1r,simd_load2r,simd_load3r,simd_load4r") (const_string "neon_vld2_2_regs_vld1_vld2_all_lanes")
	  (eq_attr "simd_type" "simd_load1s,simd_load2s") (const_string "neon_vld1_vld2_lane")
	  (eq_attr "simd_type" "simd_load3s,simd_load4s") (const_string "neon_vld3_vld4_lane")
	  (eq_attr "simd_type" "simd_store1,simd_store2") (const_string "neon_vst1_1_2_regs_vst2_2_regs")
	  (eq_attr "simd_type" "simd_store3,simd_store4") (const_string "neon_vst1_3_4_regs")
	  (eq_attr "simd_type" "simd_store1s,simd_store2s") (const_string "neon_vst1_vst2_lane")
	  (eq_attr "simd_type" "simd_store3s,simd_store4s") (const_string "neon_vst3_vst4_lane")
	  (and (eq_attr "simd_type" "simd_frcpe,simd_frcps") (eq_attr "simd_mode" "V2SF")) (const_string "neon_fp_vrecps_vrsqrts_ddd")
	  (and (eq_attr "simd_type" "simd_frcpe,simd_frcps") (eq_attr "simd_mode" "V4SF,V2DF")) (const_string "neon_fp_vrecps_vrsqrts_qqq")
	  (eq_attr "simd_type" "none") (const_string "none")
  ]
  (const_string "unknown")))


(define_expand "mov<mode>"
  [(set (match_operand:VALL 0 "aarch64_simd_nonimmediate_operand" "")
	(match_operand:VALL 1 "aarch64_simd_general_operand" ""))]
  "TARGET_SIMD"
  "
    if (GET_CODE (operands[0]) == MEM)
      operands[1] = force_reg (<MODE>mode, operands[1]);
  "
)

(define_expand "movmisalign<mode>"
  [(set (match_operand:VALL 0 "aarch64_simd_nonimmediate_operand" "")
        (match_operand:VALL 1 "aarch64_simd_general_operand" ""))]
  "TARGET_SIMD"
{
  /* This pattern is not permitted to fail during expansion: if both arguments
     are non-registers (e.g. memory := constant, which can be created by the
     auto-vectorizer), force operand 1 into a register.  */
  if (!register_operand (operands[0], <MODE>mode)
      && !register_operand (operands[1], <MODE>mode))
    operands[1] = force_reg (<MODE>mode, operands[1]);
})

(define_insn "aarch64_simd_dup<mode>"
  [(set (match_operand:VDQ 0 "register_operand" "=w")
        (vec_duplicate:VDQ (match_operand:<VEL> 1 "register_operand" "r")))]
  "TARGET_SIMD"
  "dup\\t%0.<Vtype>, %<vw>1"
  [(set_attr "simd_type" "simd_dupgp")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_dup_lane<mode>"
  [(set (match_operand:VDQ_I 0 "register_operand" "=w")
        (vec_duplicate:VDQ_I
	  (vec_select:<VEL>
	    (match_operand:<VCON> 1 "register_operand" "w")
	    (parallel [(match_operand:SI 2 "immediate_operand" "i")])
          )))]
  "TARGET_SIMD"
  "dup\\t%<v>0<Vmtype>, %1.<Vetype>[%2]"
  [(set_attr "simd_type" "simd_dup")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_dup_lane<mode>"
  [(set (match_operand:SDQ_I 0 "register_operand" "=w")
	(vec_select:<VEL>
	  (match_operand:<VCON> 1 "register_operand" "w")
	  (parallel [(match_operand:SI 2 "immediate_operand" "i")])
        ))]
  "TARGET_SIMD"
  "dup\\t%<v>0<Vmtype>, %1.<Vetype>[%2]"
  [(set_attr "simd_type" "simd_dup")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_simd_dup<mode>"
  [(set (match_operand:VDQF 0 "register_operand" "=w")
        (vec_duplicate:VDQF (match_operand:<VEL> 1 "register_operand" "w")))]
  "TARGET_SIMD"
  "dup\\t%0.<Vtype>, %1.<Vetype>[0]"
  [(set_attr "simd_type" "simd_dup")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "*aarch64_simd_mov<mode>"
  [(set (match_operand:VD 0 "aarch64_simd_nonimmediate_operand"
		"=w, Utv,  w, ?r, ?w, ?r, w")
	(match_operand:VD 1 "aarch64_simd_general_operand"
		"Utv,  w,  w,  w,  r,  r, Dn"))]
  "TARGET_SIMD
   && (register_operand (operands[0], <MODE>mode)
       || register_operand (operands[1], <MODE>mode))"
{
   switch (which_alternative)
     {
     case 0: return "ld1\t{%0.<Vtype>}, %1";
     case 1: return "st1\t{%1.<Vtype>}, %0";
     case 2: return "orr\t%0.<Vbtype>, %1.<Vbtype>, %1.<Vbtype>";
     case 3: return "umov\t%0, %1.d[0]";
     case 4: return "ins\t%0.d[0], %1";
     case 5: return "mov\t%0, %1";
     case 6:
	return aarch64_output_simd_mov_immediate (&operands[1],
						  <MODE>mode, 64);
     default: gcc_unreachable ();
     }
}
  [(set_attr "simd_type" "simd_load1,simd_store1,simd_move,simd_movgp,simd_insgp,simd_move,simd_move_imm")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "*aarch64_simd_mov<mode>"
  [(set (match_operand:VQ 0 "aarch64_simd_nonimmediate_operand"
		"=w, Utv,  w, ?r, ?w, ?r, w")
	(match_operand:VQ 1 "aarch64_simd_general_operand"
		"Utv,  w,  w,  w,  r,  r, Dn"))]
  "TARGET_SIMD
   && (register_operand (operands[0], <MODE>mode)
       || register_operand (operands[1], <MODE>mode))"
{
  switch (which_alternative)
    {
    case 0: return "ld1\t{%0.<Vtype>}, %1";
    case 1: return "st1\t{%1.<Vtype>}, %0";
    case 2: return "orr\t%0.<Vbtype>, %1.<Vbtype>, %1.<Vbtype>";
    case 3: return "umov\t%0, %1.d[0]\;umov\t%H0, %1.d[1]";
    case 4: return "ins\t%0.d[0], %1\;ins\t%0.d[1], %H1";
    case 5: return "#";
    case 6:
	return aarch64_output_simd_mov_immediate (&operands[1],
						  <MODE>mode, 128);
    default: gcc_unreachable ();
    }
}
  [(set_attr "simd_type" "simd_load1,simd_store1,simd_move,simd_movgp,simd_insgp,simd_move,simd_move_imm")
   (set_attr "simd_mode" "<MODE>")
   (set_attr "length" "4,4,4,8,8,8,4")]
)

(define_split
  [(set (match_operand:VQ 0 "register_operand" "")
      (match_operand:VQ 1 "register_operand" ""))]
  "TARGET_SIMD && reload_completed
   && GP_REGNUM_P (REGNO (operands[0]))
   && GP_REGNUM_P (REGNO (operands[1]))"
  [(set (match_dup 0) (match_dup 1))
   (set (match_dup 2) (match_dup 3))]
{
  int rdest = REGNO (operands[0]);
  int rsrc = REGNO (operands[1]);
  rtx dest[2], src[2];

  dest[0] = gen_rtx_REG (DImode, rdest);
  src[0] = gen_rtx_REG (DImode, rsrc);
  dest[1] = gen_rtx_REG (DImode, rdest + 1);
  src[1] = gen_rtx_REG (DImode, rsrc + 1);

  aarch64_simd_disambiguate_copy (operands, dest, src, 2);
})

(define_insn "orn<mode>3"
 [(set (match_operand:VDQ 0 "register_operand" "=w")
       (ior:VDQ (not:VDQ (match_operand:VDQ 1 "register_operand" "w"))
		(match_operand:VDQ 2 "register_operand" "w")))]
 "TARGET_SIMD"
 "orn\t%0.<Vbtype>, %2.<Vbtype>, %1.<Vbtype>"
  [(set_attr "simd_type" "simd_logic")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "bic<mode>3"
 [(set (match_operand:VDQ 0 "register_operand" "=w")
       (and:VDQ (not:VDQ (match_operand:VDQ 1 "register_operand" "w"))
		(match_operand:VDQ 2 "register_operand" "w")))]
 "TARGET_SIMD"
 "bic\t%0.<Vbtype>, %2.<Vbtype>, %1.<Vbtype>"
  [(set_attr "simd_type" "simd_logic")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "add<mode>3"
  [(set (match_operand:VDQ 0 "register_operand" "=w")
        (plus:VDQ (match_operand:VDQ 1 "register_operand" "w")
		  (match_operand:VDQ 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "add\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_add")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "sub<mode>3"
  [(set (match_operand:VDQ 0 "register_operand" "=w")
        (minus:VDQ (match_operand:VDQ 1 "register_operand" "w")
		   (match_operand:VDQ 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "sub\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_add")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "mul<mode>3"
  [(set (match_operand:VDQM 0 "register_operand" "=w")
        (mult:VDQM (match_operand:VDQM 1 "register_operand" "w")
		   (match_operand:VDQM 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "mul\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_mul")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "neg<mode>2"
  [(set (match_operand:VDQM 0 "register_operand" "=w")
        (neg:VDQM (match_operand:VDQM 1 "register_operand" "w")))]
  "TARGET_SIMD"
  "neg\t%0.<Vtype>, %1.<Vtype>"
  [(set_attr "simd_type" "simd_negabs")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "abs<mode>2"
  [(set (match_operand:VDQ 0 "register_operand" "=w")
        (abs:VDQ (match_operand:VDQ 1 "register_operand" "w")))]
  "TARGET_SIMD"
  "abs\t%0.<Vtype>, %1.<Vtype>"
  [(set_attr "simd_type" "simd_negabs")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "and<mode>3"
  [(set (match_operand:VDQ 0 "register_operand" "=w")
        (and:VDQ (match_operand:VDQ 1 "register_operand" "w")
		 (match_operand:VDQ 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "and\t%0.<Vbtype>, %1.<Vbtype>, %2.<Vbtype>"
  [(set_attr "simd_type" "simd_logic")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "ior<mode>3"
  [(set (match_operand:VDQ 0 "register_operand" "=w")
        (ior:VDQ (match_operand:VDQ 1 "register_operand" "w")
		 (match_operand:VDQ 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "orr\t%0.<Vbtype>, %1.<Vbtype>, %2.<Vbtype>"
  [(set_attr "simd_type" "simd_logic")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "xor<mode>3"
  [(set (match_operand:VDQ 0 "register_operand" "=w")
        (xor:VDQ (match_operand:VDQ 1 "register_operand" "w")
		 (match_operand:VDQ 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "eor\t%0.<Vbtype>, %1.<Vbtype>, %2.<Vbtype>"
  [(set_attr "simd_type" "simd_logic")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "one_cmpl<mode>2"
  [(set (match_operand:VDQ 0 "register_operand" "=w")
        (not:VDQ (match_operand:VDQ 1 "register_operand" "w")))]
  "TARGET_SIMD"
  "not\t%0.<Vbtype>, %1.<Vbtype>"
  [(set_attr "simd_type" "simd_logic")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_simd_vec_set<mode>"
  [(set (match_operand:VQ_S 0 "register_operand" "=w")
        (vec_merge:VQ_S
	    (vec_duplicate:VQ_S
		(match_operand:<VEL> 1 "register_operand" "r"))
	    (match_operand:VQ_S 3 "register_operand" "0")
	    (match_operand:SI 2 "immediate_operand" "i")))]
  "TARGET_SIMD"
  "ins\t%0.<Vetype>[%p2], %w1";
  [(set_attr "simd_type" "simd_insgp")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_simd_lshr<mode>"
 [(set (match_operand:VDQ 0 "register_operand" "=w")
       (lshiftrt:VDQ (match_operand:VDQ 1 "register_operand" "w")
		     (match_operand:VDQ  2 "aarch64_simd_rshift_imm" "Dr")))]
 "TARGET_SIMD"
 "ushr\t%0.<Vtype>, %1.<Vtype>, %2"
  [(set_attr "simd_type" "simd_shift_imm")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_simd_ashr<mode>"
 [(set (match_operand:VDQ 0 "register_operand" "=w")
       (ashiftrt:VDQ (match_operand:VDQ 1 "register_operand" "w")
		     (match_operand:VDQ  2 "aarch64_simd_rshift_imm" "Dr")))]
 "TARGET_SIMD"
 "sshr\t%0.<Vtype>, %1.<Vtype>, %2"
  [(set_attr "simd_type" "simd_shift_imm")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_simd_imm_shl<mode>"
 [(set (match_operand:VDQ 0 "register_operand" "=w")
       (ashift:VDQ (match_operand:VDQ 1 "register_operand" "w")
		   (match_operand:VDQ  2 "aarch64_simd_lshift_imm" "Dl")))]
 "TARGET_SIMD"
  "shl\t%0.<Vtype>, %1.<Vtype>, %2"
  [(set_attr "simd_type" "simd_shift_imm")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_simd_reg_sshl<mode>"
 [(set (match_operand:VDQ 0 "register_operand" "=w")
       (ashift:VDQ (match_operand:VDQ 1 "register_operand" "w")
		   (match_operand:VDQ 2 "register_operand" "w")))]
 "TARGET_SIMD"
 "sshl\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_shift")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_simd_reg_shl<mode>_unsigned"
 [(set (match_operand:VDQ 0 "register_operand" "=w")
       (unspec:VDQ [(match_operand:VDQ 1 "register_operand" "w")
		    (match_operand:VDQ 2 "register_operand" "w")]
		   UNSPEC_ASHIFT_UNSIGNED))]
 "TARGET_SIMD"
 "ushl\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_shift")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_simd_reg_shl<mode>_signed"
 [(set (match_operand:VDQ 0 "register_operand" "=w")
       (unspec:VDQ [(match_operand:VDQ 1 "register_operand" "w")
		    (match_operand:VDQ 2 "register_operand" "w")]
		   UNSPEC_ASHIFT_SIGNED))]
 "TARGET_SIMD"
 "sshl\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_shift")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "ashl<mode>3"
  [(match_operand:VDQ 0 "register_operand" "")
   (match_operand:VDQ 1 "register_operand" "")
   (match_operand:SI  2 "general_operand" "")]
 "TARGET_SIMD"
{
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  int shift_amount;

  if (CONST_INT_P (operands[2]))
    {
      shift_amount = INTVAL (operands[2]);
      if (shift_amount >= 0 && shift_amount < bit_width)
        {
	  rtx tmp = aarch64_simd_gen_const_vector_dup (<MODE>mode,
						       shift_amount);
	  emit_insn (gen_aarch64_simd_imm_shl<mode> (operands[0],
						     operands[1],
						     tmp));
          DONE;
        }
      else
        {
          operands[2] = force_reg (SImode, operands[2]);
        }
    }
  else if (MEM_P (operands[2]))
    {
      operands[2] = force_reg (SImode, operands[2]);
    }

  if (REG_P (operands[2]))
    {
      rtx tmp = gen_reg_rtx (<MODE>mode);
      emit_insn (gen_aarch64_simd_dup<mode> (tmp,
					     convert_to_mode (<VEL>mode,
							      operands[2],
							      0)));
      emit_insn (gen_aarch64_simd_reg_sshl<mode> (operands[0], operands[1],
						  tmp));
      DONE;
    }
  else
    FAIL;
}
)

(define_expand "lshr<mode>3"
  [(match_operand:VDQ 0 "register_operand" "")
   (match_operand:VDQ 1 "register_operand" "")
   (match_operand:SI  2 "general_operand" "")]
 "TARGET_SIMD"
{
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  int shift_amount;

  if (CONST_INT_P (operands[2]))
    {
      shift_amount = INTVAL (operands[2]);
      if (shift_amount > 0 && shift_amount <= bit_width)
        {
	  rtx tmp = aarch64_simd_gen_const_vector_dup (<MODE>mode,
						       shift_amount);
          emit_insn (gen_aarch64_simd_lshr<mode> (operands[0],
						  operands[1],
						  tmp));
	  DONE;
	}
      else
        operands[2] = force_reg (SImode, operands[2]);
    }
  else if (MEM_P (operands[2]))
    {
      operands[2] = force_reg (SImode, operands[2]);
    }

  if (REG_P (operands[2]))
    {
      rtx tmp = gen_reg_rtx (SImode);
      rtx tmp1 = gen_reg_rtx (<MODE>mode);
      emit_insn (gen_negsi2 (tmp, operands[2]));
      emit_insn (gen_aarch64_simd_dup<mode> (tmp1,
					     convert_to_mode (<VEL>mode,
							      tmp, 0)));
      emit_insn (gen_aarch64_simd_reg_shl<mode>_unsigned (operands[0],
							  operands[1],
							  tmp1));
      DONE;
    }
  else
    FAIL;
}
)

(define_expand "ashr<mode>3"
  [(match_operand:VDQ 0 "register_operand" "")
   (match_operand:VDQ 1 "register_operand" "")
   (match_operand:SI  2 "general_operand" "")]
 "TARGET_SIMD"
{
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  int shift_amount;

  if (CONST_INT_P (operands[2]))
    {
      shift_amount = INTVAL (operands[2]);
      if (shift_amount > 0 && shift_amount <= bit_width)
        {
	  rtx tmp = aarch64_simd_gen_const_vector_dup (<MODE>mode,
						       shift_amount);
          emit_insn (gen_aarch64_simd_ashr<mode> (operands[0],
						  operands[1],
						  tmp));
          DONE;
	}
      else
        operands[2] = force_reg (SImode, operands[2]);
    }
  else if (MEM_P (operands[2]))
    {
      operands[2] = force_reg (SImode, operands[2]);
    }

  if (REG_P (operands[2]))
    {
      rtx tmp = gen_reg_rtx (SImode);
      rtx tmp1 = gen_reg_rtx (<MODE>mode);
      emit_insn (gen_negsi2 (tmp, operands[2]));
      emit_insn (gen_aarch64_simd_dup<mode> (tmp1,
					     convert_to_mode (<VEL>mode,
							      tmp, 0)));
      emit_insn (gen_aarch64_simd_reg_shl<mode>_signed (operands[0],
							operands[1],
							tmp1));
      DONE;
    }
  else
    FAIL;
}
)

(define_expand "vashl<mode>3"
 [(match_operand:VDQ 0 "register_operand" "")
  (match_operand:VDQ 1 "register_operand" "")
  (match_operand:VDQ 2 "register_operand" "")]
 "TARGET_SIMD"
{
  emit_insn (gen_aarch64_simd_reg_sshl<mode> (operands[0], operands[1],
					      operands[2]));
  DONE;
})

;; Using mode VQ_S as there is no V2DImode neg!
;; Negating individual lanes most certainly offsets the
;; gain from vectorization.
(define_expand "vashr<mode>3"
 [(match_operand:VQ_S 0 "register_operand" "")
  (match_operand:VQ_S 1 "register_operand" "")
  (match_operand:VQ_S 2 "register_operand" "")]
 "TARGET_SIMD"
{
  rtx neg = gen_reg_rtx (<MODE>mode);
  emit (gen_neg<mode>2 (neg, operands[2]));
  emit_insn (gen_aarch64_simd_reg_shl<mode>_signed (operands[0], operands[1],
						    neg));
  DONE;
})

(define_expand "vlshr<mode>3"
 [(match_operand:VQ_S 0 "register_operand" "")
  (match_operand:VQ_S 1 "register_operand" "")
  (match_operand:VQ_S 2 "register_operand" "")]
 "TARGET_SIMD"
{
  rtx neg = gen_reg_rtx (<MODE>mode);
  emit (gen_neg<mode>2 (neg, operands[2]));
  emit_insn (gen_aarch64_simd_reg_shl<mode>_unsigned (operands[0], operands[1],
						      neg));
  DONE;
})

(define_expand "vec_set<mode>"
  [(match_operand:VQ_S 0 "register_operand" "+w")
   (match_operand:<VEL> 1 "register_operand" "r")
   (match_operand:SI 2 "immediate_operand" "")]
  "TARGET_SIMD"
  {
    HOST_WIDE_INT elem = (HOST_WIDE_INT) 1 << INTVAL (operands[2]);
    emit_insn (gen_aarch64_simd_vec_set<mode> (operands[0], operands[1],
					    GEN_INT (elem), operands[0]));
    DONE;
  }
)

(define_insn "aarch64_simd_vec_setv2di"
  [(set (match_operand:V2DI 0 "register_operand" "=w")
        (vec_merge:V2DI
	    (vec_duplicate:V2DI
		(match_operand:DI 1 "register_operand" "r"))
	    (match_operand:V2DI 3 "register_operand" "0")
	    (match_operand:SI 2 "immediate_operand" "i")))]
  "TARGET_SIMD"
  "ins\t%0.d[%p2], %1";
  [(set_attr "simd_type" "simd_insgp")
   (set_attr "simd_mode" "V2DI")]
)

(define_expand "vec_setv2di"
  [(match_operand:V2DI 0 "register_operand" "+w")
   (match_operand:DI 1 "register_operand" "r")
   (match_operand:SI 2 "immediate_operand" "")]
  "TARGET_SIMD"
  {
    HOST_WIDE_INT elem = (HOST_WIDE_INT) 1 << INTVAL (operands[2]);
    emit_insn (gen_aarch64_simd_vec_setv2di (operands[0], operands[1],
					  GEN_INT (elem), operands[0]));
    DONE;
  }
)

(define_insn "aarch64_simd_vec_set<mode>"
  [(set (match_operand:VDQF 0 "register_operand" "=w")
        (vec_merge:VDQF
	    (vec_duplicate:VDQF
		(match_operand:<VEL> 1 "register_operand" "w"))
	    (match_operand:VDQF 3 "register_operand" "0")
	    (match_operand:SI 2 "immediate_operand" "i")))]
  "TARGET_SIMD"
  "ins\t%0.<Vetype>[%p2], %1.<Vetype>[0]";
  [(set_attr "simd_type" "simd_ins")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "vec_set<mode>"
  [(match_operand:VDQF 0 "register_operand" "+w")
   (match_operand:<VEL> 1 "register_operand" "w")
   (match_operand:SI 2 "immediate_operand" "")]
  "TARGET_SIMD"
  {
    HOST_WIDE_INT elem = (HOST_WIDE_INT) 1 << INTVAL (operands[2]);
    emit_insn (gen_aarch64_simd_vec_set<mode> (operands[0], operands[1],
					  GEN_INT (elem), operands[0]));
    DONE;
  }
)


(define_insn "aarch64_mla<mode>"
 [(set (match_operand:VQ_S 0 "register_operand" "=w")
       (plus:VQ_S (mult:VQ_S (match_operand:VQ_S 2 "register_operand" "w")
			     (match_operand:VQ_S 3 "register_operand" "w"))
		  (match_operand:VQ_S 1 "register_operand" "0")))]
 "TARGET_SIMD"
 "mla\t%0.<Vtype>, %2.<Vtype>, %3.<Vtype>"
  [(set_attr "simd_type" "simd_mla")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_mls<mode>"
 [(set (match_operand:VQ_S 0 "register_operand" "=w")
       (minus:VQ_S (match_operand:VQ_S 1 "register_operand" "0")
		   (mult:VQ_S (match_operand:VQ_S 2 "register_operand" "w")
			      (match_operand:VQ_S 3 "register_operand" "w"))))]
 "TARGET_SIMD"
 "mls\t%0.<Vtype>, %2.<Vtype>, %3.<Vtype>"
  [(set_attr "simd_type" "simd_mla")
   (set_attr "simd_mode" "<MODE>")]
)

;; Max/Min operations.
(define_insn "<maxmin><mode>3"
 [(set (match_operand:VQ_S 0 "register_operand" "=w")
       (MAXMIN:VQ_S (match_operand:VQ_S 1 "register_operand" "w")
		    (match_operand:VQ_S 2 "register_operand" "w")))]
 "TARGET_SIMD"
 "<maxmin>\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_minmax")
   (set_attr "simd_mode" "<MODE>")]
)

;; Move into low-half clearing high half to 0.

(define_insn "move_lo_quad_<mode>"
  [(set (match_operand:VQ 0 "register_operand" "=w")
        (vec_concat:VQ
	  (match_operand:<VHALF> 1 "register_operand" "w")
	  (vec_duplicate:<VHALF> (const_int 0))))]
  "TARGET_SIMD"
  "mov\\t%d0, %d1";
  [(set_attr "simd_type" "simd_dup")
   (set_attr "simd_mode" "<MODE>")]
)

;; Move into high-half.

(define_insn "aarch64_simd_move_hi_quad_<mode>"
  [(set (match_operand:VQ 0 "register_operand" "+w")
        (vec_concat:VQ
          (vec_select:<VHALF>
                (match_dup 0)
                (match_operand:VQ 2 "vect_par_cnst_lo_half" ""))
	  (match_operand:<VHALF> 1 "register_operand" "w")))]
  "TARGET_SIMD"
  "ins\\t%0.d[1], %1.d[0]";
  [(set_attr "simd_type" "simd_ins")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "move_hi_quad_<mode>"
 [(match_operand:VQ 0 "register_operand" "")
  (match_operand:<VHALF> 1 "register_operand" "")]
 "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, false);
  emit_insn (gen_aarch64_simd_move_hi_quad_<mode> (operands[0],
						   operands[1], p));
  DONE;
})

;; Narrowing operations.

;; For doubles.
(define_insn "aarch64_simd_vec_pack_trunc_<mode>"
 [(set (match_operand:<VNARROWQ> 0 "register_operand" "=w")
       (truncate:<VNARROWQ> (match_operand:VQN 1 "register_operand" "w")))]
 "TARGET_SIMD"
 "xtn\\t%0.<Vntype>, %1.<Vtype>"
  [(set_attr "simd_type" "simd_shiftn_imm")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "vec_pack_trunc_<mode>"
 [(match_operand:<VNARROWD> 0 "register_operand" "")
  (match_operand:VDN 1 "register_operand" "")
  (match_operand:VDN 2 "register_operand" "")]
 "TARGET_SIMD"
{
  rtx tempreg = gen_reg_rtx (<VDBL>mode);

  emit_insn (gen_move_lo_quad_<Vdbl> (tempreg, operands[1]));
  emit_insn (gen_move_hi_quad_<Vdbl> (tempreg, operands[2]));
  emit_insn (gen_aarch64_simd_vec_pack_trunc_<Vdbl> (operands[0], tempreg));
  DONE;
})

;; For quads.

(define_insn "vec_pack_trunc_<mode>"
 [(set (match_operand:<VNARROWQ2> 0 "register_operand" "+&w")
       (vec_concat:<VNARROWQ2>
	 (truncate:<VNARROWQ> (match_operand:VQN 1 "register_operand" "w"))
	 (truncate:<VNARROWQ> (match_operand:VQN 2 "register_operand" "w"))))]
 "TARGET_SIMD"
 "xtn\\t%0.<Vntype>, %1.<Vtype>\;xtn2\\t%0.<V2ntype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_shiftn2_imm")
   (set_attr "simd_mode" "<MODE>")
   (set_attr "length" "8")]
)

;; Widening operations.

(define_insn "aarch64_simd_vec_unpack<su>_lo_<mode>"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ANY_EXTEND:<VWIDE> (vec_select:<VHALF>
			       (match_operand:VQW 1 "register_operand" "w")
			       (match_operand:VQW 2 "vect_par_cnst_lo_half" "")
			    )))]
  "TARGET_SIMD"
  "<su>shll %0.<Vwtype>, %1.<Vhalftype>, 0"
  [(set_attr "simd_type" "simd_shiftl_imm")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_simd_vec_unpack<su>_hi_<mode>"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ANY_EXTEND:<VWIDE> (vec_select:<VHALF>
			       (match_operand:VQW 1 "register_operand" "w")
			       (match_operand:VQW 2 "vect_par_cnst_hi_half" "")
			    )))]
  "TARGET_SIMD"
  "<su>shll2 %0.<Vwtype>, %1.<Vtype>, 0"
  [(set_attr "simd_type" "simd_shiftl_imm")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "vec_unpack<su>_hi_<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "")
   (ANY_EXTEND:<VWIDE> (match_operand:VQW 1 "register_operand"))]
  "TARGET_SIMD"
  {
    rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
    emit_insn (gen_aarch64_simd_vec_unpack<su>_hi_<mode> (operands[0],
							  operands[1], p));
    DONE;
  }
)

(define_expand "vec_unpack<su>_lo_<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "")
   (ANY_EXTEND:<VWIDE> (match_operand:VQW 1 "register_operand" ""))]
  "TARGET_SIMD"
  {
    rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, false);
    emit_insn (gen_aarch64_simd_vec_unpack<su>_lo_<mode> (operands[0],
							  operands[1], p));
    DONE;
  }
)

;; Widening arithmetic.

(define_insn "aarch64_simd_vec_<su>mult_lo_<mode>"
 [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
       (mult:<VWIDE> (ANY_EXTEND:<VWIDE> (vec_select:<VHALF>
			   (match_operand:VQW 1 "register_operand" "w")
                           (match_operand:VQW 3 "vect_par_cnst_lo_half" "")))
		     (ANY_EXTEND:<VWIDE> (vec_select:<VHALF>
                           (match_operand:VQW 2 "register_operand" "w")
                           (match_dup 3)))))]
  "TARGET_SIMD"
  "<su>mull\\t%0.<Vwtype>, %1.<Vhalftype>, %2.<Vhalftype>"
  [(set_attr "simd_type" "simd_mull")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "vec_widen_<su>mult_lo_<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "")
   (ANY_EXTEND:<VWIDE> (match_operand:VQW 1 "register_operand" ""))
   (ANY_EXTEND:<VWIDE> (match_operand:VQW 2 "register_operand" ""))]
 "TARGET_SIMD"
 {
   rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, false);
   emit_insn (gen_aarch64_simd_vec_<su>mult_lo_<mode> (operands[0],
						       operands[1],
						       operands[2], p));
   DONE;
 }
)

(define_insn "aarch64_simd_vec_<su>mult_hi_<mode>"
 [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
      (mult:<VWIDE> (ANY_EXTEND:<VWIDE> (vec_select:<VHALF>
			    (match_operand:VQW 1 "register_operand" "w")
			    (match_operand:VQW 3 "vect_par_cnst_hi_half" "")))
		    (ANY_EXTEND:<VWIDE> (vec_select:<VHALF>
			    (match_operand:VQW 2 "register_operand" "w")
			    (match_dup 3)))))]
  "TARGET_SIMD"
  "<su>mull2\\t%0.<Vwtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_mull")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "vec_widen_<su>mult_hi_<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "")
   (ANY_EXTEND:<VWIDE> (match_operand:VQW 1 "register_operand" ""))
   (ANY_EXTEND:<VWIDE> (match_operand:VQW 2 "register_operand" ""))]
 "TARGET_SIMD"
 {
   rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
   emit_insn (gen_aarch64_simd_vec_<su>mult_hi_<mode> (operands[0],
						       operands[1],
						       operands[2], p));
   DONE;

 }
)

;; FP vector operations.
;; AArch64 AdvSIMD supports single-precision (32-bit) and 
;; double-precision (64-bit) floating-point data types and arithmetic as
;; defined by the IEEE 754-2008 standard.  This makes them vectorizable 
;; without the need for -ffast-math or -funsafe-math-optimizations.
;;
;; Floating-point operations can raise an exception.  Vectorizing such
;; operations are safe because of reasons explained below.
;;
;; ARMv8 permits an extension to enable trapped floating-point
;; exception handling, however this is an optional feature.  In the
;; event of a floating-point exception being raised by vectorised
;; code then:
;; 1.  If trapped floating-point exceptions are available, then a trap
;;     will be taken when any lane raises an enabled exception.  A trap
;;     handler may determine which lane raised the exception.
;; 2.  Alternatively a sticky exception flag is set in the
;;     floating-point status register (FPSR).  Software may explicitly
;;     test the exception flags, in which case the tests will either
;;     prevent vectorisation, allowing precise identification of the
;;     failing operation, or if tested outside of vectorisable regions
;;     then the specific operation and lane are not of interest.

;; FP arithmetic operations.

(define_insn "add<mode>3"
 [(set (match_operand:VDQF 0 "register_operand" "=w")
       (plus:VDQF (match_operand:VDQF 1 "register_operand" "w")
		  (match_operand:VDQF 2 "register_operand" "w")))]
 "TARGET_SIMD"
 "fadd\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_fadd")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "sub<mode>3"
 [(set (match_operand:VDQF 0 "register_operand" "=w")
       (minus:VDQF (match_operand:VDQF 1 "register_operand" "w")
		   (match_operand:VDQF 2 "register_operand" "w")))]
 "TARGET_SIMD"
 "fsub\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_fadd")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "mul<mode>3"
 [(set (match_operand:VDQF 0 "register_operand" "=w")
       (mult:VDQF (match_operand:VDQF 1 "register_operand" "w")
		  (match_operand:VDQF 2 "register_operand" "w")))]
 "TARGET_SIMD"
 "fmul\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_fmul")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "div<mode>3"
 [(set (match_operand:VDQF 0 "register_operand" "=w")
       (div:VDQF (match_operand:VDQF 1 "register_operand" "w")
		 (match_operand:VDQF 2 "register_operand" "w")))]
 "TARGET_SIMD"
 "fdiv\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_fdiv")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "neg<mode>2"
 [(set (match_operand:VDQF 0 "register_operand" "=w")
       (neg:VDQF (match_operand:VDQF 1 "register_operand" "w")))]
 "TARGET_SIMD"
 "fneg\\t%0.<Vtype>, %1.<Vtype>"
  [(set_attr "simd_type" "simd_fnegabs")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "abs<mode>2"
 [(set (match_operand:VDQF 0 "register_operand" "=w")
       (abs:VDQF (match_operand:VDQF 1 "register_operand" "w")))]
 "TARGET_SIMD"
 "fabs\\t%0.<Vtype>, %1.<Vtype>"
  [(set_attr "simd_type" "simd_fnegabs")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "fma<mode>4"
  [(set (match_operand:VDQF 0 "register_operand" "=w")
       (fma:VDQF (match_operand:VDQF 1 "register_operand" "w")
                (match_operand:VDQF 2 "register_operand" "w")
                (match_operand:VDQF 3 "register_operand" "0")))]
  "TARGET_SIMD"
 "fmla\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_fmla")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_frint<frint_suffix><mode>"
  [(set (match_operand:VDQF 0 "register_operand" "=w")
	(unspec:VDQF [(match_operand:VDQF 1 "register_operand" "w")]
		      FRINT))]
  "TARGET_SIMD"
  "frint<frint_suffix>\\t%0.<Vtype>, %1.<Vtype>"
  [(set_attr "simd_type" "simd_frint")
   (set_attr "simd_mode" "<MODE>")]
)

;; Vector versions of the floating-point frint patterns.
;; Expands to btrunc, ceil, floor, nearbyint, rint, round.
(define_expand "<frint_pattern><mode>2"
  [(set (match_operand:VDQF 0 "register_operand")
	(unspec:VDQF [(match_operand:VDQF 1 "register_operand")]
		      FRINT))]
  "TARGET_SIMD"
  {})

(define_insn "aarch64_fcvt<frint_suffix><su><mode>"
  [(set (match_operand:<FCVT_TARGET> 0 "register_operand" "=w")
	(FIXUORS:<FCVT_TARGET> (unspec:<FCVT_TARGET>
			       [(match_operand:VDQF 1 "register_operand" "w")]
			       FCVT)))]
  "TARGET_SIMD"
  "fcvt<frint_suffix><su>\\t%0.<Vtype>, %1.<Vtype>"
  [(set_attr "simd_type" "simd_fcvti")
   (set_attr "simd_mode" "<MODE>")]
)

;; Vector versions of the fcvt standard patterns.
;; Expands to lbtrunc, lround, lceil, lfloor
(define_expand "l<fcvt_pattern><su_optab><fcvt_target><VDQF:mode>2"
  [(set (match_operand:<FCVT_TARGET> 0 "register_operand")
	(FIXUORS:<FCVT_TARGET> (unspec:<FCVT_TARGET>
			       [(match_operand:VDQF 1 "register_operand")]
			       FCVT)))]
  "TARGET_SIMD"
  {})

(define_insn "aarch64_vmls<mode>"
  [(set (match_operand:VDQF 0 "register_operand" "=w")
       (minus:VDQF (match_operand:VDQF 1 "register_operand" "0")
		   (mult:VDQF (match_operand:VDQF 2 "register_operand" "w")
			      (match_operand:VDQF 3 "register_operand" "w"))))]
  "TARGET_SIMD"
 "fmls\\t%0.<Vtype>, %2.<Vtype>, %3.<Vtype>"
  [(set_attr "simd_type" "simd_fmla")
   (set_attr "simd_mode" "<MODE>")]
)

;; FP Max/Min
;; Max/Min are introduced by idiom recognition by GCC's mid-end.  An
;; expression like:
;;      a = (b < c) ? b : c;
;; is idiom-matched as MIN_EXPR<b,c> only if -ffinite-math-only is enabled
;; either explicitly or indirectly via -ffast-math.
;;
;; MIN_EXPR and MAX_EXPR eventually map to 'smin' and 'smax' in RTL.
;; The 'smax' and 'smin' RTL standard pattern names do not specify which
;; operand will be returned when both operands are zero (i.e. they may not
;; honour signed zeroes), or when either operand is NaN.  Therefore GCC
;; only introduces MIN_EXPR/MAX_EXPR in fast math mode or when not honouring
;; NaNs.

(define_insn "smax<mode>3"
  [(set (match_operand:VDQF 0 "register_operand" "=w")
        (smax:VDQF (match_operand:VDQF 1 "register_operand" "w")
		   (match_operand:VDQF 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "fmaxnm\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_fminmax")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "smin<mode>3"
  [(set (match_operand:VDQF 0 "register_operand" "=w")
        (smin:VDQF (match_operand:VDQF 1 "register_operand" "w")
		   (match_operand:VDQF 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "fminnm\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_fminmax")
   (set_attr "simd_mode" "<MODE>")]
)

;; FP 'across lanes' max and min ops.

(define_insn "reduc_s<fmaxminv>_v4sf"
 [(set (match_operand:V4SF 0 "register_operand" "=w")
       (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "w")]
		    FMAXMINV))]
 "TARGET_SIMD"
 "f<fmaxminv>nmv\\t%s0, %1.4s";
  [(set_attr "simd_type" "simd_fminmaxv")
   (set_attr "simd_mode" "V4SF")]
)

(define_insn "reduc_s<fmaxminv>_<mode>"
 [(set (match_operand:V2F 0 "register_operand" "=w")
       (unspec:V2F [(match_operand:V2F 1 "register_operand" "w")]
		    FMAXMINV))]
 "TARGET_SIMD"
 "f<fmaxminv>nmp\\t%0.<Vtype>, %1.<Vtype>, %1.<Vtype>";
  [(set_attr "simd_type" "simd_fminmax")
   (set_attr "simd_mode" "<MODE>")]
)

;; FP 'across lanes' add.

(define_insn "aarch64_addvv4sf"
 [(set (match_operand:V4SF 0 "register_operand" "=w")
       (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "w")]
		    UNSPEC_FADDV))]
 "TARGET_SIMD"
 "faddp\\t%0.4s, %1.4s, %1.4s"
  [(set_attr "simd_type" "simd_fadd")
   (set_attr "simd_mode" "V4SF")]
)

(define_expand "reduc_uplus_v4sf"
 [(set (match_operand:V4SF 0 "register_operand" "=w")
       (match_operand:V4SF 1 "register_operand" "w"))]
 "TARGET_SIMD"
{
  rtx tmp = gen_reg_rtx (V4SFmode);
  emit_insn (gen_aarch64_addvv4sf (tmp, operands[1]));
  emit_insn (gen_aarch64_addvv4sf (operands[0], tmp));
  DONE;
})

(define_expand "reduc_splus_v4sf"
 [(set (match_operand:V4SF 0 "register_operand" "=w")
       (match_operand:V4SF 1 "register_operand" "w"))]
 "TARGET_SIMD"
{
  rtx tmp = gen_reg_rtx (V4SFmode);
  emit_insn (gen_aarch64_addvv4sf (tmp, operands[1]));
  emit_insn (gen_aarch64_addvv4sf (operands[0], tmp));
  DONE;
})

(define_insn "aarch64_addv<mode>"
 [(set (match_operand:V2F 0 "register_operand" "=w")
       (unspec:V2F [(match_operand:V2F 1 "register_operand" "w")]
		    UNSPEC_FADDV))]
 "TARGET_SIMD"
 "faddp\\t%<Vetype>0, %1.<Vtype>"
  [(set_attr "simd_type" "simd_fadd")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "reduc_uplus_<mode>"
 [(set (match_operand:V2F 0 "register_operand" "=w")
       (unspec:V2F [(match_operand:V2F 1 "register_operand" "w")]
		    UNSPEC_FADDV))]
 "TARGET_SIMD"
 ""
)

(define_expand "reduc_splus_<mode>"
 [(set (match_operand:V2F 0 "register_operand" "=w")
       (unspec:V2F [(match_operand:V2F 1 "register_operand" "w")]
		    UNSPEC_FADDV))]
 "TARGET_SIMD"
 ""
)

;; Reduction across lanes.

(define_insn "aarch64_addv<mode>"
 [(set (match_operand:VDQV 0 "register_operand" "=w")
       (unspec:VDQV [(match_operand:VDQV 1 "register_operand" "w")]
		    UNSPEC_ADDV))]
 "TARGET_SIMD"
 "addv\\t%<Vetype>0, %1.<Vtype>"
  [(set_attr "simd_type" "simd_addv")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "reduc_splus_<mode>"
 [(set (match_operand:VDQV 0 "register_operand" "=w")
       (unspec:VDQV [(match_operand:VDQV 1 "register_operand" "w")]
		    UNSPEC_ADDV))]
 "TARGET_SIMD"
 ""
)

(define_expand "reduc_uplus_<mode>"
 [(set (match_operand:VDQV 0 "register_operand" "=w")
       (unspec:VDQV [(match_operand:VDQV 1 "register_operand" "w")]
		    UNSPEC_ADDV))]
 "TARGET_SIMD"
 ""
)

(define_insn "aarch64_addvv2di"
 [(set (match_operand:V2DI 0 "register_operand" "=w")
       (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "w")]
		    UNSPEC_ADDV))]
 "TARGET_SIMD"
 "addp\\t%d0, %1.2d"
  [(set_attr "simd_type" "simd_add")
   (set_attr "simd_mode" "V2DI")]
)

(define_expand "reduc_uplus_v2di"
 [(set (match_operand:V2DI 0 "register_operand" "=w")
       (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "w")]
		    UNSPEC_ADDV))]
 "TARGET_SIMD"
 ""
)

(define_expand "reduc_splus_v2di"
 [(set (match_operand:V2DI 0 "register_operand" "=w")
       (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "w")]
		    UNSPEC_ADDV))]
 "TARGET_SIMD"
 ""
)

(define_insn "aarch64_addvv2si"
 [(set (match_operand:V2SI 0 "register_operand" "=w")
       (unspec:V2SI [(match_operand:V2SI 1 "register_operand" "w")]
		    UNSPEC_ADDV))]
 "TARGET_SIMD"
 "addp\\t%0.2s, %1.2s, %1.2s"
  [(set_attr "simd_type" "simd_add")
   (set_attr "simd_mode" "V2SI")]
)

(define_expand "reduc_uplus_v2si"
 [(set (match_operand:V2SI 0 "register_operand" "=w")
       (unspec:V2SI [(match_operand:V2SI 1 "register_operand" "w")]
		    UNSPEC_ADDV))]
 "TARGET_SIMD"
 ""
)

(define_expand "reduc_splus_v2si"
 [(set (match_operand:V2SI 0 "register_operand" "=w")
       (unspec:V2SI [(match_operand:V2SI 1 "register_operand" "w")]
		    UNSPEC_ADDV))]
 "TARGET_SIMD"
 ""
)

(define_insn "reduc_<maxminv>_<mode>"
 [(set (match_operand:VDQV 0 "register_operand" "=w")
       (unspec:VDQV [(match_operand:VDQV 1 "register_operand" "w")]
		    MAXMINV))]
 "TARGET_SIMD"
 "<maxminv>v\\t%<Vetype>0, %1.<Vtype>"
  [(set_attr "simd_type" "simd_minmaxv")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "reduc_<maxminv>_v2si"
 [(set (match_operand:V2SI 0 "register_operand" "=w")
       (unspec:V2SI [(match_operand:V2SI 1 "register_operand" "w")]
		    MAXMINV))]
 "TARGET_SIMD"
 "<maxminv>p\\t%0.2s, %1.2s, %1.2s"
  [(set_attr "simd_type" "simd_minmax")
   (set_attr "simd_mode" "V2SI")]
)

;; vbsl_* intrinsics may compile to any of bsl/bif/bit depending on register
;; allocation.  For an intrinsic of form:
;;   vD = bsl_* (vS, vN, vM)
;; We can use any of:
;;   bsl vS, vN, vM  (if D = S)
;;   bit vD, vN, vS  (if D = M, so 1-bits in vS choose bits from vN, else vM)
;;   bif vD, vM, vS  (if D = N, so 0-bits in vS choose bits from vM, else vN)

(define_insn "aarch64_simd_bsl<mode>_internal"
  [(set (match_operand:VALL 0 "register_operand"		"=w,w,w")
	(unspec:VALL
	 [(match_operand:<V_cmp_result> 1 "register_operand"	" 0,w,w")
	  (match_operand:VALL 2 "register_operand"		" w,w,0")
	  (match_operand:VALL 3 "register_operand"		" w,0,w")]
	 UNSPEC_BSL))]
  "TARGET_SIMD"
  "@
  bsl\\t%0.<Vbtype>, %2.<Vbtype>, %3.<Vbtype>
  bit\\t%0.<Vbtype>, %2.<Vbtype>, %1.<Vbtype>
  bif\\t%0.<Vbtype>, %3.<Vbtype>, %1.<Vbtype>"
)

(define_expand "aarch64_simd_bsl<mode>"
  [(set (match_operand:VALL 0 "register_operand")
	(unspec:VALL [(match_operand:<V_cmp_result> 1 "register_operand")
		      (match_operand:VALL 2 "register_operand")
		      (match_operand:VALL 3 "register_operand")]
		     UNSPEC_BSL))]
  "TARGET_SIMD"
{
  /* We can't alias operands together if they have different modes.  */
  operands[1] = gen_lowpart (<V_cmp_result>mode, operands[1]);
})

(define_expand "aarch64_vcond_internal<mode>"
  [(set (match_operand:VDQ 0 "register_operand")
	(if_then_else:VDQ
	  (match_operator 3 "comparison_operator"
	    [(match_operand:VDQ 4 "register_operand")
	     (match_operand:VDQ 5 "nonmemory_operand")])
	  (match_operand:VDQ 1 "register_operand")
	  (match_operand:VDQ 2 "register_operand")))]
  "TARGET_SIMD"
{
  int inverse = 0, has_zero_imm_form = 0;
  rtx mask = gen_reg_rtx (<MODE>mode);

  switch (GET_CODE (operands[3]))
    {
    case LE:
    case LT:
    case NE:
      inverse = 1;
      /* Fall through.  */
    case GE:
    case GT:
    case EQ:
      has_zero_imm_form = 1;
      break;
    case LEU:
    case LTU:
      inverse = 1;
      break;
    default:
      break;
    }

  if (!REG_P (operands[5])
      && (operands[5] != CONST0_RTX (<MODE>mode) || !has_zero_imm_form))
    operands[5] = force_reg (<MODE>mode, operands[5]);

  switch (GET_CODE (operands[3]))
    {
    case LT:
    case GE:
      emit_insn (gen_aarch64_cmge<mode> (mask, operands[4], operands[5]));
      break;

    case LE:
    case GT:
      emit_insn (gen_aarch64_cmgt<mode> (mask, operands[4], operands[5]));
      break;

    case LTU:
    case GEU:
      emit_insn (gen_aarch64_cmgeu<mode> (mask, operands[4], operands[5]));
      break;

    case LEU:
    case GTU:
      emit_insn (gen_aarch64_cmgtu<mode> (mask, operands[4], operands[5]));
      break;

    case NE:
    case EQ:
      emit_insn (gen_aarch64_cmeq<mode> (mask, operands[4], operands[5]));
      break;

    default:
      gcc_unreachable ();
    }

  if (inverse)
    emit_insn (gen_aarch64_simd_bsl<mode> (operands[0], mask, operands[2],
				    operands[1]));
  else
    emit_insn (gen_aarch64_simd_bsl<mode> (operands[0], mask, operands[1],
				    operands[2]));

  DONE;
})

(define_expand "aarch64_vcond_internal<mode>"
  [(set (match_operand:VDQF 0 "register_operand")
	(if_then_else:VDQF
	  (match_operator 3 "comparison_operator"
	    [(match_operand:VDQF 4 "register_operand")
	     (match_operand:VDQF 5 "nonmemory_operand")])
	  (match_operand:VDQF 1 "register_operand")
	  (match_operand:VDQF 2 "register_operand")))]
  "TARGET_SIMD"
{
  int inverse = 0;
  int use_zero_form = 0;
  int swap_bsl_operands = 0;
  rtx mask = gen_reg_rtx (<V_cmp_result>mode);
  rtx tmp = gen_reg_rtx (<V_cmp_result>mode);

  rtx (*base_comparison) (rtx, rtx, rtx);
  rtx (*complimentary_comparison) (rtx, rtx, rtx);

  switch (GET_CODE (operands[3]))
    {
    case GE:
    case GT:
    case LE:
    case LT:
    case EQ:
      if (operands[5] == CONST0_RTX (<MODE>mode))
	{
	  use_zero_form = 1;
	  break;
	}
      /* Fall through.  */
    default:
      if (!REG_P (operands[5]))
	operands[5] = force_reg (<MODE>mode, operands[5]);
    }

  switch (GET_CODE (operands[3]))
    {
    case LT:
    case UNLT:
      inverse = 1;
      /* Fall through.  */
    case GE:
    case UNGE:
    case ORDERED:
    case UNORDERED:
      base_comparison = gen_aarch64_cmge<mode>;
      complimentary_comparison = gen_aarch64_cmgt<mode>;
      break;
    case LE:
    case UNLE:
      inverse = 1;
      /* Fall through.  */
    case GT:
    case UNGT:
      base_comparison = gen_aarch64_cmgt<mode>;
      complimentary_comparison = gen_aarch64_cmge<mode>;
      break;
    case EQ:
    case NE:
    case UNEQ:
      base_comparison = gen_aarch64_cmeq<mode>;
      complimentary_comparison = gen_aarch64_cmeq<mode>;
      break;
    default:
      gcc_unreachable ();
    }

  switch (GET_CODE (operands[3]))
    {
    case LT:
    case LE:
    case GT:
    case GE:
    case EQ:
      /* The easy case.  Here we emit one of FCMGE, FCMGT or FCMEQ.
	 As a LT b <=> b GE a && a LE b <=> b GT a.  Our transformations are:
	 a GE b -> a GE b
	 a GT b -> a GT b
	 a LE b -> b GE a
	 a LT b -> b GT a
	 a EQ b -> a EQ b
	 Note that there also exist direct comparison against 0 forms,
	 so catch those as a special case.  */
      if (use_zero_form)
	{
	  inverse = 0;
	  switch (GET_CODE (operands[3]))
	    {
	    case LT:
	      base_comparison = gen_aarch64_cmlt<mode>;
	      break;
	    case LE:
	      base_comparison = gen_aarch64_cmle<mode>;
	      break;
	    default:
	      /* Do nothing, other zero form cases already have the correct
		 base_comparison.  */
	      break;
	    }
	}

      if (!inverse)
	emit_insn (base_comparison (mask, operands[4], operands[5]));
      else
	emit_insn (complimentary_comparison (mask, operands[5], operands[4]));
      break;
    case UNLT:
    case UNLE:
    case UNGT:
    case UNGE:
    case NE:
      /* FCM returns false for lanes which are unordered, so if we use
	 the inverse of the comparison we actually want to emit, then
	 swap the operands to BSL, we will end up with the correct result.
	 Note that a NE NaN and NaN NE b are true for all a, b.

	 Our transformations are:
	 a GE b -> !(b GT a)
	 a GT b -> !(b GE a)
	 a LE b -> !(a GT b)
	 a LT b -> !(a GE b)
	 a NE b -> !(a EQ b)  */

      if (inverse)
	emit_insn (base_comparison (mask, operands[4], operands[5]));
      else
	emit_insn (complimentary_comparison (mask, operands[5], operands[4]));

      swap_bsl_operands = 1;
      break;
    case UNEQ:
      /* We check (a > b ||  b > a).  combining these comparisons give us
	 true iff !(a != b && a ORDERED b), swapping the operands to BSL
	 will then give us (a == b ||  a UNORDERED b) as intended.  */

      emit_insn (gen_aarch64_cmgt<mode> (mask, operands[4], operands[5]));
      emit_insn (gen_aarch64_cmgt<mode> (tmp, operands[5], operands[4]));
      emit_insn (gen_ior<v_cmp_result>3 (mask, mask, tmp));
      swap_bsl_operands = 1;
      break;
    case UNORDERED:
       /* Operands are ORDERED iff (a > b || b >= a).
	 Swapping the operands to BSL will give the UNORDERED case.  */
     swap_bsl_operands = 1;
     /* Fall through.  */
    case ORDERED:
      emit_insn (gen_aarch64_cmgt<mode> (tmp, operands[4], operands[5]));
      emit_insn (gen_aarch64_cmge<mode> (mask, operands[5], operands[4]));
      emit_insn (gen_ior<v_cmp_result>3 (mask, mask, tmp));
      break;
    default:
      gcc_unreachable ();
    }

  if (swap_bsl_operands)
    emit_insn (gen_aarch64_simd_bsl<mode> (operands[0], mask, operands[2],
				    operands[1]));
  else
    emit_insn (gen_aarch64_simd_bsl<mode> (operands[0], mask, operands[1],
				    operands[2]));
  DONE;
})

(define_expand "vcond<mode><mode>"
  [(set (match_operand:VALL 0 "register_operand")
	(if_then_else:VALL
	  (match_operator 3 "comparison_operator"
	    [(match_operand:VALL 4 "register_operand")
	     (match_operand:VALL 5 "nonmemory_operand")])
	  (match_operand:VALL 1 "register_operand")
	  (match_operand:VALL 2 "register_operand")))]
  "TARGET_SIMD"
{
  emit_insn (gen_aarch64_vcond_internal<mode> (operands[0], operands[1],
					       operands[2], operands[3],
					       operands[4], operands[5]));
  DONE;
})


(define_expand "vcondu<mode><mode>"
  [(set (match_operand:VDQ 0 "register_operand")
	(if_then_else:VDQ
	  (match_operator 3 "comparison_operator"
	    [(match_operand:VDQ 4 "register_operand")
	     (match_operand:VDQ 5 "nonmemory_operand")])
	  (match_operand:VDQ 1 "register_operand")
	  (match_operand:VDQ 2 "register_operand")))]
  "TARGET_SIMD"
{
  emit_insn (gen_aarch64_vcond_internal<mode> (operands[0], operands[1],
					       operands[2], operands[3],
					       operands[4], operands[5]));
  DONE;
})

;; Patterns for AArch64 SIMD Intrinsics.

(define_expand "aarch64_create<mode>"
  [(match_operand:VD_RE 0 "register_operand" "")
   (match_operand:DI 1 "general_operand" "")]
  "TARGET_SIMD"
{
  rtx src = gen_lowpart (<MODE>mode, operands[1]);
  emit_move_insn (operands[0], src);
  DONE;
})

(define_insn "aarch64_get_lane_signed<mode>"
  [(set (match_operand:<VEL> 0 "register_operand" "=r")
	(sign_extend:<VEL>
	  (vec_select:<VEL>
	    (match_operand:VQ_S 1 "register_operand" "w")
	    (parallel [(match_operand:SI 2 "immediate_operand" "i")]))))]
  "TARGET_SIMD"
  "smov\\t%0, %1.<Vetype>[%2]"
  [(set_attr "simd_type" "simd_movgp")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_get_lane_unsigned<mode>"
  [(set (match_operand:<VEL> 0 "register_operand" "=r")
	(zero_extend:<VEL>
	  (vec_select:<VEL>
	    (match_operand:VDQ 1 "register_operand" "w")
	    (parallel [(match_operand:SI 2 "immediate_operand" "i")]))))]
  "TARGET_SIMD"
  "umov\\t%<vw>0, %1.<Vetype>[%2]"
  [(set_attr "simd_type" "simd_movgp")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_get_lane<mode>"
  [(set (match_operand:<VEL> 0 "register_operand" "=w")
	(vec_select:<VEL>
	    (match_operand:VDQF 1 "register_operand" "w")
	    (parallel [(match_operand:SI 2 "immediate_operand" "i")])))]
  "TARGET_SIMD"
  "mov\\t%0.<Vetype>[0], %1.<Vetype>[%2]"
  [(set_attr "simd_type" "simd_ins")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_get_lanedi"
  [(match_operand:DI 0 "register_operand" "=r")
   (match_operand:DI 1 "register_operand" "w")
   (match_operand:SI 2 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  aarch64_simd_lane_bounds (operands[2], 0, 1);
  emit_move_insn (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv8qi<mode>"
  [(match_operand:V8QI 0 "register_operand" "")
   (match_operand:VDC 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv4hi<mode>"
  [(match_operand:V4HI 0 "register_operand" "")
   (match_operand:VDC 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv2si<mode>"
  [(match_operand:V2SI 0 "register_operand" "")
   (match_operand:VDC 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv2sf<mode>"
  [(match_operand:V2SF 0 "register_operand" "")
   (match_operand:VDC 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretdi<mode>"
  [(match_operand:DI 0 "register_operand" "")
   (match_operand:VD_RE 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv16qi<mode>"
  [(match_operand:V16QI 0 "register_operand" "")
   (match_operand:VQ 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv8hi<mode>"
  [(match_operand:V8HI 0 "register_operand" "")
   (match_operand:VQ 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv4si<mode>"
  [(match_operand:V4SI 0 "register_operand" "")
   (match_operand:VQ 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv4sf<mode>"
  [(match_operand:V4SF 0 "register_operand" "")
   (match_operand:VQ 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv2di<mode>"
  [(match_operand:V2DI 0 "register_operand" "")
   (match_operand:VQ 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

(define_expand "aarch64_reinterpretv2df<mode>"
  [(match_operand:V2DF 0 "register_operand" "")
   (match_operand:VQ 1 "register_operand" "")]
  "TARGET_SIMD"
{
  aarch64_simd_reinterpret (operands[0], operands[1]);
  DONE;
})

;; In this insn, operand 1 should be low, and operand 2 the high part of the
;; dest vector.

(define_insn "*aarch64_combinez<mode>"
  [(set (match_operand:<VDBL> 0 "register_operand" "=&w")
        (vec_concat:<VDBL>
	   (match_operand:VDIC 1 "register_operand" "w")
	   (match_operand:VDIC 2 "aarch64_simd_imm_zero" "Dz")))]
  "TARGET_SIMD"
  "mov\\t%0.8b, %1.8b"
  [(set_attr "simd_type" "simd_move")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_combine<mode>"
  [(set (match_operand:<VDBL> 0 "register_operand" "=&w")
        (vec_concat:<VDBL> (match_operand:VDC 1 "register_operand" "w")
			   (match_operand:VDC 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "mov\\t%0.d[0], %1.d[0]\;ins\\t%0.d[1], %2.d[0]"
  [(set_attr "simd_type" "simd_ins")
   (set_attr "simd_mode" "<MODE>")]
)

;; <su><addsub>l<q>.

(define_insn "aarch64_<ANY_EXTEND:su><ADDSUB:optab>l2<mode>_internal"
 [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
       (ADDSUB:<VWIDE> (ANY_EXTEND:<VWIDE> (vec_select:<VHALF>
			   (match_operand:VQW 1 "register_operand" "w")
			   (match_operand:VQW 3 "vect_par_cnst_hi_half" "")))
		       (ANY_EXTEND:<VWIDE> (vec_select:<VHALF>
			   (match_operand:VQW 2 "register_operand" "w")
			   (match_dup 3)))))]
  "TARGET_SIMD"
  "<ANY_EXTEND:su><ADDSUB:optab>l2 %0.<Vwtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_addl")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_saddl2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VQW 1 "register_operand" "w")
   (match_operand:VQW 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_saddl2<mode>_internal (operands[0], operands[1],
						operands[2], p));
  DONE;
})

(define_expand "aarch64_uaddl2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VQW 1 "register_operand" "w")
   (match_operand:VQW 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_uaddl2<mode>_internal (operands[0], operands[1],
						operands[2], p));
  DONE;
})

(define_expand "aarch64_ssubl2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VQW 1 "register_operand" "w")
   (match_operand:VQW 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_ssubl2<mode>_internal (operands[0], operands[1],
						operands[2], p));
  DONE;
})

(define_expand "aarch64_usubl2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VQW 1 "register_operand" "w")
   (match_operand:VQW 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_usubl2<mode>_internal (operands[0], operands[1],
						operands[2], p));
  DONE;
})

(define_insn "aarch64_<ANY_EXTEND:su><ADDSUB:optab>l<mode>"
 [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
       (ADDSUB:<VWIDE> (ANY_EXTEND:<VWIDE>
			   (match_operand:VDW 1 "register_operand" "w"))
		       (ANY_EXTEND:<VWIDE>
			   (match_operand:VDW 2 "register_operand" "w"))))]
  "TARGET_SIMD"
  "<ANY_EXTEND:su><ADDSUB:optab>l %0.<Vwtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_addl")
   (set_attr "simd_mode" "<MODE>")]
)

;; <su><addsub>w<q>.

(define_insn "aarch64_<ANY_EXTEND:su><ADDSUB:optab>w<mode>"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ADDSUB:<VWIDE> (match_operand:<VWIDE> 1 "register_operand" "w")
			(ANY_EXTEND:<VWIDE>
			  (match_operand:VDW 2 "register_operand" "w"))))]
  "TARGET_SIMD"
  "<ANY_EXTEND:su><ADDSUB:optab>w\\t%0.<Vwtype>, %1.<Vwtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_addl")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_<ANY_EXTEND:su><ADDSUB:optab>w2<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ADDSUB:<VWIDE> (match_operand:<VWIDE> 1 "register_operand" "w")
			(ANY_EXTEND:<VWIDE>
			  (vec_select:<VHALF>
			   (match_operand:VQW 2 "register_operand" "w")
			   (match_operand:VQW 3 "vect_par_cnst_hi_half" "")))))]
  "TARGET_SIMD"
  "<ANY_EXTEND:su><ADDSUB:optab>w2\\t%0.<Vwtype>, %1.<Vwtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_addl")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_saddw2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQW 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_saddw2<mode>_internal (operands[0], operands[1],
						operands[2], p));
  DONE;
})

(define_expand "aarch64_uaddw2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQW 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_uaddw2<mode>_internal (operands[0], operands[1],
						operands[2], p));
  DONE;
})


(define_expand "aarch64_ssubw2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQW 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_ssubw2<mode>_internal (operands[0], operands[1],
						operands[2], p));
  DONE;
})

(define_expand "aarch64_usubw2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQW 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_usubw2<mode>_internal (operands[0], operands[1],
						operands[2], p));
  DONE;
})

;; <su><r>h<addsub>.

(define_insn "aarch64_<sur>h<addsub><mode>"
  [(set (match_operand:VQ_S 0 "register_operand" "=w")
        (unspec:VQ_S [(match_operand:VQ_S 1 "register_operand" "w")
		      (match_operand:VQ_S 2 "register_operand" "w")]
		     HADDSUB))]
  "TARGET_SIMD"
  "<sur>h<addsub>\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_add")
   (set_attr "simd_mode" "<MODE>")]
)

;; <r><addsub>hn<q>.

(define_insn "aarch64_<sur><addsub>hn<mode>"
  [(set (match_operand:<VNARROWQ> 0 "register_operand" "=w")
        (unspec:<VNARROWQ> [(match_operand:VQN 1 "register_operand" "w")
			    (match_operand:VQN 2 "register_operand" "w")]
                           ADDSUBHN))]
  "TARGET_SIMD"
  "<sur><addsub>hn\\t%0.<Vntype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_addn")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_<sur><addsub>hn2<mode>"
  [(set (match_operand:<VNARROWQ2> 0 "register_operand" "=w")
        (unspec:<VNARROWQ2> [(match_operand:<VNARROWQ> 1 "register_operand" "0")
			     (match_operand:VQN 2 "register_operand" "w")
			     (match_operand:VQN 3 "register_operand" "w")]
                            ADDSUBHN2))]
  "TARGET_SIMD"
  "<sur><addsub>hn2\\t%0.<V2ntype>, %2.<Vtype>, %3.<Vtype>"
  [(set_attr "simd_type" "simd_addn2")
   (set_attr "simd_mode" "<MODE>")]
)

;; pmul.

(define_insn "aarch64_pmul<mode>"
  [(set (match_operand:VB 0 "register_operand" "=w")
        (unspec:VB [(match_operand:VB 1 "register_operand" "w")
		    (match_operand:VB 2 "register_operand" "w")]
		   UNSPEC_PMUL))]
 "TARGET_SIMD"
 "pmul\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_mul")
   (set_attr "simd_mode" "<MODE>")]
)

;; <su>q<addsub>

(define_insn "aarch64_<su_optab><optab><mode>"
  [(set (match_operand:VSDQ_I 0 "register_operand" "=w")
	(BINQOPS:VSDQ_I (match_operand:VSDQ_I 1 "register_operand" "w")
			  (match_operand:VSDQ_I 2 "register_operand" "w")))]
  "TARGET_SIMD"
  "<su_optab><optab>\\t%<v>0<Vmtype>, %<v>1<Vmtype>, %<v>2<Vmtype>"
  [(set_attr "simd_type" "simd_add")
   (set_attr "simd_mode" "<MODE>")]
)

;; suqadd and usqadd

(define_insn "aarch64_<sur>qadd<mode>"
  [(set (match_operand:VSDQ_I 0 "register_operand" "=w")
	(unspec:VSDQ_I [(match_operand:VSDQ_I 1 "register_operand" "0")
			(match_operand:VSDQ_I 2 "register_operand" "w")]
		       USSUQADD))]
  "TARGET_SIMD"
  "<sur>qadd\\t%<v>0<Vmtype>, %<v>2<Vmtype>"
  [(set_attr "simd_type" "simd_sat_add")
   (set_attr "simd_mode" "<MODE>")]
)

;; sqmovun

(define_insn "aarch64_sqmovun<mode>"
  [(set (match_operand:<VNARROWQ> 0 "register_operand" "=w")
	(unspec:<VNARROWQ> [(match_operand:VSQN_HSDI 1 "register_operand" "w")]
                            UNSPEC_SQXTUN))]
   "TARGET_SIMD"
   "sqxtun\\t%<vn2>0<Vmntype>, %<v>1<Vmtype>"
   [(set_attr "simd_type" "simd_sat_shiftn_imm")
    (set_attr "simd_mode" "<MODE>")]
 )

;; sqmovn and uqmovn

(define_insn "aarch64_<sur>qmovn<mode>"
  [(set (match_operand:<VNARROWQ> 0 "register_operand" "=w")
	(unspec:<VNARROWQ> [(match_operand:VSQN_HSDI 1 "register_operand" "w")]
                            SUQMOVN))]
  "TARGET_SIMD"
  "<sur>qxtn\\t%<vn2>0<Vmntype>, %<v>1<Vmtype>"
   [(set_attr "simd_type" "simd_sat_shiftn_imm")
    (set_attr "simd_mode" "<MODE>")]
 )

;; <su>q<absneg>

(define_insn "aarch64_s<optab><mode>"
  [(set (match_operand:VSDQ_I_BHSI 0 "register_operand" "=w")
	(UNQOPS:VSDQ_I_BHSI
	  (match_operand:VSDQ_I_BHSI 1 "register_operand" "w")))]
  "TARGET_SIMD"
  "s<optab>\\t%<v>0<Vmtype>, %<v>1<Vmtype>"
  [(set_attr "simd_type" "simd_sat_negabs")
   (set_attr "simd_mode" "<MODE>")]
)

;; sq<r>dmulh.

(define_insn "aarch64_sq<r>dmulh<mode>"
  [(set (match_operand:VSDQ_HSI 0 "register_operand" "=w")
	(unspec:VSDQ_HSI
	  [(match_operand:VSDQ_HSI 1 "register_operand" "w")
	   (match_operand:VSDQ_HSI 2 "register_operand" "w")]
	 VQDMULH))]
  "TARGET_SIMD"
  "sq<r>dmulh\\t%<v>0<Vmtype>, %<v>1<Vmtype>, %<v>2<Vmtype>"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

;; sq<r>dmulh_lane

(define_insn "aarch64_sq<r>dmulh_lane<mode>"
  [(set (match_operand:VDQHS 0 "register_operand" "=w")
        (unspec:VDQHS
	  [(match_operand:VDQHS 1 "register_operand" "w")
           (vec_select:<VEL>
             (match_operand:<VCOND> 2 "register_operand" "<vwx>")
             (parallel [(match_operand:SI 3 "immediate_operand" "i")]))]
	 VQDMULH))]
  "TARGET_SIMD"
  "*
   aarch64_simd_lane_bounds (operands[3], 0, GET_MODE_NUNITS (<VCOND>mode));
   return \"sq<r>dmulh\\t%0.<Vtype>, %1.<Vtype>, %2.<Vetype>[%3]\";"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_sq<r>dmulh_laneq<mode>"
  [(set (match_operand:VDQHS 0 "register_operand" "=w")
        (unspec:VDQHS
	  [(match_operand:VDQHS 1 "register_operand" "w")
           (vec_select:<VEL>
             (match_operand:<VCONQ> 2 "register_operand" "<vwx>")
             (parallel [(match_operand:SI 3 "immediate_operand" "i")]))]
	 VQDMULH))]
  "TARGET_SIMD"
  "*
   aarch64_simd_lane_bounds (operands[3], 0, GET_MODE_NUNITS (<VCONQ>mode));
   return \"sq<r>dmulh\\t%0.<Vtype>, %1.<Vtype>, %2.<Vetype>[%3]\";"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_sq<r>dmulh_lane<mode>"
  [(set (match_operand:SD_HSI 0 "register_operand" "=w")
        (unspec:SD_HSI
	  [(match_operand:SD_HSI 1 "register_operand" "w")
           (vec_select:<VEL>
             (match_operand:<VCONQ> 2 "register_operand" "<vwx>")
             (parallel [(match_operand:SI 3 "immediate_operand" "i")]))]
	 VQDMULH))]
  "TARGET_SIMD"
  "*
   aarch64_simd_lane_bounds (operands[3], 0, GET_MODE_NUNITS (<VCONQ>mode));
   return \"sq<r>dmulh\\t%<v>0, %<v>1, %2.<v>[%3]\";"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

;; vqdml[sa]l

(define_insn "aarch64_sqdml<SBINQOPS:as>l<mode>"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (SBINQOPS:<VWIDE>
	  (match_operand:<VWIDE> 1 "register_operand" "0")
	  (ss_ashift:<VWIDE>
	      (mult:<VWIDE>
		(sign_extend:<VWIDE>
		      (match_operand:VSD_HSI 2 "register_operand" "w"))
		(sign_extend:<VWIDE>
		      (match_operand:VSD_HSI 3 "register_operand" "w")))
	      (const_int 1))))]
  "TARGET_SIMD"
  "sqdml<SBINQOPS:as>l\\t%<vw2>0<Vmwtype>, %<v>2<Vmtype>, %<v>3<Vmtype>"
  [(set_attr "simd_type" "simd_sat_mlal")
   (set_attr "simd_mode" "<MODE>")]
)

;; vqdml[sa]l_lane

(define_insn "aarch64_sqdml<SBINQOPS:as>l_lane<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (SBINQOPS:<VWIDE>
	  (match_operand:<VWIDE> 1 "register_operand" "0")
	  (ss_ashift:<VWIDE>
	    (mult:<VWIDE>
	      (sign_extend:<VWIDE>
		(match_operand:VD_HSI 2 "register_operand" "w"))
	      (sign_extend:<VWIDE>
		(vec_duplicate:VD_HSI
		  (vec_select:<VEL>
		    (match_operand:<VCON> 3 "register_operand" "<vwx>")
		    (parallel [(match_operand:SI 4 "immediate_operand" "i")])))
              ))
	    (const_int 1))))]
  "TARGET_SIMD"
  "sqdml<SBINQOPS:as>l\\t%<vw2>0<Vmwtype>, %<v>2<Vmtype>, %3.<Vetype>[%4]"
  [(set_attr "simd_type" "simd_sat_mlal")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_sqdml<SBINQOPS:as>l_lane<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (SBINQOPS:<VWIDE>
	  (match_operand:<VWIDE> 1 "register_operand" "0")
	  (ss_ashift:<VWIDE>
	    (mult:<VWIDE>
	      (sign_extend:<VWIDE>
		(match_operand:SD_HSI 2 "register_operand" "w"))
	      (sign_extend:<VWIDE>
		(vec_select:<VEL>
		  (match_operand:<VCON> 3 "register_operand" "<vwx>")
		  (parallel [(match_operand:SI 4 "immediate_operand" "i")])))
              )
	    (const_int 1))))]
  "TARGET_SIMD"
  "sqdml<SBINQOPS:as>l\\t%<vw2>0<Vmwtype>, %<v>2<Vmtype>, %3.<Vetype>[%4]"
  [(set_attr "simd_type" "simd_sat_mlal")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_sqdmlal_lane<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "0")
   (match_operand:VSD_HSI 2 "register_operand" "w")
   (match_operand:<VCON> 3 "register_operand" "<vwx>")
   (match_operand:SI 4 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  aarch64_simd_lane_bounds (operands[4], 0, GET_MODE_NUNITS (<VCON>mode) / 2);
  emit_insn (gen_aarch64_sqdmlal_lane<mode>_internal (operands[0], operands[1],
						      operands[2], operands[3],
						      operands[4]));
  DONE;
})

(define_expand "aarch64_sqdmlal_laneq<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "0")
   (match_operand:VSD_HSI 2 "register_operand" "w")
   (match_operand:<VCON> 3 "register_operand" "<vwx>")
   (match_operand:SI 4 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  aarch64_simd_lane_bounds (operands[4], 0, GET_MODE_NUNITS (<VCON>mode));
  emit_insn (gen_aarch64_sqdmlal_lane<mode>_internal (operands[0], operands[1],
						      operands[2], operands[3],
						      operands[4]));
  DONE;
})

(define_expand "aarch64_sqdmlsl_lane<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "0")
   (match_operand:VSD_HSI 2 "register_operand" "w")
   (match_operand:<VCON> 3 "register_operand" "<vwx>")
   (match_operand:SI 4 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  aarch64_simd_lane_bounds (operands[4], 0, GET_MODE_NUNITS (<VCON>mode) / 2);
  emit_insn (gen_aarch64_sqdmlsl_lane<mode>_internal (operands[0], operands[1],
						      operands[2], operands[3],
						      operands[4]));
  DONE;
})

(define_expand "aarch64_sqdmlsl_laneq<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "0")
   (match_operand:VSD_HSI 2 "register_operand" "w")
   (match_operand:<VCON> 3 "register_operand" "<vwx>")
   (match_operand:SI 4 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  aarch64_simd_lane_bounds (operands[4], 0, GET_MODE_NUNITS (<VCON>mode));
  emit_insn (gen_aarch64_sqdmlsl_lane<mode>_internal (operands[0], operands[1],
						      operands[2], operands[3],
						      operands[4]));
  DONE;
})

;; vqdml[sa]l_n

(define_insn "aarch64_sqdml<SBINQOPS:as>l_n<mode>"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (SBINQOPS:<VWIDE>
	  (match_operand:<VWIDE> 1 "register_operand" "0")
	  (ss_ashift:<VWIDE>
	      (mult:<VWIDE>
		(sign_extend:<VWIDE>
		      (match_operand:VD_HSI 2 "register_operand" "w"))
		(sign_extend:<VWIDE>
		  (vec_duplicate:VD_HSI
		    (match_operand:<VEL> 3 "register_operand" "w"))))
	      (const_int 1))))]
  "TARGET_SIMD"
  "sqdml<SBINQOPS:as>l\\t%<vw2>0<Vmwtype>, %<v>2<Vmtype>, %3.<Vetype>[0]"
  [(set_attr "simd_type" "simd_sat_mlal")
   (set_attr "simd_mode" "<MODE>")]
)

;; sqdml[as]l2

(define_insn "aarch64_sqdml<SBINQOPS:as>l2<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (SBINQOPS:<VWIDE>
         (match_operand:<VWIDE> 1 "register_operand" "0")
         (ss_ashift:<VWIDE>
             (mult:<VWIDE>
               (sign_extend:<VWIDE>
                 (vec_select:<VHALF>
                     (match_operand:VQ_HSI 2 "register_operand" "w")
                     (match_operand:VQ_HSI 4 "vect_par_cnst_hi_half" "")))
               (sign_extend:<VWIDE>
                 (vec_select:<VHALF>
                     (match_operand:VQ_HSI 3 "register_operand" "w")
                     (match_dup 4))))
             (const_int 1))))]
  "TARGET_SIMD"
  "sqdml<SBINQOPS:as>l2\\t%<vw2>0<Vmwtype>, %<v>2<Vmtype>, %<v>3<Vmtype>"
  [(set_attr "simd_type" "simd_sat_mlal")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_sqdmlal2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQ_HSI 2 "register_operand" "w")
   (match_operand:VQ_HSI 3 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_sqdmlal2<mode>_internal (operands[0], operands[1],
						  operands[2], operands[3], p));
  DONE;
})

(define_expand "aarch64_sqdmlsl2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQ_HSI 2 "register_operand" "w")
   (match_operand:VQ_HSI 3 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_sqdmlsl2<mode>_internal (operands[0], operands[1],
						  operands[2], operands[3], p));
  DONE;
})

;; vqdml[sa]l2_lane

(define_insn "aarch64_sqdml<SBINQOPS:as>l2_lane<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (SBINQOPS:<VWIDE>
	  (match_operand:<VWIDE> 1 "register_operand" "0")
	  (ss_ashift:<VWIDE>
	      (mult:<VWIDE>
		(sign_extend:<VWIDE>
                  (vec_select:<VHALF>
                    (match_operand:VQ_HSI 2 "register_operand" "w")
                    (match_operand:VQ_HSI 5 "vect_par_cnst_hi_half" "")))
		(sign_extend:<VWIDE>
                  (vec_duplicate:<VHALF>
		    (vec_select:<VEL>
		      (match_operand:<VCON> 3 "register_operand" "<vwx>")
		      (parallel [(match_operand:SI 4 "immediate_operand" "i")])
		    ))))
	      (const_int 1))))]
  "TARGET_SIMD"
  "sqdml<SBINQOPS:as>l2\\t%<vw2>0<Vmwtype>, %<v>2<Vmtype>, %3.<Vetype>[%4]"
  [(set_attr "simd_type" "simd_sat_mlal")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_sqdmlal2_lane<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQ_HSI 2 "register_operand" "w")
   (match_operand:<VCON> 3 "register_operand" "<vwx>")
   (match_operand:SI 4 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  aarch64_simd_lane_bounds (operands[4], 0, GET_MODE_NUNITS (<MODE>mode) / 2);
  emit_insn (gen_aarch64_sqdmlal2_lane<mode>_internal (operands[0], operands[1],
						       operands[2], operands[3],
						       operands[4], p));
  DONE;
})

(define_expand "aarch64_sqdmlal2_laneq<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQ_HSI 2 "register_operand" "w")
   (match_operand:<VCON> 3 "register_operand" "<vwx>")
   (match_operand:SI 4 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  aarch64_simd_lane_bounds (operands[4], 0, GET_MODE_NUNITS (<MODE>mode));
  emit_insn (gen_aarch64_sqdmlal2_lane<mode>_internal (operands[0], operands[1],
						       operands[2], operands[3],
						       operands[4], p));
  DONE;
})

(define_expand "aarch64_sqdmlsl2_lane<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQ_HSI 2 "register_operand" "w")
   (match_operand:<VCON> 3 "register_operand" "<vwx>")
   (match_operand:SI 4 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  aarch64_simd_lane_bounds (operands[4], 0, GET_MODE_NUNITS (<MODE>mode) / 2);
  emit_insn (gen_aarch64_sqdmlsl2_lane<mode>_internal (operands[0], operands[1],
						       operands[2], operands[3],
						       operands[4], p));
  DONE;
})

(define_expand "aarch64_sqdmlsl2_laneq<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQ_HSI 2 "register_operand" "w")
   (match_operand:<VCON> 3 "register_operand" "<vwx>")
   (match_operand:SI 4 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  aarch64_simd_lane_bounds (operands[4], 0, GET_MODE_NUNITS (<MODE>mode));
  emit_insn (gen_aarch64_sqdmlsl2_lane<mode>_internal (operands[0], operands[1],
						       operands[2], operands[3],
						       operands[4], p));
  DONE;
})

(define_insn "aarch64_sqdml<SBINQOPS:as>l2_n<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (SBINQOPS:<VWIDE>
	  (match_operand:<VWIDE> 1 "register_operand" "0")
	  (ss_ashift:<VWIDE>
	    (mult:<VWIDE>
	      (sign_extend:<VWIDE>
                (vec_select:<VHALF>
                  (match_operand:VQ_HSI 2 "register_operand" "w")
                  (match_operand:VQ_HSI 4 "vect_par_cnst_hi_half" "")))
	      (sign_extend:<VWIDE>
                (vec_duplicate:<VHALF>
		  (match_operand:<VEL> 3 "register_operand" "w"))))
	    (const_int 1))))]
  "TARGET_SIMD"
  "sqdml<SBINQOPS:as>l2\\t%<vw2>0<Vmwtype>, %<v>2<Vmtype>, %3.<Vetype>[0]"
  [(set_attr "simd_type" "simd_sat_mlal")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_sqdmlal2_n<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQ_HSI 2 "register_operand" "w")
   (match_operand:<VEL> 3 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_sqdmlal2_n<mode>_internal (operands[0], operands[1],
						    operands[2], operands[3],
						    p));
  DONE;
})

(define_expand "aarch64_sqdmlsl2_n<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:<VWIDE> 1 "register_operand" "w")
   (match_operand:VQ_HSI 2 "register_operand" "w")
   (match_operand:<VEL> 3 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_sqdmlsl2_n<mode>_internal (operands[0], operands[1],
						    operands[2], operands[3],
						    p));
  DONE;
})

;; vqdmull

(define_insn "aarch64_sqdmull<mode>"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ss_ashift:<VWIDE>
	     (mult:<VWIDE>
	       (sign_extend:<VWIDE>
		     (match_operand:VSD_HSI 1 "register_operand" "w"))
	       (sign_extend:<VWIDE>
		     (match_operand:VSD_HSI 2 "register_operand" "w")))
	     (const_int 1)))]
  "TARGET_SIMD"
  "sqdmull\\t%<vw2>0<Vmwtype>, %<v>1<Vmtype>, %<v>2<Vmtype>"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

;; vqdmull_lane

(define_insn "aarch64_sqdmull_lane<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ss_ashift:<VWIDE>
	     (mult:<VWIDE>
	       (sign_extend:<VWIDE>
		 (match_operand:VD_HSI 1 "register_operand" "w"))
	       (sign_extend:<VWIDE>
                 (vec_duplicate:VD_HSI
                   (vec_select:<VEL>
		     (match_operand:<VCON> 2 "register_operand" "<vwx>")
		     (parallel [(match_operand:SI 3 "immediate_operand" "i")])))
	       ))
	     (const_int 1)))]
  "TARGET_SIMD"
  "sqdmull\\t%<vw2>0<Vmwtype>, %<v>1<Vmtype>, %2.<Vetype>[%3]"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_sqdmull_lane<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ss_ashift:<VWIDE>
	     (mult:<VWIDE>
	       (sign_extend:<VWIDE>
		 (match_operand:SD_HSI 1 "register_operand" "w"))
	       (sign_extend:<VWIDE>
                 (vec_select:<VEL>
		   (match_operand:<VCON> 2 "register_operand" "<vwx>")
		   (parallel [(match_operand:SI 3 "immediate_operand" "i")]))
	       ))
	     (const_int 1)))]
  "TARGET_SIMD"
  "sqdmull\\t%<vw2>0<Vmwtype>, %<v>1<Vmtype>, %2.<Vetype>[%3]"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_sqdmull_lane<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VSD_HSI 1 "register_operand" "w")
   (match_operand:<VCON> 2 "register_operand" "<vwx>")
   (match_operand:SI 3 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  aarch64_simd_lane_bounds (operands[3], 0, GET_MODE_NUNITS (<VCON>mode) / 2);
  emit_insn (gen_aarch64_sqdmull_lane<mode>_internal (operands[0], operands[1],
						      operands[2], operands[3]));
  DONE;
})

(define_expand "aarch64_sqdmull_laneq<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VD_HSI 1 "register_operand" "w")
   (match_operand:<VCON> 2 "register_operand" "<vwx>")
   (match_operand:SI 3 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  aarch64_simd_lane_bounds (operands[3], 0, GET_MODE_NUNITS (<VCON>mode));
  emit_insn (gen_aarch64_sqdmull_lane<mode>_internal
	       (operands[0], operands[1], operands[2], operands[3]));
  DONE;
})

;; vqdmull_n

(define_insn "aarch64_sqdmull_n<mode>"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ss_ashift:<VWIDE>
	     (mult:<VWIDE>
	       (sign_extend:<VWIDE>
		 (match_operand:VD_HSI 1 "register_operand" "w"))
	       (sign_extend:<VWIDE>
                 (vec_duplicate:VD_HSI
                   (match_operand:<VEL> 2 "register_operand" "w")))
	       )
	     (const_int 1)))]
  "TARGET_SIMD"
  "sqdmull\\t%<vw2>0<Vmwtype>, %<v>1<Vmtype>, %2.<Vetype>[0]"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

;; vqdmull2



(define_insn "aarch64_sqdmull2<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ss_ashift:<VWIDE>
	     (mult:<VWIDE>
	       (sign_extend:<VWIDE>
		 (vec_select:<VHALF>
                   (match_operand:VQ_HSI 1 "register_operand" "w")
                   (match_operand:VQ_HSI 3 "vect_par_cnst_hi_half" "")))
	       (sign_extend:<VWIDE>
		 (vec_select:<VHALF>
                   (match_operand:VQ_HSI 2 "register_operand" "w")
                   (match_dup 3)))
	       )
	     (const_int 1)))]
  "TARGET_SIMD"
  "sqdmull2\\t%<vw2>0<Vmwtype>, %<v>1<Vmtype>, %<v>2<Vmtype>"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_sqdmull2<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VQ_HSI 1 "register_operand" "w")
   (match_operand:<VCON> 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_sqdmull2<mode>_internal (operands[0], operands[1],
						  operands[2], p));
  DONE;
})

;; vqdmull2_lane

(define_insn "aarch64_sqdmull2_lane<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ss_ashift:<VWIDE>
	     (mult:<VWIDE>
	       (sign_extend:<VWIDE>
		 (vec_select:<VHALF>
                   (match_operand:VQ_HSI 1 "register_operand" "w")
                   (match_operand:VQ_HSI 4 "vect_par_cnst_hi_half" "")))
	       (sign_extend:<VWIDE>
                 (vec_duplicate:<VHALF>
                   (vec_select:<VEL>
		     (match_operand:<VCON> 2 "register_operand" "<vwx>")
		     (parallel [(match_operand:SI 3 "immediate_operand" "i")])))
	       ))
	     (const_int 1)))]
  "TARGET_SIMD"
  "sqdmull2\\t%<vw2>0<Vmwtype>, %<v>1<Vmtype>, %2.<Vetype>[%3]"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_sqdmull2_lane<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VQ_HSI 1 "register_operand" "w")
   (match_operand:<VCON> 2 "register_operand" "<vwx>")
   (match_operand:SI 3 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  aarch64_simd_lane_bounds (operands[3], 0, GET_MODE_NUNITS (<MODE>mode) / 2);
  emit_insn (gen_aarch64_sqdmull2_lane<mode>_internal (operands[0], operands[1],
						       operands[2], operands[3],
						       p));
  DONE;
})

(define_expand "aarch64_sqdmull2_laneq<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VQ_HSI 1 "register_operand" "w")
   (match_operand:<VCON> 2 "register_operand" "<vwx>")
   (match_operand:SI 3 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  aarch64_simd_lane_bounds (operands[3], 0, GET_MODE_NUNITS (<MODE>mode));
  emit_insn (gen_aarch64_sqdmull2_lane<mode>_internal (operands[0], operands[1],
						       operands[2], operands[3],
						       p));
  DONE;
})

;; vqdmull2_n

(define_insn "aarch64_sqdmull2_n<mode>_internal"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
        (ss_ashift:<VWIDE>
	     (mult:<VWIDE>
	       (sign_extend:<VWIDE>
		 (vec_select:<VHALF>
                   (match_operand:VQ_HSI 1 "register_operand" "w")
                   (match_operand:VQ_HSI 3 "vect_par_cnst_hi_half" "")))
	       (sign_extend:<VWIDE>
                 (vec_duplicate:<VHALF>
                   (match_operand:<VEL> 2 "register_operand" "w")))
	       )
	     (const_int 1)))]
  "TARGET_SIMD"
  "sqdmull2\\t%<vw2>0<Vmwtype>, %<v>1<Vmtype>, %2.<Vetype>[0]"
  [(set_attr "simd_type" "simd_sat_mul")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_sqdmull2_n<mode>"
  [(match_operand:<VWIDE> 0 "register_operand" "=w")
   (match_operand:VQ_HSI 1 "register_operand" "w")
   (match_operand:<VEL> 2 "register_operand" "w")]
  "TARGET_SIMD"
{
  rtx p = aarch64_simd_vect_par_cnst_half (<MODE>mode, true);
  emit_insn (gen_aarch64_sqdmull2_n<mode>_internal (operands[0], operands[1],
						    operands[2], p));
  DONE;
})

;; vshl

(define_insn "aarch64_<sur>shl<mode>"
  [(set (match_operand:VSDQ_I_DI 0 "register_operand" "=w")
        (unspec:VSDQ_I_DI
	  [(match_operand:VSDQ_I_DI 1 "register_operand" "w")
           (match_operand:VSDQ_I_DI 2 "register_operand" "w")]
         VSHL))]
  "TARGET_SIMD"
  "<sur>shl\\t%<v>0<Vmtype>, %<v>1<Vmtype>, %<v>2<Vmtype>";
  [(set_attr "simd_type" "simd_shift")
   (set_attr "simd_mode" "<MODE>")]
)


;; vqshl

(define_insn "aarch64_<sur>q<r>shl<mode>"
  [(set (match_operand:VSDQ_I 0 "register_operand" "=w")
        (unspec:VSDQ_I
	  [(match_operand:VSDQ_I 1 "register_operand" "w")
           (match_operand:VSDQ_I 2 "register_operand" "w")]
         VQSHL))]
  "TARGET_SIMD"
  "<sur>q<r>shl\\t%<v>0<Vmtype>, %<v>1<Vmtype>, %<v>2<Vmtype>";
  [(set_attr "simd_type" "simd_sat_shift")
   (set_attr "simd_mode" "<MODE>")]
)

;; vshl_n

(define_expand "aarch64_sshl_n<mode>"
  [(match_operand:VSDQ_I_DI 0 "register_operand" "=w")
   (match_operand:VSDQ_I_DI 1 "register_operand" "w")
   (match_operand:SI 2 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  emit_insn (gen_ashl<mode>3 (operands[0], operands[1], operands[2]));
  DONE;
})

(define_expand "aarch64_ushl_n<mode>"
  [(match_operand:VSDQ_I_DI 0 "register_operand" "=w")
   (match_operand:VSDQ_I_DI 1 "register_operand" "w")
   (match_operand:SI 2 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  emit_insn (gen_ashl<mode>3 (operands[0], operands[1], operands[2]));
  DONE;
})

;; vshll_n

(define_insn "aarch64_<sur>shll_n<mode>"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
	(unspec:<VWIDE> [(match_operand:VDW 1 "register_operand" "w")
			 (match_operand:SI 2 "immediate_operand" "i")]
                         VSHLL))]
  "TARGET_SIMD"
  "*
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  aarch64_simd_const_bounds (operands[2], 0, bit_width + 1);
  if (INTVAL (operands[2]) == bit_width)
  {
    return \"shll\\t%0.<Vwtype>, %1.<Vtype>, %2\";
  }
  else {
    return \"<sur>shll\\t%0.<Vwtype>, %1.<Vtype>, %2\";
  }"
  [(set_attr "simd_type" "simd_shift_imm")
   (set_attr "simd_mode" "<MODE>")]
)

;; vshll_high_n

(define_insn "aarch64_<sur>shll2_n<mode>"
  [(set (match_operand:<VWIDE> 0 "register_operand" "=w")
	(unspec:<VWIDE> [(match_operand:VQW 1 "register_operand" "w")
			 (match_operand:SI 2 "immediate_operand" "i")]
                         VSHLL))]
  "TARGET_SIMD"
  "*
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  aarch64_simd_const_bounds (operands[2], 0, bit_width + 1);
  if (INTVAL (operands[2]) == bit_width)
  {
    return \"shll2\\t%0.<Vwtype>, %1.<Vtype>, %2\";
  }
  else {
    return \"<sur>shll2\\t%0.<Vwtype>, %1.<Vtype>, %2\";
  }"
  [(set_attr "simd_type" "simd_shift_imm")
   (set_attr "simd_mode" "<MODE>")]
)

;; vshr_n

(define_expand "aarch64_sshr_n<mode>"
  [(match_operand:VSDQ_I_DI 0 "register_operand" "=w")
   (match_operand:VSDQ_I_DI 1 "register_operand" "w")
   (match_operand:SI 2 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  emit_insn (gen_ashr<mode>3 (operands[0], operands[1], operands[2]));
  DONE;
})

(define_expand "aarch64_ushr_n<mode>"
  [(match_operand:VSDQ_I_DI 0 "register_operand" "=w")
   (match_operand:VSDQ_I_DI 1 "register_operand" "w")
   (match_operand:SI 2 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  emit_insn (gen_lshr<mode>3 (operands[0], operands[1], operands[2]));
  DONE;
})

;; vrshr_n

(define_insn "aarch64_<sur>shr_n<mode>"
  [(set (match_operand:VSDQ_I_DI 0 "register_operand" "=w")
        (unspec:VSDQ_I_DI [(match_operand:VSDQ_I_DI 1 "register_operand" "w")
			   (match_operand:SI 2 "immediate_operand" "i")]
			  VRSHR_N))]
  "TARGET_SIMD"
  "*
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  aarch64_simd_const_bounds (operands[2], 1, bit_width + 1);
  return \"<sur>shr\\t%<v>0<Vmtype>, %<v>1<Vmtype>, %2\";"
  [(set_attr "simd_type" "simd_shift_imm")
   (set_attr "simd_mode" "<MODE>")]
)

;; v(r)sra_n

(define_insn "aarch64_<sur>sra_n<mode>"
  [(set (match_operand:VSDQ_I_DI 0 "register_operand" "=w")
	(unspec:VSDQ_I_DI [(match_operand:VSDQ_I_DI 1 "register_operand" "0")
		       (match_operand:VSDQ_I_DI 2 "register_operand" "w")
                       (match_operand:SI 3 "immediate_operand" "i")]
                      VSRA))]
  "TARGET_SIMD"
  "*
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  aarch64_simd_const_bounds (operands[3], 1, bit_width + 1);
  return \"<sur>sra\\t%<v>0<Vmtype>, %<v>2<Vmtype>, %3\";"
  [(set_attr "simd_type" "simd_shift_imm_acc")
   (set_attr "simd_mode" "<MODE>")]
)

;; vs<lr>i_n

(define_insn "aarch64_<sur>s<lr>i_n<mode>"
  [(set (match_operand:VSDQ_I_DI 0 "register_operand" "=w")
	(unspec:VSDQ_I_DI [(match_operand:VSDQ_I_DI 1 "register_operand" "0")
		       (match_operand:VSDQ_I_DI 2 "register_operand" "w")
                       (match_operand:SI 3 "immediate_operand" "i")]
                      VSLRI))]
  "TARGET_SIMD"
  "*
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  aarch64_simd_const_bounds (operands[3], 1 - <VSLRI:offsetlr>,
                             bit_width - <VSLRI:offsetlr> + 1);
  return \"s<lr>i\\t%<v>0<Vmtype>, %<v>2<Vmtype>, %3\";"
  [(set_attr "simd_type" "simd_shift_imm")
   (set_attr "simd_mode" "<MODE>")]
)

;; vqshl(u)

(define_insn "aarch64_<sur>qshl<u>_n<mode>"
  [(set (match_operand:VSDQ_I 0 "register_operand" "=w")
	(unspec:VSDQ_I [(match_operand:VSDQ_I 1 "register_operand" "w")
		       (match_operand:SI 2 "immediate_operand" "i")]
                      VQSHL_N))]
  "TARGET_SIMD"
  "*
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  aarch64_simd_const_bounds (operands[2], 0, bit_width);
  return \"<sur>qshl<u>\\t%<v>0<Vmtype>, %<v>1<Vmtype>, %2\";"
  [(set_attr "simd_type" "simd_sat_shift_imm")
   (set_attr "simd_mode" "<MODE>")]
)


;; vq(r)shr(u)n_n

(define_insn "aarch64_<sur>q<r>shr<u>n_n<mode>"
  [(set (match_operand:<VNARROWQ> 0 "register_operand" "=w")
        (unspec:<VNARROWQ> [(match_operand:VSQN_HSDI 1 "register_operand" "w")
			    (match_operand:SI 2 "immediate_operand" "i")]
			   VQSHRN_N))]
  "TARGET_SIMD"
  "*
  int bit_width = GET_MODE_UNIT_SIZE (<MODE>mode) * BITS_PER_UNIT;
  aarch64_simd_const_bounds (operands[2], 1, bit_width + 1);
  return \"<sur>q<r>shr<u>n\\t%<vn2>0<Vmntype>, %<v>1<Vmtype>, %2\";"
  [(set_attr "simd_type" "simd_sat_shiftn_imm")
   (set_attr "simd_mode" "<MODE>")]
)


;; cm(eq|ge|gt|lt|le)
;; Note, we have constraints for Dz and Z as different expanders
;; have different ideas of what should be passed to this pattern.

(define_insn "aarch64_cm<optab><mode>"
  [(set (match_operand:<V_cmp_result> 0 "register_operand" "=w,w")
	(neg:<V_cmp_result>
	  (COMPARISONS:<V_cmp_result>
	    (match_operand:VDQ 1 "register_operand" "w,w")
	    (match_operand:VDQ 2 "aarch64_simd_reg_or_zero" "w,ZDz")
	  )))]
  "TARGET_SIMD"
  "@
  cm<n_optab>\t%<v>0<Vmtype>, %<v><cmp_1><Vmtype>, %<v><cmp_2><Vmtype>
  cm<optab>\t%<v>0<Vmtype>, %<v>1<Vmtype>, #0"
  [(set_attr "simd_type" "simd_cmp")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn_and_split "aarch64_cm<optab>di"
  [(set (match_operand:DI 0 "register_operand" "=w,w,r")
	(neg:DI
	  (COMPARISONS:DI
	    (match_operand:DI 1 "register_operand" "w,w,r")
	    (match_operand:DI 2 "aarch64_simd_reg_or_zero" "w,ZDz,r")
	  )))]
  "TARGET_SIMD"
  "@
  cm<n_optab>\t%d0, %d<cmp_1>, %d<cmp_2>
  cm<optab>\t%d0, %d1, #0
  #"
  "reload_completed
   /* We need to prevent the split from
      happening in the 'w' constraint cases.  */
   && GP_REGNUM_P (REGNO (operands[0]))
   && GP_REGNUM_P (REGNO (operands[1]))"
  [(set (reg:CC CC_REGNUM)
    (compare:CC
      (match_dup 1)
      (match_dup 2)))
  (set (match_dup 0)
    (neg:DI
      (COMPARISONS:DI
	(match_operand 3 "cc_register" "")
	(const_int 0))))]
  {
    enum machine_mode mode = SELECT_CC_MODE (<CMP>, operands[1], operands[2]);
    rtx cc_reg = aarch64_gen_compare_reg (<CMP>, operands[1], operands[2]);
    rtx comparison = gen_rtx_<CMP> (mode, operands[1], operands[2]);
    emit_insn (gen_cstoredi_neg (operands[0], comparison, cc_reg));
    DONE;
  }
  [(set_attr "simd_type" "simd_cmp")
   (set_attr "simd_mode" "DI")]
)

;; cm(hs|hi)

(define_insn "aarch64_cm<optab><mode>"
  [(set (match_operand:<V_cmp_result> 0 "register_operand" "=w")
	(neg:<V_cmp_result>
	  (UCOMPARISONS:<V_cmp_result>
	    (match_operand:VDQ 1 "register_operand" "w")
	    (match_operand:VDQ 2 "register_operand" "w")
	  )))]
  "TARGET_SIMD"
  "cm<n_optab>\t%<v>0<Vmtype>, %<v><cmp_1><Vmtype>, %<v><cmp_2><Vmtype>"
  [(set_attr "simd_type" "simd_cmp")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn_and_split "aarch64_cm<optab>di"
  [(set (match_operand:DI 0 "register_operand" "=w,r")
	(neg:DI
	  (UCOMPARISONS:DI
	    (match_operand:DI 1 "register_operand" "w,r")
	    (match_operand:DI 2 "aarch64_simd_reg_or_zero" "w,r")
	  )))]
  "TARGET_SIMD"
  "@
  cm<n_optab>\t%d0, %d<cmp_1>, %d<cmp_2>
  #"
  "reload_completed
   /* We need to prevent the split from
      happening in the 'w' constraint cases.  */
   && GP_REGNUM_P (REGNO (operands[0]))
   && GP_REGNUM_P (REGNO (operands[1]))"
  [(set (reg:CC CC_REGNUM)
    (compare:CC
      (match_dup 1)
      (match_dup 2)))
  (set (match_dup 0)
    (neg:DI
      (UCOMPARISONS:DI
	(match_operand 3 "cc_register" "")
	(const_int 0))))]
  {
    enum machine_mode mode = SELECT_CC_MODE (<CMP>, operands[1], operands[2]);
    rtx cc_reg = aarch64_gen_compare_reg (<CMP>, operands[1], operands[2]);
    rtx comparison = gen_rtx_<CMP> (mode, operands[1], operands[2]);
    emit_insn (gen_cstoredi_neg (operands[0], comparison, cc_reg));
    DONE;
  }
  [(set_attr "simd_type" "simd_cmp")
   (set_attr "simd_mode" "DI")]
)

;; cmtst

(define_insn "aarch64_cmtst<mode>"
  [(set (match_operand:<V_cmp_result> 0 "register_operand" "=w")
	(neg:<V_cmp_result>
	  (ne:<V_cmp_result>
	    (and:VDQ
	      (match_operand:VDQ 1 "register_operand" "w")
	      (match_operand:VDQ 2 "register_operand" "w"))
	    (vec_duplicate:<V_cmp_result> (const_int 0)))))]
  "TARGET_SIMD"
  "cmtst\t%<v>0<Vmtype>, %<v>1<Vmtype>, %<v>2<Vmtype>"
  [(set_attr "simd_type" "simd_cmp")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn_and_split "aarch64_cmtstdi"
  [(set (match_operand:DI 0 "register_operand" "=w,r")
	(neg:DI
	  (ne:DI
	    (and:DI
	      (match_operand:DI 1 "register_operand" "w,r")
	      (match_operand:DI 2 "register_operand" "w,r"))
	    (const_int 0))))]
  "TARGET_SIMD"
  "@
  cmtst\t%d0, %d1, %d2
  #"
  "reload_completed
   /* We need to prevent the split from
      happening in the 'w' constraint cases.  */
   && GP_REGNUM_P (REGNO (operands[0]))
   && GP_REGNUM_P (REGNO (operands[1]))"
   [(set (reg:CC_NZ CC_REGNUM)
	(compare:CC_NZ
	 (and:DI (match_dup 1)
		  (match_dup 2))
	 (const_int 0)))
  (set (match_dup 0)
    (neg:DI
      (ne:DI
	(match_operand 3 "cc_register" "")
	(const_int 0))))]
  {
    rtx and_tree = gen_rtx_AND (DImode, operands[1], operands[2]);
    enum machine_mode mode = SELECT_CC_MODE (NE, and_tree, const0_rtx);
    rtx cc_reg = aarch64_gen_compare_reg (NE, and_tree, const0_rtx);
    rtx comparison = gen_rtx_NE (mode, and_tree, const0_rtx);
    emit_insn (gen_cstoredi_neg (operands[0], comparison, cc_reg));
    DONE;
  }
  [(set_attr "simd_type" "simd_cmp")
   (set_attr "simd_mode" "DI")]
)

;; fcm(eq|ge|gt|le|lt)

(define_insn "aarch64_cm<optab><mode>"
  [(set (match_operand:<V_cmp_result> 0 "register_operand" "=w,w")
	(neg:<V_cmp_result>
	  (COMPARISONS:<V_cmp_result>
	    (match_operand:VALLF 1 "register_operand" "w,w")
	    (match_operand:VALLF 2 "aarch64_simd_reg_or_zero" "w,YDz")
	  )))]
  "TARGET_SIMD"
  "@
  fcm<n_optab>\t%<v>0<Vmtype>, %<v><cmp_1><Vmtype>, %<v><cmp_2><Vmtype>
  fcm<optab>\t%<v>0<Vmtype>, %<v>1<Vmtype>, 0"
  [(set_attr "simd_type" "simd_fcmp")
   (set_attr "simd_mode" "<MODE>")]
)

;; addp

(define_insn "aarch64_addp<mode>"
  [(set (match_operand:VD_BHSI 0 "register_operand" "=w")
        (unspec:VD_BHSI
          [(match_operand:VD_BHSI 1 "register_operand" "w")
	   (match_operand:VD_BHSI 2 "register_operand" "w")]
          UNSPEC_ADDP))]
  "TARGET_SIMD"
  "addp\t%<v>0<Vmtype>, %<v>1<Vmtype>, %<v>2<Vmtype>"
  [(set_attr "simd_type" "simd_add")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_addpdi"
  [(set (match_operand:DI 0 "register_operand" "=w")
        (unspec:DI
          [(match_operand:V2DI 1 "register_operand" "w")]
          UNSPEC_ADDP))]
  "TARGET_SIMD"
  "addp\t%d0, %1.2d"
  [(set_attr "simd_type" "simd_add")
   (set_attr "simd_mode" "DI")]
)

;; v(max|min)

(define_expand "aarch64_<maxmin><mode>"
 [(set (match_operand:VDQ_BHSI 0 "register_operand" "=w")
       (MAXMIN:VDQ_BHSI (match_operand:VDQ_BHSI 1 "register_operand" "w")
			(match_operand:VDQ_BHSI 2 "register_operand" "w")))]
 "TARGET_SIMD"
{
  emit_insn (gen_<maxmin><mode>3 (operands[0], operands[1], operands[2]));
  DONE;
})


(define_insn "aarch64_<fmaxmin><mode>"
  [(set (match_operand:VDQF 0 "register_operand" "=w")
        (unspec:VDQF [(match_operand:VDQF 1 "register_operand" "w")
		      (match_operand:VDQF 2 "register_operand" "w")]
		      FMAXMIN))]
  "TARGET_SIMD"
  "<fmaxmin>\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_fminmax")
   (set_attr "simd_mode" "<MODE>")]
)

;; sqrt

(define_insn "sqrt<mode>2"
  [(set (match_operand:VDQF 0 "register_operand" "=w")
        (sqrt:VDQF (match_operand:VDQF 1 "register_operand" "w")))]
  "TARGET_SIMD"
  "fsqrt\\t%0.<Vtype>, %1.<Vtype>"
  [(set_attr "simd_type" "simd_fsqrt")
   (set_attr "simd_mode" "<MODE>")]
)

(define_expand "aarch64_sqrt<mode>"
  [(match_operand:VDQF 0 "register_operand" "=w")
   (match_operand:VDQF 1 "register_operand" "w")]
  "TARGET_SIMD"
{
  emit_insn (gen_sqrt<mode>2 (operands[0], operands[1]));
  DONE;
})


;; Patterns for vector struct loads and stores.

(define_insn "vec_load_lanesoi<mode>"
  [(set (match_operand:OI 0 "register_operand" "=w")
	(unspec:OI [(match_operand:OI 1 "aarch64_simd_struct_operand" "Utv")
		    (unspec:VQ [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
		   UNSPEC_LD2))]
  "TARGET_SIMD"
  "ld2\\t{%S0.<Vtype> - %T0.<Vtype>}, %1"
  [(set_attr "simd_type" "simd_load2")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "vec_store_lanesoi<mode>"
  [(set (match_operand:OI 0 "aarch64_simd_struct_operand" "=Utv")
	(unspec:OI [(match_operand:OI 1 "register_operand" "w")
                    (unspec:VQ [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
                   UNSPEC_ST2))]
  "TARGET_SIMD"
  "st2\\t{%S1.<Vtype> - %T1.<Vtype>}, %0"
  [(set_attr "simd_type" "simd_store2")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "vec_load_lanesci<mode>"
  [(set (match_operand:CI 0 "register_operand" "=w")
	(unspec:CI [(match_operand:CI 1 "aarch64_simd_struct_operand" "Utv")
		    (unspec:VQ [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
		   UNSPEC_LD3))]
  "TARGET_SIMD"
  "ld3\\t{%S0.<Vtype> - %U0.<Vtype>}, %1"
  [(set_attr "simd_type" "simd_load3")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "vec_store_lanesci<mode>"
  [(set (match_operand:CI 0 "aarch64_simd_struct_operand" "=Utv")
	(unspec:CI [(match_operand:CI 1 "register_operand" "w")
                    (unspec:VQ [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
                   UNSPEC_ST3))]
  "TARGET_SIMD"
  "st3\\t{%S1.<Vtype> - %U1.<Vtype>}, %0"
  [(set_attr "simd_type" "simd_store3")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "vec_load_lanesxi<mode>"
  [(set (match_operand:XI 0 "register_operand" "=w")
	(unspec:XI [(match_operand:XI 1 "aarch64_simd_struct_operand" "Utv")
		    (unspec:VQ [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
		   UNSPEC_LD4))]
  "TARGET_SIMD"
  "ld4\\t{%S0.<Vtype> - %V0.<Vtype>}, %1"
  [(set_attr "simd_type" "simd_load4")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "vec_store_lanesxi<mode>"
  [(set (match_operand:XI 0 "aarch64_simd_struct_operand" "=Utv")
	(unspec:XI [(match_operand:XI 1 "register_operand" "w")
                    (unspec:VQ [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
                   UNSPEC_ST4))]
  "TARGET_SIMD"
  "st4\\t{%S1.<Vtype> - %V1.<Vtype>}, %0"
  [(set_attr "simd_type" "simd_store4")
   (set_attr "simd_mode" "<MODE>")])

;; Reload patterns for AdvSIMD register list operands.

(define_expand "mov<mode>"
  [(set (match_operand:VSTRUCT 0 "aarch64_simd_nonimmediate_operand" "")
	(match_operand:VSTRUCT 1 "aarch64_simd_general_operand" ""))]
  "TARGET_SIMD"
{
  if (can_create_pseudo_p ())
    {
      if (GET_CODE (operands[0]) != REG)
	operands[1] = force_reg (<MODE>mode, operands[1]);
    }
})

(define_insn "*aarch64_mov<mode>"
  [(set (match_operand:VSTRUCT 0 "aarch64_simd_nonimmediate_operand" "=w,Utv,w")
	(match_operand:VSTRUCT 1 "aarch64_simd_general_operand"	" w,w,Utv"))]
  "TARGET_SIMD
   && (register_operand (operands[0], <MODE>mode)
       || register_operand (operands[1], <MODE>mode))"

{
  switch (which_alternative)
    {
    case 0: return "#";
    case 1: return "st1\\t{%S1.16b - %<Vendreg>1.16b}, %0";
    case 2: return "ld1\\t{%S0.16b - %<Vendreg>0.16b}, %1";
    default: gcc_unreachable ();
    }
}
  [(set_attr "simd_type" "simd_move,simd_store<nregs>,simd_load<nregs>")
   (set (attr "length") (symbol_ref "aarch64_simd_attr_length_move (insn)"))
   (set_attr "simd_mode" "<MODE>")])

(define_split
  [(set (match_operand:OI 0 "register_operand" "")
	(match_operand:OI 1 "register_operand" ""))]
  "TARGET_SIMD && reload_completed"
  [(set (match_dup 0) (match_dup 1))
   (set (match_dup 2) (match_dup 3))]
{
  int rdest = REGNO (operands[0]);
  int rsrc = REGNO (operands[1]);
  rtx dest[2], src[2];

  dest[0] = gen_rtx_REG (TFmode, rdest);
  src[0] = gen_rtx_REG (TFmode, rsrc);
  dest[1] = gen_rtx_REG (TFmode, rdest + 1);
  src[1] = gen_rtx_REG (TFmode, rsrc + 1);

  aarch64_simd_disambiguate_copy (operands, dest, src, 2);
})

(define_split
  [(set (match_operand:CI 0 "register_operand" "")
	(match_operand:CI 1 "register_operand" ""))]
  "TARGET_SIMD && reload_completed"
  [(set (match_dup 0) (match_dup 1))
   (set (match_dup 2) (match_dup 3))
   (set (match_dup 4) (match_dup 5))]
{
  int rdest = REGNO (operands[0]);
  int rsrc = REGNO (operands[1]);
  rtx dest[3], src[3];

  dest[0] = gen_rtx_REG (TFmode, rdest);
  src[0] = gen_rtx_REG (TFmode, rsrc);
  dest[1] = gen_rtx_REG (TFmode, rdest + 1);
  src[1] = gen_rtx_REG (TFmode, rsrc + 1);
  dest[2] = gen_rtx_REG (TFmode, rdest + 2);
  src[2] = gen_rtx_REG (TFmode, rsrc + 2);

  aarch64_simd_disambiguate_copy (operands, dest, src, 3);
})

(define_split
  [(set (match_operand:XI 0 "register_operand" "")
	(match_operand:XI 1 "register_operand" ""))]
  "TARGET_SIMD && reload_completed"
  [(set (match_dup 0) (match_dup 1))
   (set (match_dup 2) (match_dup 3))
   (set (match_dup 4) (match_dup 5))
   (set (match_dup 6) (match_dup 7))]
{
  int rdest = REGNO (operands[0]);
  int rsrc = REGNO (operands[1]);
  rtx dest[4], src[4];

  dest[0] = gen_rtx_REG (TFmode, rdest);
  src[0] = gen_rtx_REG (TFmode, rsrc);
  dest[1] = gen_rtx_REG (TFmode, rdest + 1);
  src[1] = gen_rtx_REG (TFmode, rsrc + 1);
  dest[2] = gen_rtx_REG (TFmode, rdest + 2);
  src[2] = gen_rtx_REG (TFmode, rsrc + 2);
  dest[3] = gen_rtx_REG (TFmode, rdest + 3);
  src[3] = gen_rtx_REG (TFmode, rsrc + 3);

  aarch64_simd_disambiguate_copy (operands, dest, src, 4);
})

(define_insn "aarch64_ld2<mode>_dreg"
  [(set (match_operand:OI 0 "register_operand" "=w")
	(subreg:OI
	  (vec_concat:<VRL2>
	    (vec_concat:<VDBL>
	     (unspec:VD [(match_operand:TI 1 "aarch64_simd_struct_operand" "Utv")]
			UNSPEC_LD2)
	     (vec_duplicate:VD (const_int 0)))
	    (vec_concat:<VDBL>
	     (unspec:VD [(match_dup 1)]
			UNSPEC_LD2)
	     (vec_duplicate:VD (const_int 0)))) 0))]
  "TARGET_SIMD"
  "ld2\\t{%S0.<Vtype> - %T0.<Vtype>}, %1"
  [(set_attr "simd_type" "simd_load2")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_ld2<mode>_dreg"
  [(set (match_operand:OI 0 "register_operand" "=w")
	(subreg:OI
	  (vec_concat:<VRL2>
	    (vec_concat:<VDBL>
	     (unspec:DX [(match_operand:TI 1 "aarch64_simd_struct_operand" "Utv")]
			UNSPEC_LD2)
	     (const_int 0))
	    (vec_concat:<VDBL>
	     (unspec:DX [(match_dup 1)]
			UNSPEC_LD2)
	     (const_int 0))) 0))]
  "TARGET_SIMD"
  "ld1\\t{%S0.1d - %T0.1d}, %1"
  [(set_attr "simd_type" "simd_load2")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_ld3<mode>_dreg"
  [(set (match_operand:CI 0 "register_operand" "=w")
	(subreg:CI
	 (vec_concat:<VRL3>
	  (vec_concat:<VRL2>
	    (vec_concat:<VDBL>
	     (unspec:VD [(match_operand:EI 1 "aarch64_simd_struct_operand" "Utv")]
			UNSPEC_LD3)
	     (vec_duplicate:VD (const_int 0)))
	    (vec_concat:<VDBL>
	     (unspec:VD [(match_dup 1)]
			UNSPEC_LD3)
	     (vec_duplicate:VD (const_int 0))))
	  (vec_concat:<VDBL>
	     (unspec:VD [(match_dup 1)]
			UNSPEC_LD3)
	     (vec_duplicate:VD (const_int 0)))) 0))]
  "TARGET_SIMD"
  "ld3\\t{%S0.<Vtype> - %U0.<Vtype>}, %1"
  [(set_attr "simd_type" "simd_load3")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_ld3<mode>_dreg"
  [(set (match_operand:CI 0 "register_operand" "=w")
	(subreg:CI
	 (vec_concat:<VRL3>
	  (vec_concat:<VRL2>
	    (vec_concat:<VDBL>
	     (unspec:DX [(match_operand:EI 1 "aarch64_simd_struct_operand" "Utv")]
			UNSPEC_LD3)
	     (const_int 0))
	    (vec_concat:<VDBL>
	     (unspec:DX [(match_dup 1)]
			UNSPEC_LD3)
	     (const_int 0)))
	  (vec_concat:<VDBL>
	     (unspec:DX [(match_dup 1)]
			UNSPEC_LD3)
	     (const_int 0))) 0))]
  "TARGET_SIMD"
  "ld1\\t{%S0.1d - %U0.1d}, %1"
  [(set_attr "simd_type" "simd_load3")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_ld4<mode>_dreg"
  [(set (match_operand:XI 0 "register_operand" "=w")
	(subreg:XI
	 (vec_concat:<VRL4>
	   (vec_concat:<VRL2>
	     (vec_concat:<VDBL>
	       (unspec:VD [(match_operand:OI 1 "aarch64_simd_struct_operand" "Utv")]
			  UNSPEC_LD4)
	       (vec_duplicate:VD (const_int 0)))
	      (vec_concat:<VDBL>
	        (unspec:VD [(match_dup 1)]
			UNSPEC_LD4)
	        (vec_duplicate:VD (const_int 0))))
	   (vec_concat:<VRL2>
	     (vec_concat:<VDBL>
	       (unspec:VD [(match_dup 1)]
			UNSPEC_LD4)
	       (vec_duplicate:VD (const_int 0)))
	     (vec_concat:<VDBL>
	       (unspec:VD [(match_dup 1)]
			UNSPEC_LD4)
	       (vec_duplicate:VD (const_int 0))))) 0))]
  "TARGET_SIMD"
  "ld4\\t{%S0.<Vtype> - %V0.<Vtype>}, %1"
  [(set_attr "simd_type" "simd_load4")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_ld4<mode>_dreg"
  [(set (match_operand:XI 0 "register_operand" "=w")
	(subreg:XI
	 (vec_concat:<VRL4>
	   (vec_concat:<VRL2>
	     (vec_concat:<VDBL>
	       (unspec:DX [(match_operand:OI 1 "aarch64_simd_struct_operand" "Utv")]
			  UNSPEC_LD4)
	       (const_int 0))
	      (vec_concat:<VDBL>
	        (unspec:DX [(match_dup 1)]
			UNSPEC_LD4)
	        (const_int 0)))
	   (vec_concat:<VRL2>
	     (vec_concat:<VDBL>
	       (unspec:DX [(match_dup 1)]
			UNSPEC_LD4)
	       (const_int 0))
	     (vec_concat:<VDBL>
	       (unspec:DX [(match_dup 1)]
			UNSPEC_LD4)
	       (const_int 0)))) 0))]
  "TARGET_SIMD"
  "ld1\\t{%S0.1d - %V0.1d}, %1"
  [(set_attr "simd_type" "simd_load4")
   (set_attr "simd_mode" "<MODE>")])

(define_expand "aarch64_ld<VSTRUCT:nregs><VDC:mode>"
 [(match_operand:VSTRUCT 0 "register_operand" "=w")
  (match_operand:DI 1 "register_operand" "r")
  (unspec:VDC [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
  "TARGET_SIMD"
{
  enum machine_mode mode = <VSTRUCT:VSTRUCT_DREG>mode;
  rtx mem = gen_rtx_MEM (mode, operands[1]);

  emit_insn (gen_aarch64_ld<VSTRUCT:nregs><VDC:mode>_dreg (operands[0], mem));
  DONE;
})

(define_expand "aarch64_ld1<VALL:mode>"
 [(match_operand:VALL 0 "register_operand")
  (match_operand:DI 1 "register_operand")]
  "TARGET_SIMD"
{
  enum machine_mode mode = <VALL:MODE>mode;
  rtx mem = gen_rtx_MEM (mode, operands[1]);
  emit_move_insn (operands[0], mem);
  DONE;
})

(define_expand "aarch64_ld<VSTRUCT:nregs><VQ:mode>"
 [(match_operand:VSTRUCT 0 "register_operand" "=w")
  (match_operand:DI 1 "register_operand" "r")
  (unspec:VQ [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
  "TARGET_SIMD"
{
  enum machine_mode mode = <VSTRUCT:MODE>mode;
  rtx mem = gen_rtx_MEM (mode, operands[1]);

  emit_insn (gen_vec_load_lanes<VSTRUCT:mode><VQ:mode> (operands[0], mem));
  DONE;
})

;; Expanders for builtins to extract vector registers from large
;; opaque integer modes.

;; D-register list.

(define_expand "aarch64_get_dreg<VSTRUCT:mode><VDC:mode>"
 [(match_operand:VDC 0 "register_operand" "=w")
  (match_operand:VSTRUCT 1 "register_operand" "w")
  (match_operand:SI 2 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  int part = INTVAL (operands[2]);
  rtx temp = gen_reg_rtx (<VDC:VDBL>mode);
  int offset = part * 16;

  emit_move_insn (temp, gen_rtx_SUBREG (<VDC:VDBL>mode, operands[1], offset));
  emit_move_insn (operands[0], gen_lowpart (<VDC:MODE>mode, temp));
  DONE;
})

;; Q-register list.

(define_expand "aarch64_get_qreg<VSTRUCT:mode><VQ:mode>"
 [(match_operand:VQ 0 "register_operand" "=w")
  (match_operand:VSTRUCT 1 "register_operand" "w")
  (match_operand:SI 2 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  int part = INTVAL (operands[2]);
  int offset = part * 16;

  emit_move_insn (operands[0],
		  gen_rtx_SUBREG (<VQ:MODE>mode, operands[1], offset));
  DONE;
})

;; Permuted-store expanders for neon intrinsics.

;; Permute instructions

;; vec_perm support

(define_expand "vec_perm_const<mode>"
  [(match_operand:VALL 0 "register_operand")
   (match_operand:VALL 1 "register_operand")
   (match_operand:VALL 2 "register_operand")
   (match_operand:<V_cmp_result> 3)]
  "TARGET_SIMD"
{
  if (aarch64_expand_vec_perm_const (operands[0], operands[1],
				     operands[2], operands[3]))
    DONE;
  else
    FAIL;
})

(define_expand "vec_perm<mode>"
  [(match_operand:VB 0 "register_operand")
   (match_operand:VB 1 "register_operand")
   (match_operand:VB 2 "register_operand")
   (match_operand:VB 3 "register_operand")]
  "TARGET_SIMD"
{
  aarch64_expand_vec_perm (operands[0], operands[1],
			   operands[2], operands[3]);
  DONE;
})

(define_insn "aarch64_tbl1<mode>"
  [(set (match_operand:VB 0 "register_operand" "=w")
	(unspec:VB [(match_operand:V16QI 1 "register_operand" "w")
		    (match_operand:VB 2 "register_operand" "w")]
		   UNSPEC_TBL))]
  "TARGET_SIMD"
  "tbl\\t%0.<Vtype>, {%1.16b}, %2.<Vtype>"
  [(set_attr "simd_type" "simd_tbl")
   (set_attr "simd_mode" "<MODE>")]
)

;; Two source registers.

(define_insn "aarch64_tbl2v16qi"
  [(set (match_operand:V16QI 0 "register_operand" "=w")
	(unspec:V16QI [(match_operand:OI 1 "register_operand" "w")
		       (match_operand:V16QI 2 "register_operand" "w")]
		      UNSPEC_TBL))]
  "TARGET_SIMD"
  "tbl\\t%0.16b, {%S1.16b - %T1.16b}, %2.16b"
  [(set_attr "simd_type" "simd_tbl")
   (set_attr "simd_mode" "V16QI")]
)

(define_insn_and_split "aarch64_combinev16qi"
  [(set (match_operand:OI 0 "register_operand" "=w")
	(unspec:OI [(match_operand:V16QI 1 "register_operand" "w")
		    (match_operand:V16QI 2 "register_operand" "w")]
		   UNSPEC_CONCAT))]
  "TARGET_SIMD"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
  aarch64_split_combinev16qi (operands);
  DONE;
})

(define_insn "aarch64_<PERMUTE:perm_insn><PERMUTE:perm_hilo><mode>"
  [(set (match_operand:VALL 0 "register_operand" "=w")
	(unspec:VALL [(match_operand:VALL 1 "register_operand" "w")
		      (match_operand:VALL 2 "register_operand" "w")]
		       PERMUTE))]
  "TARGET_SIMD"
  "<PERMUTE:perm_insn><PERMUTE:perm_hilo>\\t%0.<Vtype>, %1.<Vtype>, %2.<Vtype>"
  [(set_attr "simd_type" "simd_<PERMUTE:perm_insn>")
   (set_attr "simd_mode" "<MODE>")]
)

(define_insn "aarch64_st2<mode>_dreg"
  [(set (match_operand:TI 0 "aarch64_simd_struct_operand" "=Utv")
	(unspec:TI [(match_operand:OI 1 "register_operand" "w")
                    (unspec:VD [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
                   UNSPEC_ST2))]
  "TARGET_SIMD"
  "st2\\t{%S1.<Vtype> - %T1.<Vtype>}, %0"
  [(set_attr "simd_type" "simd_store2")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_st2<mode>_dreg"
  [(set (match_operand:TI 0 "aarch64_simd_struct_operand" "=Utv")
	(unspec:TI [(match_operand:OI 1 "register_operand" "w")
                    (unspec:DX [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
                   UNSPEC_ST2))]
  "TARGET_SIMD"
  "st1\\t{%S1.1d - %T1.1d}, %0"
  [(set_attr "simd_type" "simd_store2")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_st3<mode>_dreg"
  [(set (match_operand:EI 0 "aarch64_simd_struct_operand" "=Utv")
	(unspec:EI [(match_operand:CI 1 "register_operand" "w")
                    (unspec:VD [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
                   UNSPEC_ST3))]
  "TARGET_SIMD"
  "st3\\t{%S1.<Vtype> - %U1.<Vtype>}, %0"
  [(set_attr "simd_type" "simd_store3")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_st3<mode>_dreg"
  [(set (match_operand:EI 0 "aarch64_simd_struct_operand" "=Utv")
	(unspec:EI [(match_operand:CI 1 "register_operand" "w")
                    (unspec:DX [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
                   UNSPEC_ST3))]
  "TARGET_SIMD"
  "st1\\t{%S1.1d - %U1.1d}, %0"
  [(set_attr "simd_type" "simd_store3")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_st4<mode>_dreg"
  [(set (match_operand:OI 0 "aarch64_simd_struct_operand" "=Utv")
	(unspec:OI [(match_operand:XI 1 "register_operand" "w")
                    (unspec:VD [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
                   UNSPEC_ST4))]
  "TARGET_SIMD"
  "st4\\t{%S1.<Vtype> - %V1.<Vtype>}, %0"
  [(set_attr "simd_type" "simd_store4")
   (set_attr "simd_mode" "<MODE>")])

(define_insn "aarch64_st4<mode>_dreg"
  [(set (match_operand:OI 0 "aarch64_simd_struct_operand" "=Utv")
	(unspec:OI [(match_operand:XI 1 "register_operand" "w")
                    (unspec:DX [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
                   UNSPEC_ST4))]
  "TARGET_SIMD"
  "st1\\t{%S1.1d - %V1.1d}, %0"
  [(set_attr "simd_type" "simd_store4")
   (set_attr "simd_mode" "<MODE>")])

(define_expand "aarch64_st<VSTRUCT:nregs><VDC:mode>"
 [(match_operand:DI 0 "register_operand" "r")
  (match_operand:VSTRUCT 1 "register_operand" "w")
  (unspec:VDC [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
  "TARGET_SIMD"
{
  enum machine_mode mode = <VSTRUCT:VSTRUCT_DREG>mode;
  rtx mem = gen_rtx_MEM (mode, operands[0]);

  emit_insn (gen_aarch64_st<VSTRUCT:nregs><VDC:mode>_dreg (mem, operands[1]));
  DONE;
})

(define_expand "aarch64_st<VSTRUCT:nregs><VQ:mode>"
 [(match_operand:DI 0 "register_operand" "r")
  (match_operand:VSTRUCT 1 "register_operand" "w")
  (unspec:VQ [(const_int 0)] UNSPEC_VSTRUCTDUMMY)]
  "TARGET_SIMD"
{
  enum machine_mode mode = <VSTRUCT:MODE>mode;
  rtx mem = gen_rtx_MEM (mode, operands[0]);

  emit_insn (gen_vec_store_lanes<VSTRUCT:mode><VQ:mode> (mem, operands[1]));
  DONE;
})

(define_expand "aarch64_st1<VALL:mode>"
 [(match_operand:DI 0 "register_operand")
  (match_operand:VALL 1 "register_operand")]
  "TARGET_SIMD"
{
  enum machine_mode mode = <VALL:MODE>mode;
  rtx mem = gen_rtx_MEM (mode, operands[0]);
  emit_move_insn (mem, operands[1]);
  DONE;
})

;; Expander for builtins to insert vector registers into large
;; opaque integer modes.

;; Q-register list.  We don't need a D-reg inserter as we zero
;; extend them in arm_neon.h and insert the resulting Q-regs.

(define_expand "aarch64_set_qreg<VSTRUCT:mode><VQ:mode>"
 [(match_operand:VSTRUCT 0 "register_operand" "+w")
  (match_operand:VSTRUCT 1 "register_operand" "0")
  (match_operand:VQ 2 "register_operand" "w")
  (match_operand:SI 3 "immediate_operand" "i")]
  "TARGET_SIMD"
{
  int part = INTVAL (operands[3]);
  int offset = part * 16;

  emit_move_insn (operands[0], operands[1]);
  emit_move_insn (gen_rtx_SUBREG (<VQ:MODE>mode, operands[0], offset),
		  operands[2]);
  DONE;
})

;; Standard pattern name vec_init<mode>.

(define_expand "vec_init<mode>"
  [(match_operand:VALL 0 "register_operand" "")
   (match_operand 1 "" "")]
  "TARGET_SIMD"
{
  aarch64_expand_vector_init (operands[0], operands[1]);
  DONE;
})

(define_insn "*aarch64_simd_ld1r<mode>"
  [(set (match_operand:VALLDI 0 "register_operand" "=w")
	(vec_duplicate:VALLDI
	  (match_operand:<VEL> 1 "aarch64_simd_struct_operand" "Utv")))]
  "TARGET_SIMD"
  "ld1r\\t{%0.<Vtype>}, %1"
  [(set_attr "simd_type" "simd_load1r")
   (set_attr "simd_mode" "<MODE>")])
