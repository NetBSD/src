;; GCC machine description for SSE instructions
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

(define_c_enum "unspec" [
  ;; SSE
  UNSPEC_MOVNT
  UNSPEC_LOADU
  UNSPEC_STOREU

  ;; SSE3
  UNSPEC_LDDQU

  ;; SSSE3
  UNSPEC_PSHUFB
  UNSPEC_PSIGN
  UNSPEC_PALIGNR

  ;; For SSE4A support
  UNSPEC_EXTRQI
  UNSPEC_EXTRQ
  UNSPEC_INSERTQI
  UNSPEC_INSERTQ

  ;; For SSE4.1 support
  UNSPEC_BLENDV
  UNSPEC_INSERTPS
  UNSPEC_DP
  UNSPEC_MOVNTDQA
  UNSPEC_MPSADBW
  UNSPEC_PHMINPOSUW
  UNSPEC_PTEST

  ;; For SSE4.2 support
  UNSPEC_PCMPESTR
  UNSPEC_PCMPISTR

  ;; For FMA4 support
  UNSPEC_FMADDSUB
  UNSPEC_XOP_UNSIGNED_CMP
  UNSPEC_XOP_TRUEFALSE
  UNSPEC_XOP_PERMUTE
  UNSPEC_FRCZ

  ;; For AES support
  UNSPEC_AESENC
  UNSPEC_AESENCLAST
  UNSPEC_AESDEC
  UNSPEC_AESDECLAST
  UNSPEC_AESIMC
  UNSPEC_AESKEYGENASSIST

  ;; For PCLMUL support
  UNSPEC_PCLMUL

  ;; For AVX support
  UNSPEC_PCMP
  UNSPEC_VPERMIL
  UNSPEC_VPERMIL2
  UNSPEC_VPERMIL2F128
  UNSPEC_CAST
  UNSPEC_VTESTP
  UNSPEC_VCVTPH2PS
  UNSPEC_VCVTPS2PH

  ;; For AVX2 support
  UNSPEC_VPERMVAR
  UNSPEC_VPERMTI
  UNSPEC_GATHER
  UNSPEC_VSIBADDR
])

(define_c_enum "unspecv" [
  UNSPECV_LDMXCSR
  UNSPECV_STMXCSR
  UNSPECV_CLFLUSH
  UNSPECV_MONITOR
  UNSPECV_MWAIT
  UNSPECV_VZEROALL
  UNSPECV_VZEROUPPER
])

;; All vector modes including V?TImode, used in move patterns.
(define_mode_iterator V16
  [(V32QI "TARGET_AVX") V16QI
   (V16HI "TARGET_AVX") V8HI
   (V8SI "TARGET_AVX") V4SI
   (V4DI "TARGET_AVX") V2DI
   (V2TI "TARGET_AVX") V1TI
   (V8SF "TARGET_AVX") V4SF
   (V4DF "TARGET_AVX") V2DF])

;; All vector modes
(define_mode_iterator V
  [(V32QI "TARGET_AVX") V16QI
   (V16HI "TARGET_AVX") V8HI
   (V8SI "TARGET_AVX") V4SI
   (V4DI "TARGET_AVX") V2DI
   (V8SF "TARGET_AVX") V4SF
   (V4DF "TARGET_AVX") (V2DF "TARGET_SSE2")])

;; All 128bit vector modes
(define_mode_iterator V_128
  [V16QI V8HI V4SI V2DI V4SF (V2DF "TARGET_SSE2")])

;; All 256bit vector modes
(define_mode_iterator V_256
  [V32QI V16HI V8SI V4DI V8SF V4DF])

;; All vector float modes
(define_mode_iterator VF
  [(V8SF "TARGET_AVX") V4SF
   (V4DF "TARGET_AVX") (V2DF "TARGET_SSE2")])

;; All SFmode vector float modes
(define_mode_iterator VF1
  [(V8SF "TARGET_AVX") V4SF])

;; All DFmode vector float modes
(define_mode_iterator VF2
  [(V4DF "TARGET_AVX") V2DF])

;; All 128bit vector float modes
(define_mode_iterator VF_128
  [V4SF (V2DF "TARGET_SSE2")])

;; All 256bit vector float modes
(define_mode_iterator VF_256
  [V8SF V4DF])

;; All vector integer modes
(define_mode_iterator VI
  [(V32QI "TARGET_AVX") V16QI
   (V16HI "TARGET_AVX") V8HI
   (V8SI "TARGET_AVX") V4SI
   (V4DI "TARGET_AVX") V2DI])

(define_mode_iterator VI_AVX2
  [(V32QI "TARGET_AVX2") V16QI
   (V16HI "TARGET_AVX2") V8HI
   (V8SI "TARGET_AVX2") V4SI
   (V4DI "TARGET_AVX2") V2DI])

;; All QImode vector integer modes
(define_mode_iterator VI1
  [(V32QI "TARGET_AVX") V16QI])

;; All DImode vector integer modes
(define_mode_iterator VI8
  [(V4DI "TARGET_AVX") V2DI])

(define_mode_iterator VI1_AVX2
  [(V32QI "TARGET_AVX2") V16QI])

(define_mode_iterator VI2_AVX2
  [(V16HI "TARGET_AVX2") V8HI])

(define_mode_iterator VI4_AVX2
  [(V8SI "TARGET_AVX2") V4SI])

(define_mode_iterator VI8_AVX2
  [(V4DI "TARGET_AVX2") V2DI])

;; ??? We should probably use TImode instead.
(define_mode_iterator VIMAX_AVX2
  [(V2TI "TARGET_AVX2") V1TI])

;; ??? This should probably be dropped in favor of VIMAX_AVX2.
(define_mode_iterator SSESCALARMODE
  [(V2TI "TARGET_AVX2") TI])

(define_mode_iterator VI12_AVX2
  [(V32QI "TARGET_AVX2") V16QI
   (V16HI "TARGET_AVX2") V8HI])

(define_mode_iterator VI24_AVX2
  [(V16HI "TARGET_AVX2") V8HI
   (V8SI "TARGET_AVX2") V4SI])

(define_mode_iterator VI124_AVX2
  [(V32QI "TARGET_AVX2") V16QI
   (V16HI "TARGET_AVX2") V8HI
   (V8SI "TARGET_AVX2") V4SI])

(define_mode_iterator VI248_AVX2
  [(V16HI "TARGET_AVX2") V8HI
   (V8SI "TARGET_AVX2") V4SI
   (V4DI "TARGET_AVX2") V2DI])

(define_mode_iterator VI48_AVX2
  [(V8SI "TARGET_AVX2") V4SI
   (V4DI "TARGET_AVX2") V2DI])

(define_mode_iterator V48_AVX2
  [V4SF V2DF
   V8SF V4DF
   (V4SI "TARGET_AVX2") (V2DI "TARGET_AVX2")
   (V8SI "TARGET_AVX2") (V4DI "TARGET_AVX2")])

(define_mode_attr sse2_avx2
  [(V16QI "sse2") (V32QI "avx2")
   (V8HI "sse2") (V16HI "avx2")
   (V4SI "sse2") (V8SI "avx2")
   (V2DI "sse2") (V4DI "avx2")
   (V1TI "sse2") (V2TI "avx2")])

(define_mode_attr ssse3_avx2
   [(V16QI "ssse3") (V32QI "avx2")
    (V4HI "ssse3") (V8HI "ssse3") (V16HI "avx2")
    (V4SI "ssse3") (V8SI "avx2")
    (V2DI "ssse3") (V4DI "avx2")
    (TI "ssse3") (V2TI "avx2")])

(define_mode_attr sse4_1_avx2
   [(V16QI "sse4_1") (V32QI "avx2")
    (V8HI "sse4_1") (V16HI "avx2")
    (V4SI "sse4_1") (V8SI "avx2")
    (V2DI "sse4_1") (V4DI "avx2")])

(define_mode_attr avx_avx2
  [(V4SF "avx") (V2DF "avx")
   (V8SF "avx") (V4DF "avx")
   (V4SI "avx2") (V2DI "avx2")
   (V8SI "avx2") (V4DI "avx2")])

(define_mode_attr vec_avx2
  [(V16QI "vec") (V32QI "avx2")
   (V8HI "vec") (V16HI "avx2")
   (V4SI "vec") (V8SI "avx2")
   (V2DI "vec") (V4DI "avx2")])

(define_mode_attr ssedoublemode
  [(V16HI "V16SI") (V8HI "V8SI") (V4HI "V4SI")
   (V32QI "V32HI") (V16QI "V16HI")])

(define_mode_attr ssebytemode
  [(V4DI "V32QI") (V2DI "V16QI")])

;; All 128bit vector integer modes
(define_mode_iterator VI_128 [V16QI V8HI V4SI V2DI])

;; All 256bit vector integer modes
(define_mode_iterator VI_256 [V32QI V16HI V8SI V4DI])

;; Random 128bit vector integer mode combinations
(define_mode_iterator VI12_128 [V16QI V8HI])
(define_mode_iterator VI14_128 [V16QI V4SI])
(define_mode_iterator VI124_128 [V16QI V8HI V4SI])
(define_mode_iterator VI128_128 [V16QI V8HI V2DI])
(define_mode_iterator VI24_128 [V8HI V4SI])
(define_mode_iterator VI248_128 [V8HI V4SI V2DI])
(define_mode_iterator VI48_128 [V4SI V2DI])

;; Random 256bit vector integer mode combinations
(define_mode_iterator VI124_256 [V32QI V16HI V8SI])
(define_mode_iterator VI48_256 [V8SI V4DI])

;; Int-float size matches
(define_mode_iterator VI4F_128 [V4SI V4SF])
(define_mode_iterator VI8F_128 [V2DI V2DF])
(define_mode_iterator VI4F_256 [V8SI V8SF])
(define_mode_iterator VI8F_256 [V4DI V4DF])

;; Mapping from float mode to required SSE level
(define_mode_attr sse
  [(SF "sse") (DF "sse2")
   (V4SF "sse") (V2DF "sse2")
   (V8SF "avx") (V4DF "avx")])

(define_mode_attr sse2
  [(V16QI "sse2") (V32QI "avx")
   (V2DI "sse2") (V4DI "avx")])

(define_mode_attr sse3
  [(V16QI "sse3") (V32QI "avx")])

(define_mode_attr sse4_1
  [(V4SF "sse4_1") (V2DF "sse4_1")
   (V8SF "avx") (V4DF "avx")])

(define_mode_attr avxsizesuffix
  [(V32QI "256") (V16HI "256") (V8SI "256") (V4DI "256")
   (V16QI "") (V8HI "") (V4SI "") (V2DI "")
   (V8SF "256") (V4DF "256")
   (V4SF "") (V2DF "")])

;; SSE instruction mode
(define_mode_attr sseinsnmode
  [(V32QI "OI") (V16HI "OI") (V8SI "OI") (V4DI "OI") (V2TI "OI")
   (V16QI "TI") (V8HI "TI") (V4SI "TI") (V2DI "TI") (V1TI "TI")
   (V8SF "V8SF") (V4DF "V4DF")
   (V4SF "V4SF") (V2DF "V2DF")
   (TI "TI")])

;; Mapping of vector float modes to an integer mode of the same size
(define_mode_attr sseintvecmode
  [(V8SF "V8SI") (V4DF "V4DI")
   (V4SF "V4SI") (V2DF "V2DI")
   (V8SI "V8SI") (V4DI "V4DI")
   (V4SI "V4SI") (V2DI "V2DI")
   (V16HI "V16HI") (V8HI "V8HI")
   (V32QI "V32QI") (V16QI "V16QI")])

(define_mode_attr sseintvecmodelower
  [(V8SF "v8si") (V4DF "v4di")
   (V4SF "v4si") (V2DF "v2di")
   (V8SI "v8si") (V4DI "v4di")
   (V4SI "v4si") (V2DI "v2di")
   (V16HI "v16hi") (V8HI "v8hi")
   (V32QI "v32qi") (V16QI "v16qi")])

;; Mapping of vector modes to a vector mode of double size
(define_mode_attr ssedoublevecmode
  [(V32QI "V64QI") (V16HI "V32HI") (V8SI "V16SI") (V4DI "V8DI")
   (V16QI "V32QI") (V8HI "V16HI") (V4SI "V8SI") (V2DI "V4DI")
   (V8SF "V16SF") (V4DF "V8DF")
   (V4SF "V8SF") (V2DF "V4DF")])

;; Mapping of vector modes to a vector mode of half size
(define_mode_attr ssehalfvecmode
  [(V32QI "V16QI") (V16HI "V8HI") (V8SI "V4SI") (V4DI "V2DI")
   (V16QI  "V8QI") (V8HI  "V4HI") (V4SI "V2SI")
   (V8SF "V4SF") (V4DF "V2DF")
   (V4SF "V2SF")])

;; Mapping of vector modes ti packed single mode of the same size
(define_mode_attr ssePSmode
  [(V32QI "V8SF") (V16QI "V4SF")
   (V16HI "V8SF") (V8HI "V4SF")
   (V8SI "V8SF") (V4SI "V4SF")
   (V4DI "V8SF") (V2DI "V4SF")
   (V2TI "V8SF") (V1TI "V4SF")
   (V8SF "V8SF") (V4SF "V4SF")
   (V4DF "V8SF") (V2DF "V4SF")])

;; Mapping of vector modes back to the scalar modes
(define_mode_attr ssescalarmode
  [(V32QI "QI") (V16HI "HI") (V8SI "SI") (V4DI "DI")
   (V16QI "QI") (V8HI "HI") (V4SI "SI") (V2DI "DI")
   (V8SF "SF") (V4DF "DF")
   (V4SF "SF") (V2DF "DF")])

;; Number of scalar elements in each vector type
(define_mode_attr ssescalarnum
  [(V32QI "32") (V16HI "16") (V8SI "8") (V4DI "4")
   (V16QI "16") (V8HI "8") (V4SI "4") (V2DI "2")
   (V8SF "8") (V4DF "4")
   (V4SF "4") (V2DF "2")])

;; SSE prefix for integer vector modes
(define_mode_attr sseintprefix
  [(V2DI "p") (V2DF "")
   (V4DI "p") (V4DF "")
   (V4SI "p") (V4SF "")
   (V8SI "p") (V8SF "")])

;; SSE scalar suffix for vector modes
(define_mode_attr ssescalarmodesuffix
  [(SF "ss") (DF "sd")
   (V8SF "ss") (V4DF "sd")
   (V4SF "ss") (V2DF "sd")
   (V8SI "ss") (V4DI "sd")
   (V4SI "d")])

;; Pack/unpack vector modes
(define_mode_attr sseunpackmode
  [(V16QI "V8HI") (V8HI "V4SI") (V4SI "V2DI")
   (V32QI "V16HI") (V16HI "V8SI") (V8SI "V4DI")])

(define_mode_attr ssepackmode
  [(V8HI "V16QI") (V4SI "V8HI") (V2DI "V4SI")
   (V16HI "V32QI") (V8SI "V16HI") (V4DI "V8SI")])

;; Mapping of the max integer size for xop rotate immediate constraint
(define_mode_attr sserotatemax
  [(V16QI "7") (V8HI "15") (V4SI "31") (V2DI "63")])

;; Mapping of mode to cast intrinsic name
(define_mode_attr castmode [(V8SI "si") (V8SF "ps") (V4DF "pd")])

;; Instruction suffix for sign and zero extensions.
(define_code_attr extsuffix [(sign_extend "sx") (zero_extend "zx")])

;; i128 for integer vectors and TARGET_AVX2, f128 otherwise.
(define_mode_attr i128
  [(V8SF "f128") (V4DF "f128") (V32QI "%~128") (V16HI "%~128")
   (V8SI "%~128") (V4DI "%~128")])

;; Mix-n-match
(define_mode_iterator AVX256MODE2P [V8SI V8SF V4DF])

;; Mapping of immediate bits for blend instructions
(define_mode_attr blendbits
  [(V8SF "255") (V4SF "15") (V4DF "15") (V2DF "3")])

;; Patterns whose name begins with "sse{,2,3}_" are invoked by intrinsics.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Move patterns
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; All of these patterns are enabled for SSE1 as well as SSE2.
;; This is essential for maintaining stable calling conventions.

(define_expand "mov<mode>"
  [(set (match_operand:V16 0 "nonimmediate_operand")
	(match_operand:V16 1 "nonimmediate_operand"))]
  "TARGET_SSE"
{
  ix86_expand_vector_move (<MODE>mode, operands);
  DONE;
})

(define_insn "*mov<mode>_internal"
  [(set (match_operand:V16 0 "nonimmediate_operand"               "=x,x ,m")
	(match_operand:V16 1 "nonimmediate_or_sse_const_operand"  "C ,xm,x"))]
  "TARGET_SSE
   && (register_operand (operands[0], <MODE>mode)
       || register_operand (operands[1], <MODE>mode))"
{
  switch (which_alternative)
    {
    case 0:
      return standard_sse_constant_opcode (insn, operands[1]);
    case 1:
    case 2:
      switch (get_attr_mode (insn))
	{
	case MODE_V8SF:
	case MODE_V4SF:
	  if (TARGET_AVX
	      && (misaligned_operand (operands[0], <MODE>mode)
		  || misaligned_operand (operands[1], <MODE>mode)))
	    return "vmovups\t{%1, %0|%0, %1}";
	  else
	    return "%vmovaps\t{%1, %0|%0, %1}";

	case MODE_V4DF:
	case MODE_V2DF:
	  if (TARGET_AVX
	      && (misaligned_operand (operands[0], <MODE>mode)
		  || misaligned_operand (operands[1], <MODE>mode)))
	    return "vmovupd\t{%1, %0|%0, %1}";
	  else
	    return "%vmovapd\t{%1, %0|%0, %1}";

	case MODE_OI:
	case MODE_TI:
	  if (TARGET_AVX
	      && (misaligned_operand (operands[0], <MODE>mode)
		  || misaligned_operand (operands[1], <MODE>mode)))
	    return "vmovdqu\t{%1, %0|%0, %1}";
	  else
	    return "%vmovdqa\t{%1, %0|%0, %1}";

	default:
	  gcc_unreachable ();
	}
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "type" "sselog1,ssemov,ssemov")
   (set_attr "prefix" "maybe_vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "<ssePSmode>")
	       (and (eq_attr "alternative" "2")
		    (match_test "TARGET_SSE_TYPELESS_STORES"))
		 (const_string "<ssePSmode>")
	       (match_test "TARGET_AVX")
		 (const_string "<sseinsnmode>")
	       (ior (not (match_test "TARGET_SSE2"))
		    (match_test "optimize_function_for_size_p (cfun)"))
		 (const_string "V4SF")
	       (and (eq_attr "alternative" "0")
		    (match_test "TARGET_SSE_LOAD0_BY_PXOR"))
		 (const_string "TI")
	      ]
	      (const_string "<sseinsnmode>")))])

(define_insn "sse2_movq128"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(vec_concat:V2DI
	  (vec_select:DI
	    (match_operand:V2DI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0)]))
	  (const_int 0)))]
  "TARGET_SSE2"
  "%vmovq\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

;; Move a DI from a 32-bit register pair (e.g. %edx:%eax) to an xmm.
;; We'd rather avoid this entirely; if the 32-bit reg pair was loaded
;; from memory, we'd prefer to load the memory directly into the %xmm
;; register.  To facilitate this happy circumstance, this pattern won't
;; split until after register allocation.  If the 64-bit value didn't
;; come from memory, this is the best we can do.  This is much better
;; than storing %edx:%eax into a stack temporary and loading an %xmm
;; from there.

(define_insn_and_split "movdi_to_sse"
  [(parallel
    [(set (match_operand:V4SI 0 "register_operand" "=?x,x")
	  (subreg:V4SI (match_operand:DI 1 "nonimmediate_operand" "r,m") 0))
     (clobber (match_scratch:V4SI 2 "=&x,X"))])]
  "!TARGET_64BIT && TARGET_SSE2 && TARGET_INTER_UNIT_MOVES"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
 if (register_operand (operands[1], DImode))
   {
      /* The DImode arrived in a pair of integral registers (e.g. %edx:%eax).
	 Assemble the 64-bit DImode value in an xmm register.  */
      emit_insn (gen_sse2_loadld (operands[0], CONST0_RTX (V4SImode),
				  gen_rtx_SUBREG (SImode, operands[1], 0)));
      emit_insn (gen_sse2_loadld (operands[2], CONST0_RTX (V4SImode),
				  gen_rtx_SUBREG (SImode, operands[1], 4)));
      emit_insn (gen_vec_interleave_lowv4si (operands[0], operands[0],
					     operands[2]));
    }
 else if (memory_operand (operands[1], DImode))
   emit_insn (gen_vec_concatv2di (gen_lowpart (V2DImode, operands[0]),
				  operands[1], const0_rtx));
 else
   gcc_unreachable ();
})

(define_split
  [(set (match_operand:V4SF 0 "register_operand")
	(match_operand:V4SF 1 "zero_extended_scalar_load_operand"))]
  "TARGET_SSE && reload_completed"
  [(set (match_dup 0)
	(vec_merge:V4SF
	  (vec_duplicate:V4SF (match_dup 1))
	  (match_dup 2)
	  (const_int 1)))]
{
  operands[1] = simplify_gen_subreg (SFmode, operands[1], V4SFmode, 0);
  operands[2] = CONST0_RTX (V4SFmode);
})

(define_split
  [(set (match_operand:V2DF 0 "register_operand")
	(match_operand:V2DF 1 "zero_extended_scalar_load_operand"))]
  "TARGET_SSE2 && reload_completed"
  [(set (match_dup 0) (vec_concat:V2DF (match_dup 1) (match_dup 2)))]
{
  operands[1] = simplify_gen_subreg (DFmode, operands[1], V2DFmode, 0);
  operands[2] = CONST0_RTX (DFmode);
})

(define_expand "push<mode>1"
  [(match_operand:V16 0 "register_operand")]
  "TARGET_SSE"
{
  ix86_expand_push (<MODE>mode, operands[0]);
  DONE;
})

(define_expand "movmisalign<mode>"
  [(set (match_operand:V16 0 "nonimmediate_operand")
	(match_operand:V16 1 "nonimmediate_operand"))]
  "TARGET_SSE"
{
  ix86_expand_vector_move_misalign (<MODE>mode, operands);
  DONE;
})

(define_insn "<sse>_loadu<ssemodesuffix><avxsizesuffix>"
  [(set (match_operand:VF 0 "register_operand" "=x")
	(unspec:VF
	  [(match_operand:VF 1 "memory_operand" "m")]
	  UNSPEC_LOADU))]
  "TARGET_SSE"
{
  switch (get_attr_mode (insn))
    {
    case MODE_V8SF:
    case MODE_V4SF:
      return "%vmovups\t{%1, %0|%0, %1}";
    default:
      return "%vmovu<ssemodesuffix>\t{%1, %0|%0, %1}";
    }
}
  [(set_attr "type" "ssemov")
   (set_attr "movu" "1")
   (set_attr "ssememalign" "8")
   (set_attr "prefix" "maybe_vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "<ssePSmode>")
	       (match_test "TARGET_AVX")
		 (const_string "<MODE>")
	       (match_test "optimize_function_for_size_p (cfun)")
		 (const_string "V4SF")
	      ]
	      (const_string "<MODE>")))])

(define_insn "<sse>_storeu<ssemodesuffix><avxsizesuffix>"
  [(set (match_operand:VF 0 "memory_operand" "=m")
	(unspec:VF
	  [(match_operand:VF 1 "register_operand" "x")]
	  UNSPEC_STOREU))]
  "TARGET_SSE"
{
  switch (get_attr_mode (insn))
    {
    case MODE_V8SF:
    case MODE_V4SF:
      return "%vmovups\t{%1, %0|%0, %1}";
    default:
      return "%vmovu<ssemodesuffix>\t{%1, %0|%0, %1}";
    }
}
  [(set_attr "type" "ssemov")
   (set_attr "movu" "1")
   (set_attr "ssememalign" "8")
   (set_attr "prefix" "maybe_vex")
   (set (attr "mode")
	(cond [(ior (match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		    (match_test "TARGET_SSE_TYPELESS_STORES"))
		 (const_string "<ssePSmode>")
	       (match_test "TARGET_AVX")
		 (const_string "<MODE>")
	       (match_test "optimize_function_for_size_p (cfun)")
		 (const_string "V4SF")
	      ]
	      (const_string "<MODE>")))])

(define_insn "<sse2>_loaddqu<avxsizesuffix>"
  [(set (match_operand:VI1 0 "register_operand" "=x")
	(unspec:VI1 [(match_operand:VI1 1 "memory_operand" "m")]
		    UNSPEC_LOADU))]
  "TARGET_SSE2"
{
  switch (get_attr_mode (insn))
    {
    case MODE_V8SF:
    case MODE_V4SF:
      return "%vmovups\t{%1, %0|%0, %1}";
    default:
      return "%vmovdqu\t{%1, %0|%0, %1}";
    }
}
  [(set_attr "type" "ssemov")
   (set_attr "movu" "1")
   (set_attr "ssememalign" "8")
   (set (attr "prefix_data16")
     (if_then_else
       (match_test "TARGET_AVX")
     (const_string "*")
     (const_string "1")))
   (set_attr "prefix" "maybe_vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "<ssePSmode>")
	       (match_test "TARGET_AVX")
		 (const_string "<sseinsnmode>")
	       (match_test "optimize_function_for_size_p (cfun)")
	         (const_string "V4SF")
	      ]
	      (const_string "<sseinsnmode>")))])

(define_insn "<sse2>_storedqu<avxsizesuffix>"
  [(set (match_operand:VI1 0 "memory_operand" "=m")
	(unspec:VI1 [(match_operand:VI1 1 "register_operand" "x")]
		    UNSPEC_STOREU))]
  "TARGET_SSE2"
{
  switch (get_attr_mode (insn))
    {
    case MODE_V8SF:
    case MODE_V4SF:
      return "%vmovups\t{%1, %0|%0, %1}";
    default:
      return "%vmovdqu\t{%1, %0|%0, %1}";
    }
}
  [(set_attr "type" "ssemov")
   (set_attr "movu" "1")
   (set_attr "ssememalign" "8")
   (set (attr "prefix_data16")
     (if_then_else
       (match_test "TARGET_AVX")
     (const_string "*")
     (const_string "1")))
   (set_attr "prefix" "maybe_vex")
   (set (attr "mode")
	(cond [(ior (match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		    (match_test "TARGET_SSE_TYPELESS_STORES"))
		 (const_string "<ssePSmode>")
	       (match_test "TARGET_AVX")
		 (const_string "<sseinsnmode>")
	       (match_test "optimize_function_for_size_p (cfun)")
	         (const_string "V4SF")
	      ]
	      (const_string "<sseinsnmode>")))])

(define_insn "<sse3>_lddqu<avxsizesuffix>"
  [(set (match_operand:VI1 0 "register_operand" "=x")
	(unspec:VI1 [(match_operand:VI1 1 "memory_operand" "m")]
		    UNSPEC_LDDQU))]
  "TARGET_SSE3"
  "%vlddqu\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "movu" "1")
   (set_attr "ssememalign" "8")
   (set (attr "prefix_data16")
     (if_then_else
       (match_test "TARGET_AVX")
     (const_string "*")
     (const_string "0")))
   (set (attr "prefix_rep")
     (if_then_else
       (match_test "TARGET_AVX")
     (const_string "*")
     (const_string "1")))
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "sse2_movnti<mode>"
  [(set (match_operand:SWI48 0 "memory_operand" "=m")
	(unspec:SWI48 [(match_operand:SWI48 1 "register_operand" "r")]
		      UNSPEC_MOVNT))]
  "TARGET_SSE2"
  "movnti\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_data16" "0")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse>_movnt<mode>"
  [(set (match_operand:VF 0 "memory_operand" "=m")
	(unspec:VF [(match_operand:VF 1 "register_operand" "x")]
		   UNSPEC_MOVNT))]
  "TARGET_SSE"
  "%vmovnt<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse2>_movnt<mode>"
  [(set (match_operand:VI8 0 "memory_operand" "=m")
	(unspec:VI8 [(match_operand:VI8 1 "register_operand" "x")]
		    UNSPEC_MOVNT))]
  "TARGET_SSE2"
  "%vmovntdq\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set (attr "prefix_data16")
     (if_then_else
       (match_test "TARGET_AVX")
     (const_string "*")
     (const_string "1")))
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<sseinsnmode>")])

; Expand patterns for non-temporal stores.  At the moment, only those
; that directly map to insns are defined; it would be possible to
; define patterns for other modes that would expand to several insns.

;; Modes handled by storent patterns.
(define_mode_iterator STORENT_MODE
  [(DI "TARGET_SSE2 && TARGET_64BIT") (SI "TARGET_SSE2")
   (SF "TARGET_SSE4A") (DF "TARGET_SSE4A")
   (V4DI "TARGET_AVX") (V2DI "TARGET_SSE2")
   (V8SF "TARGET_AVX") V4SF
   (V4DF "TARGET_AVX") (V2DF "TARGET_SSE2")])

(define_expand "storent<mode>"
  [(set (match_operand:STORENT_MODE 0 "memory_operand")
	(unspec:STORENT_MODE
	  [(match_operand:STORENT_MODE 1 "register_operand")]
	  UNSPEC_MOVNT))]
  "TARGET_SSE")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel floating point arithmetic
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_expand "<code><mode>2"
  [(set (match_operand:VF 0 "register_operand")
	(absneg:VF
	  (match_operand:VF 1 "register_operand")))]
  "TARGET_SSE"
  "ix86_expand_fp_absneg_operator (<CODE>, <MODE>mode, operands); DONE;")

(define_insn_and_split "*absneg<mode>2"
  [(set (match_operand:VF 0 "register_operand" "=x,x,x,x")
	(match_operator:VF 3 "absneg_operator"
	  [(match_operand:VF 1 "nonimmediate_operand" "0, xm,x, m")]))
   (use (match_operand:VF 2 "nonimmediate_operand"    "xm,0, xm,x"))]
  "TARGET_SSE"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
  enum rtx_code absneg_op;
  rtx op1, op2;
  rtx t;

  if (TARGET_AVX)
    {
      if (MEM_P (operands[1]))
	op1 = operands[2], op2 = operands[1];
      else
	op1 = operands[1], op2 = operands[2];
    }
  else
    {
      op1 = operands[0];
      if (rtx_equal_p (operands[0], operands[1]))
	op2 = operands[2];
      else
	op2 = operands[1];
    }

  absneg_op = GET_CODE (operands[3]) == NEG ? XOR : AND;
  t = gen_rtx_fmt_ee (absneg_op, <MODE>mode, op1, op2);
  t = gen_rtx_SET (VOIDmode, operands[0], t);
  emit_insn (t);
  DONE;
}
  [(set_attr "isa" "noavx,noavx,avx,avx")])

(define_expand "<plusminus_insn><mode>3"
  [(set (match_operand:VF 0 "register_operand")
	(plusminus:VF
	  (match_operand:VF 1 "nonimmediate_operand")
	  (match_operand:VF 2 "nonimmediate_operand")))]
  "TARGET_SSE"
  "ix86_fixup_binary_operands_no_copy (<CODE>, <MODE>mode, operands);")

(define_insn "*<plusminus_insn><mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(plusminus:VF
	  (match_operand:VF 1 "nonimmediate_operand" "<comm>0,x")
	  (match_operand:VF 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE && ix86_binary_operator_ok (<CODE>, <MODE>mode, operands)"
  "@
   <plusminus_mnemonic><ssemodesuffix>\t{%2, %0|%0, %2}
   v<plusminus_mnemonic><ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse>_vm<plusminus_insn><mode>3"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (plusminus:VF_128
	    (match_operand:VF_128 1 "register_operand" "0,x")
	    (match_operand:VF_128 2 "nonimmediate_operand" "xm,xm"))
	  (match_dup 1)
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   <plusminus_mnemonic><ssescalarmodesuffix>\t{%2, %0|%0, %2}
   v<plusminus_mnemonic><ssescalarmodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<ssescalarmode>")])

(define_expand "mul<mode>3"
  [(set (match_operand:VF 0 "register_operand")
	(mult:VF
	  (match_operand:VF 1 "nonimmediate_operand")
	  (match_operand:VF 2 "nonimmediate_operand")))]
  "TARGET_SSE"
  "ix86_fixup_binary_operands_no_copy (MULT, <MODE>mode, operands);")

(define_insn "*mul<mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(mult:VF
	  (match_operand:VF 1 "nonimmediate_operand" "%0,x")
	  (match_operand:VF 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE && ix86_binary_operator_ok (MULT, <MODE>mode, operands)"
  "@
   mul<ssemodesuffix>\t{%2, %0|%0, %2}
   vmul<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssemul")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "direct,double")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse>_vmmul<mode>3"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (mult:VF_128
	    (match_operand:VF_128 1 "register_operand" "0,x")
	    (match_operand:VF_128 2 "nonimmediate_operand" "xm,xm"))
	  (match_dup 1)
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   mul<ssescalarmodesuffix>\t{%2, %0|%0, %2}
   vmul<ssescalarmodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssemul")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<ssescalarmode>")])

(define_expand "div<mode>3"
  [(set (match_operand:VF2 0 "register_operand")
	(div:VF2 (match_operand:VF2 1 "register_operand")
		 (match_operand:VF2 2 "nonimmediate_operand")))]
  "TARGET_SSE2"
  "ix86_fixup_binary_operands_no_copy (DIV, <MODE>mode, operands);")

(define_expand "div<mode>3"
  [(set (match_operand:VF1 0 "register_operand")
	(div:VF1 (match_operand:VF1 1 "register_operand")
		 (match_operand:VF1 2 "nonimmediate_operand")))]
  "TARGET_SSE"
{
  ix86_fixup_binary_operands_no_copy (DIV, <MODE>mode, operands);

  if (TARGET_SSE_MATH
      && TARGET_RECIP_VEC_DIV
      && !optimize_insn_for_size_p ()
      && flag_finite_math_only && !flag_trapping_math
      && flag_unsafe_math_optimizations)
    {
      ix86_emit_swdivsf (operands[0], operands[1], operands[2], <MODE>mode);
      DONE;
    }
})

(define_insn "<sse>_div<mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(div:VF
	  (match_operand:VF 1 "register_operand" "0,x")
	  (match_operand:VF 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE"
  "@
   div<ssemodesuffix>\t{%2, %0|%0, %2}
   vdiv<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssediv")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse>_vmdiv<mode>3"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (div:VF_128
	    (match_operand:VF_128 1 "register_operand" "0,x")
	    (match_operand:VF_128 2 "nonimmediate_operand" "xm,xm"))
	  (match_dup 1)
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   div<ssescalarmodesuffix>\t{%2, %0|%0, %2}
   vdiv<ssescalarmodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssediv")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "direct,double")
   (set_attr "mode" "<ssescalarmode>")])

(define_insn "<sse>_rcp<mode>2"
  [(set (match_operand:VF1 0 "register_operand" "=x")
	(unspec:VF1
	  [(match_operand:VF1 1 "nonimmediate_operand" "xm")] UNSPEC_RCP))]
  "TARGET_SSE"
  "%vrcpps\t{%1, %0|%0, %1}"
  [(set_attr "type" "sse")
   (set_attr "atom_sse_attr" "rcp")
   (set_attr "btver2_sse_attr" "rcp")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<MODE>")])

(define_insn "sse_vmrcpv4sf2"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x")
	(vec_merge:V4SF
	  (unspec:V4SF [(match_operand:V4SF 1 "nonimmediate_operand" "xm,xm")]
		       UNSPEC_RCP)
	  (match_operand:V4SF 2 "register_operand" "0,x")
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   rcpss\t{%1, %0|%0, %1}
   vrcpss\t{%1, %2, %0|%0, %2, %1}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sse")
   (set_attr "ssememalign" "32")
   (set_attr "atom_sse_attr" "rcp")
   (set_attr "btver2_sse_attr" "rcp")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "SF")])

(define_expand "sqrt<mode>2"
  [(set (match_operand:VF2 0 "register_operand")
	(sqrt:VF2 (match_operand:VF2 1 "nonimmediate_operand")))]
  "TARGET_SSE2")

(define_expand "sqrt<mode>2"
  [(set (match_operand:VF1 0 "register_operand")
	(sqrt:VF1 (match_operand:VF1 1 "nonimmediate_operand")))]
  "TARGET_SSE"
{
  if (TARGET_SSE_MATH
      && TARGET_RECIP_VEC_SQRT
      && !optimize_insn_for_size_p ()
      && flag_finite_math_only && !flag_trapping_math
      && flag_unsafe_math_optimizations)
    {
      ix86_emit_swsqrtsf (operands[0], operands[1], <MODE>mode, false);
      DONE;
    }
})

(define_insn "<sse>_sqrt<mode>2"
  [(set (match_operand:VF 0 "register_operand" "=x")
	(sqrt:VF (match_operand:VF 1 "nonimmediate_operand" "xm")))]
  "TARGET_SSE"
  "%vsqrt<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "sse")
   (set_attr "atom_sse_attr" "sqrt")
   (set_attr "btver2_sse_attr" "sqrt")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse>_vmsqrt<mode>2"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (sqrt:VF_128
	    (match_operand:VF_128 1 "nonimmediate_operand" "xm,xm"))
	  (match_operand:VF_128 2 "register_operand" "0,x")
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   sqrt<ssescalarmodesuffix>\t{%1, %0|%0, %1}
   vsqrt<ssescalarmodesuffix>\t{%1, %2, %0|%0, %2, %1}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sse")
   (set_attr "atom_sse_attr" "sqrt")
   (set_attr "btver2_sse_attr" "sqrt")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<ssescalarmode>")])

(define_expand "rsqrt<mode>2"
  [(set (match_operand:VF1 0 "register_operand")
	(unspec:VF1
	  [(match_operand:VF1 1 "nonimmediate_operand")] UNSPEC_RSQRT))]
  "TARGET_SSE_MATH"
{
  ix86_emit_swsqrtsf (operands[0], operands[1], <MODE>mode, true);
  DONE;
})

(define_insn "<sse>_rsqrt<mode>2"
  [(set (match_operand:VF1 0 "register_operand" "=x")
	(unspec:VF1
	  [(match_operand:VF1 1 "nonimmediate_operand" "xm")] UNSPEC_RSQRT))]
  "TARGET_SSE"
  "%vrsqrtps\t{%1, %0|%0, %1}"
  [(set_attr "type" "sse")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<MODE>")])

(define_insn "sse_vmrsqrtv4sf2"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x")
	(vec_merge:V4SF
	  (unspec:V4SF [(match_operand:V4SF 1 "nonimmediate_operand" "xm,xm")]
		       UNSPEC_RSQRT)
	  (match_operand:V4SF 2 "register_operand" "0,x")
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   rsqrtss\t{%1, %0|%0, %1}
   vrsqrtss\t{%1, %2, %0|%0, %2, %1}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sse")
   (set_attr "ssememalign" "32")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "SF")])

;; ??? For !flag_finite_math_only, the representation with SMIN/SMAX
;; isn't really correct, as those rtl operators aren't defined when
;; applied to NaNs.  Hopefully the optimizers won't get too smart on us.

(define_expand "<code><mode>3"
  [(set (match_operand:VF 0 "register_operand")
	(smaxmin:VF
	  (match_operand:VF 1 "nonimmediate_operand")
	  (match_operand:VF 2 "nonimmediate_operand")))]
  "TARGET_SSE"
{
  if (!flag_finite_math_only)
    operands[1] = force_reg (<MODE>mode, operands[1]);
  ix86_fixup_binary_operands_no_copy (<CODE>, <MODE>mode, operands);
})

(define_insn "*<code><mode>3_finite"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(smaxmin:VF
	  (match_operand:VF 1 "nonimmediate_operand" "%0,x")
	  (match_operand:VF 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE && flag_finite_math_only
   && ix86_binary_operator_ok (<CODE>, <MODE>mode, operands)"
  "@
   <maxmin_float><ssemodesuffix>\t{%2, %0|%0, %2}
   v<maxmin_float><ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "btver2_sse_attr" "maxmin")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_insn "*<code><mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(smaxmin:VF
	  (match_operand:VF 1 "register_operand" "0,x")
	  (match_operand:VF 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE && !flag_finite_math_only"
  "@
   <maxmin_float><ssemodesuffix>\t{%2, %0|%0, %2}
   v<maxmin_float><ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "btver2_sse_attr" "maxmin")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse>_vm<code><mode>3"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (smaxmin:VF_128
	    (match_operand:VF_128 1 "register_operand" "0,x")
	    (match_operand:VF_128 2 "nonimmediate_operand" "xm,xm"))
	 (match_dup 1)
	 (const_int 1)))]
  "TARGET_SSE"
  "@
   <maxmin_float><ssescalarmodesuffix>\t{%2, %0|%0, %2}
   v<maxmin_float><ssescalarmodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sse")
   (set_attr "btver2_sse_attr" "maxmin")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<ssescalarmode>")])

;; These versions of the min/max patterns implement exactly the operations
;;   min = (op1 < op2 ? op1 : op2)
;;   max = (!(op1 < op2) ? op1 : op2)
;; Their operands are not commutative, and thus they may be used in the
;; presence of -0.0 and NaN.

(define_insn "*ieee_smin<mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(unspec:VF
	  [(match_operand:VF 1 "register_operand" "0,x")
	   (match_operand:VF 2 "nonimmediate_operand" "xm,xm")]
	 UNSPEC_IEEE_MIN))]
  "TARGET_SSE"
  "@
   min<ssemodesuffix>\t{%2, %0|%0, %2}
   vmin<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_insn "*ieee_smax<mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(unspec:VF
	  [(match_operand:VF 1 "register_operand" "0,x")
	   (match_operand:VF 2 "nonimmediate_operand" "xm,xm")]
	 UNSPEC_IEEE_MAX))]
  "TARGET_SSE"
  "@
   max<ssemodesuffix>\t{%2, %0|%0, %2}
   vmax<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_insn "avx_addsubv4df3"
  [(set (match_operand:V4DF 0 "register_operand" "=x")
	(vec_merge:V4DF
	  (plus:V4DF
	    (match_operand:V4DF 1 "register_operand" "x")
	    (match_operand:V4DF 2 "nonimmediate_operand" "xm"))
	  (minus:V4DF (match_dup 1) (match_dup 2))
	  (const_int 10)))]
  "TARGET_AVX"
  "vaddsubpd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseadd")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

(define_insn "sse3_addsubv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=x,x")
	(vec_merge:V2DF
	  (plus:V2DF
	    (match_operand:V2DF 1 "register_operand" "0,x")
	    (match_operand:V2DF 2 "nonimmediate_operand" "xm,xm"))
	  (minus:V2DF (match_dup 1) (match_dup 2))
	  (const_int 2)))]
  "TARGET_SSE3"
  "@
   addsubpd\t{%2, %0|%0, %2}
   vaddsubpd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "atom_unit" "complex")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V2DF")])

(define_insn "avx_addsubv8sf3"
  [(set (match_operand:V8SF 0 "register_operand" "=x")
	(vec_merge:V8SF
	  (plus:V8SF
	    (match_operand:V8SF 1 "register_operand" "x")
	    (match_operand:V8SF 2 "nonimmediate_operand" "xm"))
	  (minus:V8SF (match_dup 1) (match_dup 2))
	  (const_int 170)))]
  "TARGET_AVX"
  "vaddsubps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseadd")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V8SF")])

(define_insn "sse3_addsubv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x")
	(vec_merge:V4SF
	  (plus:V4SF
	    (match_operand:V4SF 1 "register_operand" "0,x")
	    (match_operand:V4SF 2 "nonimmediate_operand" "xm,xm"))
	  (minus:V4SF (match_dup 1) (match_dup 2))
	  (const_int 10)))]
  "TARGET_SSE3"
  "@
   addsubps\t{%2, %0|%0, %2}
   vaddsubps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "prefix" "orig,vex")
   (set_attr "prefix_rep" "1,*")
   (set_attr "mode" "V4SF")])

(define_insn "avx_h<plusminus_insn>v4df3"
  [(set (match_operand:V4DF 0 "register_operand" "=x")
	(vec_concat:V4DF
	  (vec_concat:V2DF
	    (plusminus:DF
	      (vec_select:DF
		(match_operand:V4DF 1 "register_operand" "x")
		(parallel [(const_int 0)]))
	      (vec_select:DF (match_dup 1) (parallel [(const_int 1)])))
	    (plusminus:DF
	      (vec_select:DF
		(match_operand:V4DF 2 "nonimmediate_operand" "xm")
		(parallel [(const_int 0)]))
	      (vec_select:DF (match_dup 2) (parallel [(const_int 1)]))))
	  (vec_concat:V2DF
	    (plusminus:DF
	      (vec_select:DF (match_dup 1) (parallel [(const_int 2)]))
	      (vec_select:DF (match_dup 1) (parallel [(const_int 3)])))
	    (plusminus:DF
	      (vec_select:DF (match_dup 2) (parallel [(const_int 2)]))
	      (vec_select:DF (match_dup 2) (parallel [(const_int 3)]))))))]
  "TARGET_AVX"
  "vh<plusminus_mnemonic>pd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseadd")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

(define_expand "sse3_haddv2df3"
  [(set (match_operand:V2DF 0 "register_operand")
	(vec_concat:V2DF
	  (plus:DF
	    (vec_select:DF
	      (match_operand:V2DF 1 "register_operand")
	      (parallel [(const_int 0)]))
	    (vec_select:DF (match_dup 1) (parallel [(const_int 1)])))
	  (plus:DF
	    (vec_select:DF
	      (match_operand:V2DF 2 "nonimmediate_operand")
	      (parallel [(const_int 0)]))
	    (vec_select:DF (match_dup 2) (parallel [(const_int 1)])))))]
  "TARGET_SSE3")

(define_insn "*sse3_haddv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=x,x")
	(vec_concat:V2DF
	  (plus:DF
	    (vec_select:DF
	      (match_operand:V2DF 1 "register_operand" "0,x")
	      (parallel [(match_operand:SI 3 "const_0_to_1_operand")]))
	    (vec_select:DF
	      (match_dup 1)
	      (parallel [(match_operand:SI 4 "const_0_to_1_operand")])))
	  (plus:DF
	    (vec_select:DF
	      (match_operand:V2DF 2 "nonimmediate_operand" "xm,xm")
	      (parallel [(match_operand:SI 5 "const_0_to_1_operand")]))
	    (vec_select:DF
	      (match_dup 2)
	      (parallel [(match_operand:SI 6 "const_0_to_1_operand")])))))]
  "TARGET_SSE3
   && INTVAL (operands[3]) != INTVAL (operands[4])
   && INTVAL (operands[5]) != INTVAL (operands[6])"
  "@
   haddpd\t{%2, %0|%0, %2}
   vhaddpd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V2DF")])

(define_insn "sse3_hsubv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=x,x")
	(vec_concat:V2DF
	  (minus:DF
	    (vec_select:DF
	      (match_operand:V2DF 1 "register_operand" "0,x")
	      (parallel [(const_int 0)]))
	    (vec_select:DF (match_dup 1) (parallel [(const_int 1)])))
	  (minus:DF
	    (vec_select:DF
	      (match_operand:V2DF 2 "nonimmediate_operand" "xm,xm")
	      (parallel [(const_int 0)]))
	    (vec_select:DF (match_dup 2) (parallel [(const_int 1)])))))]
  "TARGET_SSE3"
  "@
   hsubpd\t{%2, %0|%0, %2}
   vhsubpd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V2DF")])

(define_insn "*sse3_haddv2df3_low"
  [(set (match_operand:DF 0 "register_operand" "=x,x")
	(plus:DF
	  (vec_select:DF
	    (match_operand:V2DF 1 "register_operand" "0,x")
	    (parallel [(match_operand:SI 2 "const_0_to_1_operand")]))
	  (vec_select:DF
	    (match_dup 1)
	    (parallel [(match_operand:SI 3 "const_0_to_1_operand")]))))]
  "TARGET_SSE3
   && INTVAL (operands[2]) != INTVAL (operands[3])"
  "@
   haddpd\t{%0, %0|%0, %0}
   vhaddpd\t{%1, %1, %0|%0, %1, %1}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V2DF")])

(define_insn "*sse3_hsubv2df3_low"
  [(set (match_operand:DF 0 "register_operand" "=x,x")
	(minus:DF
	  (vec_select:DF
	    (match_operand:V2DF 1 "register_operand" "0,x")
	    (parallel [(const_int 0)]))
	  (vec_select:DF
	    (match_dup 1)
	    (parallel [(const_int 1)]))))]
  "TARGET_SSE3"
  "@
   hsubpd\t{%0, %0|%0, %0}
   vhsubpd\t{%1, %1, %0|%0, %1, %1}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V2DF")])

(define_insn "avx_h<plusminus_insn>v8sf3"
  [(set (match_operand:V8SF 0 "register_operand" "=x")
	(vec_concat:V8SF
	  (vec_concat:V4SF
	    (vec_concat:V2SF
	      (plusminus:SF
		(vec_select:SF
		  (match_operand:V8SF 1 "register_operand" "x")
		  (parallel [(const_int 0)]))
		(vec_select:SF (match_dup 1) (parallel [(const_int 1)])))
	      (plusminus:SF
		(vec_select:SF (match_dup 1) (parallel [(const_int 2)]))
		(vec_select:SF (match_dup 1) (parallel [(const_int 3)]))))
	    (vec_concat:V2SF
	      (plusminus:SF
		(vec_select:SF
		  (match_operand:V8SF 2 "nonimmediate_operand" "xm")
		  (parallel [(const_int 0)]))
		(vec_select:SF (match_dup 2) (parallel [(const_int 1)])))
	      (plusminus:SF
		(vec_select:SF (match_dup 2) (parallel [(const_int 2)]))
		(vec_select:SF (match_dup 2) (parallel [(const_int 3)])))))
	  (vec_concat:V4SF
	    (vec_concat:V2SF
	      (plusminus:SF
		(vec_select:SF (match_dup 1) (parallel [(const_int 4)]))
		(vec_select:SF (match_dup 1) (parallel [(const_int 5)])))
	      (plusminus:SF
		(vec_select:SF (match_dup 1) (parallel [(const_int 6)]))
		(vec_select:SF (match_dup 1) (parallel [(const_int 7)]))))
	    (vec_concat:V2SF
	      (plusminus:SF
		(vec_select:SF (match_dup 2) (parallel [(const_int 4)]))
		(vec_select:SF (match_dup 2) (parallel [(const_int 5)])))
	      (plusminus:SF
		(vec_select:SF (match_dup 2) (parallel [(const_int 6)]))
		(vec_select:SF (match_dup 2) (parallel [(const_int 7)])))))))]
  "TARGET_AVX"
  "vh<plusminus_mnemonic>ps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseadd")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V8SF")])

(define_insn "sse3_h<plusminus_insn>v4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x")
	(vec_concat:V4SF
	  (vec_concat:V2SF
	    (plusminus:SF
	      (vec_select:SF
		(match_operand:V4SF 1 "register_operand" "0,x")
		(parallel [(const_int 0)]))
	      (vec_select:SF (match_dup 1) (parallel [(const_int 1)])))
	    (plusminus:SF
	      (vec_select:SF (match_dup 1) (parallel [(const_int 2)]))
	      (vec_select:SF (match_dup 1) (parallel [(const_int 3)]))))
	  (vec_concat:V2SF
	    (plusminus:SF
	      (vec_select:SF
		(match_operand:V4SF 2 "nonimmediate_operand" "xm,xm")
		(parallel [(const_int 0)]))
	      (vec_select:SF (match_dup 2) (parallel [(const_int 1)])))
	    (plusminus:SF
	      (vec_select:SF (match_dup 2) (parallel [(const_int 2)]))
	      (vec_select:SF (match_dup 2) (parallel [(const_int 3)]))))))]
  "TARGET_SSE3"
  "@
   h<plusminus_mnemonic>ps\t{%2, %0|%0, %2}
   vh<plusminus_mnemonic>ps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseadd")
   (set_attr "atom_unit" "complex")
   (set_attr "prefix" "orig,vex")
   (set_attr "prefix_rep" "1,*")
   (set_attr "mode" "V4SF")])

(define_expand "reduc_splus_v4df"
  [(match_operand:V4DF 0 "register_operand")
   (match_operand:V4DF 1 "register_operand")]
  "TARGET_AVX"
{
  rtx tmp = gen_reg_rtx (V4DFmode);
  rtx tmp2 = gen_reg_rtx (V4DFmode);
  emit_insn (gen_avx_haddv4df3 (tmp, operands[1], operands[1]));
  emit_insn (gen_avx_vperm2f128v4df3 (tmp2, tmp, tmp, GEN_INT (1)));
  emit_insn (gen_addv4df3 (operands[0], tmp, tmp2));
  DONE;
})

(define_expand "reduc_splus_v2df"
  [(match_operand:V2DF 0 "register_operand")
   (match_operand:V2DF 1 "register_operand")]
  "TARGET_SSE3"
{
  emit_insn (gen_sse3_haddv2df3 (operands[0], operands[1], operands[1]));
  DONE;
})

(define_expand "reduc_splus_v8sf"
  [(match_operand:V8SF 0 "register_operand")
   (match_operand:V8SF 1 "register_operand")]
  "TARGET_AVX"
{
  rtx tmp = gen_reg_rtx (V8SFmode);
  rtx tmp2 = gen_reg_rtx (V8SFmode);
  emit_insn (gen_avx_haddv8sf3 (tmp, operands[1], operands[1]));
  emit_insn (gen_avx_haddv8sf3 (tmp2, tmp, tmp));
  emit_insn (gen_avx_vperm2f128v8sf3 (tmp, tmp2, tmp2, GEN_INT (1)));
  emit_insn (gen_addv8sf3 (operands[0], tmp, tmp2));
  DONE;
})

(define_expand "reduc_splus_v4sf"
  [(match_operand:V4SF 0 "register_operand")
   (match_operand:V4SF 1 "register_operand")]
  "TARGET_SSE"
{
  if (TARGET_SSE3)
    {
      rtx tmp = gen_reg_rtx (V4SFmode);
      emit_insn (gen_sse3_haddv4sf3 (tmp, operands[1], operands[1]));
      emit_insn (gen_sse3_haddv4sf3 (operands[0], tmp, tmp));
    }
  else
    ix86_expand_reduc (gen_addv4sf3, operands[0], operands[1]);
  DONE;
})

;; Modes handled by reduc_sm{in,ax}* patterns.
(define_mode_iterator REDUC_SMINMAX_MODE
  [(V32QI "TARGET_AVX2") (V16HI "TARGET_AVX2")
   (V8SI "TARGET_AVX2") (V4DI "TARGET_AVX2")
   (V8SF "TARGET_AVX") (V4DF "TARGET_AVX")
   (V4SF "TARGET_SSE")])

(define_expand "reduc_<code>_<mode>"
  [(smaxmin:REDUC_SMINMAX_MODE
     (match_operand:REDUC_SMINMAX_MODE 0 "register_operand")
     (match_operand:REDUC_SMINMAX_MODE 1 "register_operand"))]
  ""
{
  ix86_expand_reduc (gen_<code><mode>3, operands[0], operands[1]);
  DONE;
})

(define_expand "reduc_<code>_<mode>"
  [(umaxmin:VI_256
     (match_operand:VI_256 0 "register_operand")
     (match_operand:VI_256 1 "register_operand"))]
  "TARGET_AVX2"
{
  ix86_expand_reduc (gen_<code><mode>3, operands[0], operands[1]);
  DONE;
})

(define_expand "reduc_umin_v8hi"
  [(umin:V8HI
     (match_operand:V8HI 0 "register_operand")
     (match_operand:V8HI 1 "register_operand"))]
  "TARGET_SSE4_1"
{
  ix86_expand_reduc (gen_uminv8hi3, operands[0], operands[1]);
  DONE;
})

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel floating point comparisons
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_insn "avx_cmp<mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x")
	(unspec:VF
	  [(match_operand:VF 1 "register_operand" "x")
	   (match_operand:VF 2 "nonimmediate_operand" "xm")
	   (match_operand:SI 3 "const_0_to_31_operand" "n")]
	  UNSPEC_PCMP))]
  "TARGET_AVX"
  "vcmp<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssecmp")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<MODE>")])

(define_insn "avx_vmcmp<mode>3"
  [(set (match_operand:VF_128 0 "register_operand" "=x")
	(vec_merge:VF_128
	  (unspec:VF_128
	    [(match_operand:VF_128 1 "register_operand" "x")
	     (match_operand:VF_128 2 "nonimmediate_operand" "xm")
	     (match_operand:SI 3 "const_0_to_31_operand" "n")]
	    UNSPEC_PCMP)
	 (match_dup 1)
	 (const_int 1)))]
  "TARGET_AVX"
  "vcmp<ssescalarmodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssecmp")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<ssescalarmode>")])

(define_insn "*<sse>_maskcmp<mode>3_comm"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(match_operator:VF 3 "sse_comparison_operator"
	  [(match_operand:VF 1 "register_operand" "%0,x")
	   (match_operand:VF 2 "nonimmediate_operand" "xm,xm")]))]
  "TARGET_SSE
   && GET_RTX_CLASS (GET_CODE (operands[3])) == RTX_COMM_COMPARE"
  "@
   cmp%D3<ssemodesuffix>\t{%2, %0|%0, %2}
   vcmp%D3<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssecmp")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse>_maskcmp<mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(match_operator:VF 3 "sse_comparison_operator"
	  [(match_operand:VF 1 "register_operand" "0,x")
	   (match_operand:VF 2 "nonimmediate_operand" "xm,xm")]))]
  "TARGET_SSE"
  "@
   cmp%D3<ssemodesuffix>\t{%2, %0|%0, %2}
   vcmp%D3<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssecmp")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse>_vmmaskcmp<mode>3"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	 (match_operator:VF_128 3 "sse_comparison_operator"
	   [(match_operand:VF_128 1 "register_operand" "0,x")
	    (match_operand:VF_128 2 "nonimmediate_operand" "xm,xm")])
	 (match_dup 1)
	 (const_int 1)))]
  "TARGET_SSE"
  "@
   cmp%D3<ssescalarmodesuffix>\t{%2, %0|%0, %2}
   vcmp%D3<ssescalarmodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssecmp")
   (set_attr "length_immediate" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<ssescalarmode>")])

(define_insn "<sse>_comi"
  [(set (reg:CCFP FLAGS_REG)
	(compare:CCFP
	  (vec_select:MODEF
	    (match_operand:<ssevecmode> 0 "register_operand" "x")
	    (parallel [(const_int 0)]))
	  (vec_select:MODEF
	    (match_operand:<ssevecmode> 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0)]))))]
  "SSE_FLOAT_MODE_P (<MODE>mode)"
  "%vcomi<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecomi")
   (set_attr "prefix" "maybe_vex")
   (set_attr "prefix_rep" "0")
   (set (attr "prefix_data16")
	(if_then_else (eq_attr "mode" "DF")
		      (const_string "1")
		      (const_string "0")))
   (set_attr "mode" "<MODE>")])

(define_insn "<sse>_ucomi"
  [(set (reg:CCFPU FLAGS_REG)
	(compare:CCFPU
	  (vec_select:MODEF
	    (match_operand:<ssevecmode> 0 "register_operand" "x")
	    (parallel [(const_int 0)]))
	  (vec_select:MODEF
	    (match_operand:<ssevecmode> 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0)]))))]
  "SSE_FLOAT_MODE_P (<MODE>mode)"
  "%vucomi<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecomi")
   (set_attr "prefix" "maybe_vex")
   (set_attr "prefix_rep" "0")
   (set (attr "prefix_data16")
	(if_then_else (eq_attr "mode" "DF")
		      (const_string "1")
		      (const_string "0")))
   (set_attr "mode" "<MODE>")])

(define_expand "vcond<V_256:mode><VF_256:mode>"
  [(set (match_operand:V_256 0 "register_operand")
	(if_then_else:V_256
	  (match_operator 3 ""
	    [(match_operand:VF_256 4 "nonimmediate_operand")
	     (match_operand:VF_256 5 "nonimmediate_operand")])
	  (match_operand:V_256 1 "general_operand")
	  (match_operand:V_256 2 "general_operand")))]
  "TARGET_AVX
   && (GET_MODE_NUNITS (<V_256:MODE>mode)
       == GET_MODE_NUNITS (<VF_256:MODE>mode))"
{
  bool ok = ix86_expand_fp_vcond (operands);
  gcc_assert (ok);
  DONE;
})

(define_expand "vcond<V_128:mode><VF_128:mode>"
  [(set (match_operand:V_128 0 "register_operand")
	(if_then_else:V_128
	  (match_operator 3 ""
	    [(match_operand:VF_128 4 "nonimmediate_operand")
	     (match_operand:VF_128 5 "nonimmediate_operand")])
	  (match_operand:V_128 1 "general_operand")
	  (match_operand:V_128 2 "general_operand")))]
  "TARGET_SSE
   && (GET_MODE_NUNITS (<V_128:MODE>mode)
       == GET_MODE_NUNITS (<VF_128:MODE>mode))"
{
  bool ok = ix86_expand_fp_vcond (operands);
  gcc_assert (ok);
  DONE;
})

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel floating point logical operations
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_insn "<sse>_andnot<mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(and:VF
	  (not:VF
	    (match_operand:VF 1 "register_operand" "0,x"))
	  (match_operand:VF 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE"
{
  static char buf[32];
  const char *ops;
  const char *suffix;

  switch (get_attr_mode (insn))
    {
    case MODE_V8SF:
    case MODE_V4SF:
      suffix = "ps";
      break;
    default:
      suffix = "<ssemodesuffix>";
    }

  switch (which_alternative)
    {
    case 0:
      ops = "andn%s\t{%%2, %%0|%%0, %%2}";
      break;
    case 1:
      ops = "vandn%s\t{%%2, %%1, %%0|%%0, %%1, %%2}";
      break;
    default:
      gcc_unreachable ();
    }

  snprintf (buf, sizeof (buf), ops, suffix);
  return buf;
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix" "orig,vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "<ssePSmode>")
	       (match_test "TARGET_AVX")
		 (const_string "<MODE>")
	       (match_test "optimize_function_for_size_p (cfun)")
		 (const_string "V4SF")
	       ]
	       (const_string "<MODE>")))])

(define_expand "<code><mode>3"
  [(set (match_operand:VF 0 "register_operand")
	(any_logic:VF
	  (match_operand:VF 1 "nonimmediate_operand")
	  (match_operand:VF 2 "nonimmediate_operand")))]
  "TARGET_SSE"
  "ix86_fixup_binary_operands_no_copy (<CODE>, <MODE>mode, operands);")

(define_insn "*<code><mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(any_logic:VF
	  (match_operand:VF 1 "nonimmediate_operand" "%0,x")
	  (match_operand:VF 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE && ix86_binary_operator_ok (<CODE>, <MODE>mode, operands)"
{
  static char buf[32];
  const char *ops;
  const char *suffix;

  switch (get_attr_mode (insn))
    {
    case MODE_V8SF:
    case MODE_V4SF:
      suffix = "ps";
      break;
    default:
      suffix = "<ssemodesuffix>";
    }

  switch (which_alternative)
    {
    case 0:
      ops = "<logic>%s\t{%%2, %%0|%%0, %%2}";
      break;
    case 1:
      ops = "v<logic>%s\t{%%2, %%1, %%0|%%0, %%1, %%2}";
      break;
    default:
      gcc_unreachable ();
    }

  snprintf (buf, sizeof (buf), ops, suffix);
  return buf;
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix" "orig,vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "<ssePSmode>")
	       (match_test "TARGET_AVX")
		 (const_string "<MODE>")
	       (match_test "optimize_function_for_size_p (cfun)")
		 (const_string "V4SF")
	       ]
	       (const_string "<MODE>")))])

(define_expand "copysign<mode>3"
  [(set (match_dup 4)
	(and:VF
	  (not:VF (match_dup 3))
	  (match_operand:VF 1 "nonimmediate_operand")))
   (set (match_dup 5)
	(and:VF (match_dup 3)
		(match_operand:VF 2 "nonimmediate_operand")))
   (set (match_operand:VF 0 "register_operand")
	(ior:VF (match_dup 4) (match_dup 5)))]
  "TARGET_SSE"
{
  operands[3] = ix86_build_signbit_mask (<MODE>mode, 1, 0);

  operands[4] = gen_reg_rtx (<MODE>mode);
  operands[5] = gen_reg_rtx (<MODE>mode);
})

;; Also define scalar versions.  These are used for abs, neg, and
;; conditional move.  Using subregs into vector modes causes register
;; allocation lossage.  These patterns do not allow memory operands
;; because the native instructions read the full 128-bits.

(define_insn "*andnot<mode>3"
  [(set (match_operand:MODEF 0 "register_operand" "=x,x")
	(and:MODEF
	  (not:MODEF
	    (match_operand:MODEF 1 "register_operand" "0,x"))
	    (match_operand:MODEF 2 "register_operand" "x,x")))]
  "SSE_FLOAT_MODE_P (<MODE>mode)"
{
  static char buf[32];
  const char *ops;
  const char *suffix
    = (get_attr_mode (insn) == MODE_V4SF) ? "ps" : "<ssevecmodesuffix>";

  switch (which_alternative)
    {
    case 0:
      ops = "andn%s\t{%%2, %%0|%%0, %%2}";
      break;
    case 1:
      ops = "vandn%s\t{%%2, %%1, %%0|%%0, %%1, %%2}";
      break;
    default:
      gcc_unreachable ();
    }

  snprintf (buf, sizeof (buf), ops, suffix);
  return buf;
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix" "orig,vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "V4SF")
	       (match_test "TARGET_AVX")
		 (const_string "<ssevecmode>")
	       (match_test "optimize_function_for_size_p (cfun)")
		 (const_string "V4SF")
	       ]
	       (const_string "<ssevecmode>")))])

(define_insn "*andnottf3"
  [(set (match_operand:TF 0 "register_operand" "=x,x")
	(and:TF
	  (not:TF (match_operand:TF 1 "register_operand" "0,x"))
	  (match_operand:TF 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE"
{
  static char buf[32];
  const char *ops;
  const char *tmp
    = (get_attr_mode (insn) == MODE_V4SF) ? "andnps" : "pandn";

  switch (which_alternative)
    {
    case 0:
      ops = "%s\t{%%2, %%0|%%0, %%2}";
      break;
    case 1:
      ops = "v%s\t{%%2, %%1, %%0|%%0, %%1, %%2}";
      break;
    default:
      gcc_unreachable ();
    }

  snprintf (buf, sizeof (buf), ops, tmp);
  return buf;
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set (attr "prefix_data16")
     (if_then_else
       (and (eq_attr "alternative" "0")
	    (eq_attr "mode" "TI"))
       (const_string "1")
       (const_string "*")))
   (set_attr "prefix" "orig,vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "V4SF")
	       (match_test "TARGET_AVX")
		 (const_string "TI")
	       (ior (not (match_test "TARGET_SSE2"))
		    (match_test "optimize_function_for_size_p (cfun)"))
		 (const_string "V4SF")
	       ]
	       (const_string "TI")))])

(define_insn "*<code><mode>3"
  [(set (match_operand:MODEF 0 "register_operand" "=x,x")
	(any_logic:MODEF
	  (match_operand:MODEF 1 "register_operand" "%0,x")
	  (match_operand:MODEF 2 "register_operand" "x,x")))]
  "SSE_FLOAT_MODE_P (<MODE>mode)"
{
  static char buf[32];
  const char *ops;
  const char *suffix
    = (get_attr_mode (insn) == MODE_V4SF) ? "ps" : "<ssevecmodesuffix>";

  switch (which_alternative)
    {
    case 0:
      ops = "<logic>%s\t{%%2, %%0|%%0, %%2}";
      break;
    case 1:
      ops = "v<logic>%s\t{%%2, %%1, %%0|%%0, %%1, %%2}";
      break;
    default:
      gcc_unreachable ();
    }

  snprintf (buf, sizeof (buf), ops, suffix);
  return buf;
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix" "orig,vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "V4SF")
	       (match_test "TARGET_AVX")
		 (const_string "<ssevecmode>")
	       (match_test "optimize_function_for_size_p (cfun)")
		 (const_string "V4SF")
	       ]
	       (const_string "<ssevecmode>")))])

(define_expand "<code>tf3"
  [(set (match_operand:TF 0 "register_operand")
	(any_logic:TF
	  (match_operand:TF 1 "nonimmediate_operand")
	  (match_operand:TF 2 "nonimmediate_operand")))]
  "TARGET_SSE"
  "ix86_fixup_binary_operands_no_copy (<CODE>, TFmode, operands);")

(define_insn "*<code>tf3"
  [(set (match_operand:TF 0 "register_operand" "=x,x")
	(any_logic:TF
	  (match_operand:TF 1 "nonimmediate_operand" "%0,x")
	  (match_operand:TF 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE
   && ix86_binary_operator_ok (<CODE>, TFmode, operands)"
{
  static char buf[32];
  const char *ops;
  const char *tmp
    = (get_attr_mode (insn) == MODE_V4SF) ? "<logic>ps" : "p<logic>";

  switch (which_alternative)
    {
    case 0:
      ops = "%s\t{%%2, %%0|%%0, %%2}";
      break;
    case 1:
      ops = "v%s\t{%%2, %%1, %%0|%%0, %%1, %%2}";
      break;
    default:
      gcc_unreachable ();
    }

  snprintf (buf, sizeof (buf), ops, tmp);
  return buf;
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set (attr "prefix_data16")
     (if_then_else
       (and (eq_attr "alternative" "0")
	    (eq_attr "mode" "TI"))
       (const_string "1")
       (const_string "*")))
   (set_attr "prefix" "orig,vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "V4SF")
	       (match_test "TARGET_AVX")
		 (const_string "TI")
	       (ior (not (match_test "TARGET_SSE2"))
		    (match_test "optimize_function_for_size_p (cfun)"))
		 (const_string "V4SF")
	       ]
	       (const_string "TI")))])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; FMA floating point multiply/accumulate instructions.  These include
;; scalar versions of the instructions as well as vector versions.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; The standard names for scalar FMA are only available with SSE math enabled.
(define_mode_iterator FMAMODEM [(SF "TARGET_SSE_MATH")
				(DF "TARGET_SSE_MATH")
				V4SF V2DF V8SF V4DF])

(define_expand "fma<mode>4"
  [(set (match_operand:FMAMODEM 0 "register_operand")
	(fma:FMAMODEM
	  (match_operand:FMAMODEM 1 "nonimmediate_operand")
	  (match_operand:FMAMODEM 2 "nonimmediate_operand")
	  (match_operand:FMAMODEM 3 "nonimmediate_operand")))]
  "TARGET_FMA || TARGET_FMA4")

(define_expand "fms<mode>4"
  [(set (match_operand:FMAMODEM 0 "register_operand")
	(fma:FMAMODEM
	  (match_operand:FMAMODEM 1 "nonimmediate_operand")
	  (match_operand:FMAMODEM 2 "nonimmediate_operand")
	  (neg:FMAMODEM (match_operand:FMAMODEM 3 "nonimmediate_operand"))))]
  "TARGET_FMA || TARGET_FMA4")

(define_expand "fnma<mode>4"
  [(set (match_operand:FMAMODEM 0 "register_operand")
	(fma:FMAMODEM
	  (neg:FMAMODEM (match_operand:FMAMODEM 1 "nonimmediate_operand"))
	  (match_operand:FMAMODEM 2 "nonimmediate_operand")
	  (match_operand:FMAMODEM 3 "nonimmediate_operand")))]
  "TARGET_FMA || TARGET_FMA4")

(define_expand "fnms<mode>4"
  [(set (match_operand:FMAMODEM 0 "register_operand")
	(fma:FMAMODEM
	  (neg:FMAMODEM (match_operand:FMAMODEM 1 "nonimmediate_operand"))
	  (match_operand:FMAMODEM 2 "nonimmediate_operand")
	  (neg:FMAMODEM (match_operand:FMAMODEM 3 "nonimmediate_operand"))))]
  "TARGET_FMA || TARGET_FMA4")

;; The builtins for intrinsics are not constrained by SSE math enabled.
(define_mode_iterator FMAMODE [SF DF V4SF V2DF V8SF V4DF])

(define_expand "fma4i_fmadd_<mode>"
  [(set (match_operand:FMAMODE 0 "register_operand")
	(fma:FMAMODE
	  (match_operand:FMAMODE 1 "nonimmediate_operand")
	  (match_operand:FMAMODE 2 "nonimmediate_operand")
	  (match_operand:FMAMODE 3 "nonimmediate_operand")))]
  "TARGET_FMA || TARGET_FMA4")

(define_insn "*fma_fmadd_<mode>"
  [(set (match_operand:FMAMODE 0 "register_operand" "=x,x,x,x,x")
	(fma:FMAMODE
	  (match_operand:FMAMODE 1 "nonimmediate_operand" "%0, 0,x, x,x")
	  (match_operand:FMAMODE 2 "nonimmediate_operand" "xm, x,xm,x,m")
	  (match_operand:FMAMODE 3 "nonimmediate_operand" " x,xm,0,xm,x")))]
  "TARGET_FMA || TARGET_FMA4"
  "@
   vfmadd132<ssemodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfmadd213<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vfmadd231<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}
   vfmadd<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}
   vfmadd<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "fma,fma,fma,fma4,fma4")
   (set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fma_fmsub_<mode>"
  [(set (match_operand:FMAMODE 0 "register_operand" "=x,x,x,x,x")
	(fma:FMAMODE
	  (match_operand:FMAMODE   1 "nonimmediate_operand" "%0, 0,x, x,x")
	  (match_operand:FMAMODE   2 "nonimmediate_operand" "xm, x,xm,x,m")
	  (neg:FMAMODE
	    (match_operand:FMAMODE 3 "nonimmediate_operand" " x,xm,0,xm,x"))))]
  "TARGET_FMA || TARGET_FMA4"
  "@
   vfmsub132<ssemodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfmsub213<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vfmsub231<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}
   vfmsub<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}
   vfmsub<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "fma,fma,fma,fma4,fma4")
   (set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fma_fnmadd_<mode>"
  [(set (match_operand:FMAMODE 0 "register_operand" "=x,x,x,x,x")
	(fma:FMAMODE
	  (neg:FMAMODE
	    (match_operand:FMAMODE 1 "nonimmediate_operand" "%0, 0,x, x,x"))
	  (match_operand:FMAMODE   2 "nonimmediate_operand" "xm, x,xm,x,m")
	  (match_operand:FMAMODE   3 "nonimmediate_operand" " x,xm,0,xm,x")))]
  "TARGET_FMA || TARGET_FMA4"
  "@
   vfnmadd132<ssemodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfnmadd213<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vfnmadd231<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}
   vfnmadd<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}
   vfnmadd<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "fma,fma,fma,fma4,fma4")
   (set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fma_fnmsub_<mode>"
  [(set (match_operand:FMAMODE 0 "register_operand" "=x,x,x,x,x")
	(fma:FMAMODE
	  (neg:FMAMODE
	    (match_operand:FMAMODE 1 "nonimmediate_operand" "%0, 0,x, x,x"))
	  (match_operand:FMAMODE   2 "nonimmediate_operand" "xm, x,xm,x,m")
	  (neg:FMAMODE
	    (match_operand:FMAMODE 3 "nonimmediate_operand" " x,xm,0,xm,x"))))]
  "TARGET_FMA || TARGET_FMA4"
  "@
   vfnmsub132<ssemodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfnmsub213<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vfnmsub231<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}
   vfnmsub<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}
   vfnmsub<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "fma,fma,fma,fma4,fma4")
   (set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

;; FMA parallel floating point multiply addsub and subadd operations.

;; It would be possible to represent these without the UNSPEC as
;;
;; (vec_merge
;;   (fma op1 op2 op3)
;;   (fma op1 op2 (neg op3))
;;   (merge-const))
;;
;; But this doesn't seem useful in practice.

(define_expand "fmaddsub_<mode>"
  [(set (match_operand:VF 0 "register_operand")
	(unspec:VF
	  [(match_operand:VF 1 "nonimmediate_operand")
	   (match_operand:VF 2 "nonimmediate_operand")
	   (match_operand:VF 3 "nonimmediate_operand")]
	  UNSPEC_FMADDSUB))]
  "TARGET_FMA || TARGET_FMA4")

(define_insn "*fma_fmaddsub_<mode>"
  [(set (match_operand:VF 0 "register_operand" "=x,x,x,x,x")
	(unspec:VF
	  [(match_operand:VF 1 "nonimmediate_operand" "%0, 0,x, x,x")
	   (match_operand:VF 2 "nonimmediate_operand" "xm, x,xm,x,m")
	   (match_operand:VF 3 "nonimmediate_operand" " x,xm,0,xm,x")]
	  UNSPEC_FMADDSUB))]
  "TARGET_FMA || TARGET_FMA4"
  "@
   vfmaddsub132<ssemodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfmaddsub213<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vfmaddsub231<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}
   vfmaddsub<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}
   vfmaddsub<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "fma,fma,fma,fma4,fma4")
   (set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fma_fmsubadd_<mode>"
  [(set (match_operand:VF 0 "register_operand" "=x,x,x,x,x")
	(unspec:VF
	  [(match_operand:VF   1 "nonimmediate_operand" "%0, 0,x, x,x")
	   (match_operand:VF   2 "nonimmediate_operand" "xm, x,xm,x,m")
	   (neg:VF
	     (match_operand:VF 3 "nonimmediate_operand" " x,xm,0,xm,x"))]
	  UNSPEC_FMADDSUB))]
  "TARGET_FMA || TARGET_FMA4"
  "@
   vfmsubadd132<ssemodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfmsubadd213<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vfmsubadd231<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}
   vfmsubadd<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}
   vfmsubadd<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "fma,fma,fma,fma4,fma4")
   (set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

;; FMA3 floating point scalar intrinsics. These merge result with
;; high-order elements from the destination register.

(define_expand "fmai_vmfmadd_<mode>"
  [(set (match_operand:VF_128 0 "register_operand")
	(vec_merge:VF_128
	  (fma:VF_128
	    (match_operand:VF_128 1 "nonimmediate_operand")
	    (match_operand:VF_128 2 "nonimmediate_operand")
	    (match_operand:VF_128 3 "nonimmediate_operand"))
	  (match_dup 1)
	  (const_int 1)))]
  "TARGET_FMA")

(define_insn "*fmai_fmadd_<mode>"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
        (vec_merge:VF_128
	  (fma:VF_128
	    (match_operand:VF_128 1 "nonimmediate_operand" " 0, 0")
	    (match_operand:VF_128 2 "nonimmediate_operand" "xm, x")
	    (match_operand:VF_128 3 "nonimmediate_operand" " x,xm"))
	  (match_dup 1)
	  (const_int 1)))]
  "TARGET_FMA"
  "@
   vfmadd132<ssescalarmodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfmadd213<ssescalarmodesuffix>\t{%3, %2, %0|%0, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fmai_fmsub_<mode>"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
        (vec_merge:VF_128
	  (fma:VF_128
	    (match_operand:VF_128   1 "nonimmediate_operand" " 0, 0")
	    (match_operand:VF_128   2 "nonimmediate_operand" "xm, x")
	    (neg:VF_128
	      (match_operand:VF_128 3 "nonimmediate_operand" " x,xm")))
	  (match_dup 1)
	  (const_int 1)))]
  "TARGET_FMA"
  "@
   vfmsub132<ssescalarmodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfmsub213<ssescalarmodesuffix>\t{%3, %2, %0|%0, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fmai_fnmadd_<mode>"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
        (vec_merge:VF_128
	  (fma:VF_128
	    (neg:VF_128
	      (match_operand:VF_128 2 "nonimmediate_operand" "xm, x"))
	    (match_operand:VF_128   1 "nonimmediate_operand" " 0, 0")
	    (match_operand:VF_128   3 "nonimmediate_operand" " x,xm"))
	  (match_dup 1)
	  (const_int 1)))]
  "TARGET_FMA"
  "@
   vfnmadd132<ssescalarmodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfnmadd213<ssescalarmodesuffix>\t{%3, %2, %0|%0, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fmai_fnmsub_<mode>"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
        (vec_merge:VF_128
	  (fma:VF_128
	    (neg:VF_128
	      (match_operand:VF_128 2 "nonimmediate_operand" "xm, x"))
	    (match_operand:VF_128   1 "nonimmediate_operand" " 0, 0")
	    (neg:VF_128
	      (match_operand:VF_128 3 "nonimmediate_operand" " x,xm")))
	  (match_dup 1)
	  (const_int 1)))]
  "TARGET_FMA"
  "@
   vfnmsub132<ssescalarmodesuffix>\t{%2, %3, %0|%0, %3, %2}
   vfnmsub213<ssescalarmodesuffix>\t{%3, %2, %0|%0, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

;; FMA4 floating point scalar intrinsics.  These write the
;; entire destination register, with the high-order elements zeroed.

(define_expand "fma4i_vmfmadd_<mode>"
  [(set (match_operand:VF_128 0 "register_operand")
	(vec_merge:VF_128
	  (fma:VF_128
	    (match_operand:VF_128 1 "nonimmediate_operand")
	    (match_operand:VF_128 2 "nonimmediate_operand")
	    (match_operand:VF_128 3 "nonimmediate_operand"))
	  (match_dup 4)
	  (const_int 1)))]
  "TARGET_FMA4"
  "operands[4] = CONST0_RTX (<MODE>mode);")

(define_insn "*fma4i_vmfmadd_<mode>"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (fma:VF_128
	    (match_operand:VF_128 1 "nonimmediate_operand" "%x,x")
	    (match_operand:VF_128 2 "nonimmediate_operand" " x,m")
	    (match_operand:VF_128 3 "nonimmediate_operand" "xm,x"))
	  (match_operand:VF_128 4 "const0_operand")
	  (const_int 1)))]
  "TARGET_FMA4"
  "vfmadd<ssescalarmodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fma4i_vmfmsub_<mode>"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (fma:VF_128
	    (match_operand:VF_128 1 "nonimmediate_operand" "%x,x")
	    (match_operand:VF_128 2 "nonimmediate_operand" " x,m")
	    (neg:VF_128
	      (match_operand:VF_128 3 "nonimmediate_operand" "xm,x")))
	  (match_operand:VF_128 4 "const0_operand")
	  (const_int 1)))]
  "TARGET_FMA4"
  "vfmsub<ssescalarmodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fma4i_vmfnmadd_<mode>"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (fma:VF_128
	    (neg:VF_128
	      (match_operand:VF_128 1 "nonimmediate_operand" "%x,x"))
	    (match_operand:VF_128   2 "nonimmediate_operand" " x,m")
	    (match_operand:VF_128   3 "nonimmediate_operand" "xm,x"))
	  (match_operand:VF_128 4 "const0_operand")
	  (const_int 1)))]
  "TARGET_FMA4"
  "vfnmadd<ssescalarmodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

(define_insn "*fma4i_vmfnmsub_<mode>"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (fma:VF_128
	    (neg:VF_128
	      (match_operand:VF_128 1 "nonimmediate_operand" "%x,x"))
	    (match_operand:VF_128   2 "nonimmediate_operand" " x,m")
	    (neg:VF_128
	      (match_operand:VF_128   3 "nonimmediate_operand" "xm,x")))
	  (match_operand:VF_128 4 "const0_operand")
	  (const_int 1)))]
  "TARGET_FMA4"
  "vfnmsub<ssescalarmodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "<MODE>")])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel single-precision floating point conversion operations
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_insn "sse_cvtpi2ps"
  [(set (match_operand:V4SF 0 "register_operand" "=x")
	(vec_merge:V4SF
	  (vec_duplicate:V4SF
	    (float:V2SF (match_operand:V2SI 2 "nonimmediate_operand" "ym")))
	  (match_operand:V4SF 1 "register_operand" "0")
	  (const_int 3)))]
  "TARGET_SSE"
  "cvtpi2ps\t{%2, %0|%0, %2}"
  [(set_attr "type" "ssecvt")
   (set_attr "mode" "V4SF")])

(define_insn "sse_cvtps2pi"
  [(set (match_operand:V2SI 0 "register_operand" "=y")
	(vec_select:V2SI
	  (unspec:V4SI [(match_operand:V4SF 1 "nonimmediate_operand" "xm")]
		       UNSPEC_FIX_NOTRUNC)
	  (parallel [(const_int 0) (const_int 1)])))]
  "TARGET_SSE"
  "cvtps2pi\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "unit" "mmx")
   (set_attr "mode" "DI")])

(define_insn "sse_cvttps2pi"
  [(set (match_operand:V2SI 0 "register_operand" "=y")
	(vec_select:V2SI
	  (fix:V4SI (match_operand:V4SF 1 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0) (const_int 1)])))]
  "TARGET_SSE"
  "cvttps2pi\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "unit" "mmx")
   (set_attr "prefix_rep" "0")
   (set_attr "mode" "SF")])

(define_insn "sse_cvtsi2ss"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x,x")
	(vec_merge:V4SF
	  (vec_duplicate:V4SF
	    (float:SF (match_operand:SI 2 "nonimmediate_operand" "r,m,rm")))
	  (match_operand:V4SF 1 "register_operand" "0,0,x")
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   cvtsi2ss\t{%2, %0|%0, %2}
   cvtsi2ss\t{%2, %0|%0, %2}
   vcvtsi2ss\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,noavx,avx")
   (set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "vector,double,*")
   (set_attr "amdfam10_decode" "vector,double,*")
   (set_attr "bdver1_decode" "double,direct,*")
   (set_attr "btver2_decode" "double,double,double")
   (set_attr "prefix" "orig,orig,vex")
   (set_attr "mode" "SF")])

(define_insn "sse_cvtsi2ssq"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x,x")
	(vec_merge:V4SF
	  (vec_duplicate:V4SF
	    (float:SF (match_operand:DI 2 "nonimmediate_operand" "r,m,rm")))
	  (match_operand:V4SF 1 "register_operand" "0,0,x")
	  (const_int 1)))]
  "TARGET_SSE && TARGET_64BIT"
  "@
   cvtsi2ssq\t{%2, %0|%0, %2}
   cvtsi2ssq\t{%2, %0|%0, %2}
   vcvtsi2ssq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,noavx,avx")
   (set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "vector,double,*")
   (set_attr "amdfam10_decode" "vector,double,*")
   (set_attr "bdver1_decode" "double,direct,*")
   (set_attr "btver2_decode" "double,double,double")
   (set_attr "length_vex" "*,*,4")
   (set_attr "prefix_rex" "1,1,*")
   (set_attr "prefix" "orig,orig,vex")
   (set_attr "mode" "SF")])

(define_insn "sse_cvtss2si"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(unspec:SI
	  [(vec_select:SF
	     (match_operand:V4SF 1 "nonimmediate_operand" "x,m")
	     (parallel [(const_int 0)]))]
	  UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE"
  "%vcvtss2si\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "SI")])

(define_insn "sse_cvtss2si_2"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(unspec:SI [(match_operand:SF 1 "nonimmediate_operand" "x,m")]
		   UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE"
  "%vcvtss2si\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "amdfam10_decode" "double,double")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "SI")])

(define_insn "sse_cvtss2siq"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(unspec:DI
	  [(vec_select:SF
	     (match_operand:V4SF 1 "nonimmediate_operand" "x,m")
	     (parallel [(const_int 0)]))]
	  UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE && TARGET_64BIT"
  "%vcvtss2si{q}\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "DI")])

(define_insn "sse_cvtss2siq_2"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(unspec:DI [(match_operand:SF 1 "nonimmediate_operand" "x,m")]
		   UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE && TARGET_64BIT"
  "%vcvtss2si{q}\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "amdfam10_decode" "double,double")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "DI")])

(define_insn "sse_cvttss2si"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(fix:SI
	  (vec_select:SF
	    (match_operand:V4SF 1 "nonimmediate_operand" "x,m")
	    (parallel [(const_int 0)]))))]
  "TARGET_SSE"
  "%vcvttss2si\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "amdfam10_decode" "double,double")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "SI")])

(define_insn "sse_cvttss2siq"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(fix:DI
	  (vec_select:SF
	    (match_operand:V4SF 1 "nonimmediate_operand" "x,m")
	    (parallel [(const_int 0)]))))]
  "TARGET_SSE && TARGET_64BIT"
  "%vcvttss2si{q}\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "amdfam10_decode" "double,double")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "DI")])

(define_insn "float<sseintvecmodelower><mode>2"
  [(set (match_operand:VF1 0 "register_operand" "=x")
	(float:VF1
	  (match_operand:<sseintvecmode> 1 "nonimmediate_operand" "xm")))]
  "TARGET_SSE2"
  "%vcvtdq2ps\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_expand "floatuns<sseintvecmodelower><mode>2"
  [(match_operand:VF1 0 "register_operand")
   (match_operand:<sseintvecmode> 1 "register_operand")]
  "TARGET_SSE2 && (<MODE>mode == V4SFmode || TARGET_AVX2)"
{
  ix86_expand_vector_convert_uns_vsivsf (operands[0], operands[1]);
  DONE;
})

(define_insn "avx_cvtps2dq256"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(unspec:V8SI [(match_operand:V8SF 1 "nonimmediate_operand" "xm")]
		     UNSPEC_FIX_NOTRUNC))]
  "TARGET_AVX"
  "vcvtps2dq\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "sse2_cvtps2dq"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(unspec:V4SI [(match_operand:V4SF 1 "nonimmediate_operand" "xm")]
		     UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE2"
  "%vcvtps2dq\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set (attr "prefix_data16")
     (if_then_else
       (match_test "TARGET_AVX")
     (const_string "*")
     (const_string "1")))
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "fix_truncv8sfv8si2"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(fix:V8SI (match_operand:V8SF 1 "nonimmediate_operand" "xm")))]
  "TARGET_AVX"
  "vcvttps2dq\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "fix_truncv4sfv4si2"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(fix:V4SI (match_operand:V4SF 1 "nonimmediate_operand" "xm")))]
  "TARGET_SSE2"
  "%vcvttps2dq\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set (attr "prefix_rep")
     (if_then_else
       (match_test "TARGET_AVX")
     (const_string "*")
     (const_string "1")))
   (set (attr "prefix_data16")
     (if_then_else
       (match_test "TARGET_AVX")
     (const_string "*")
     (const_string "0")))
   (set_attr "prefix_data16" "0")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_expand "fixuns_trunc<mode><sseintvecmodelower>2"
  [(match_operand:<sseintvecmode> 0 "register_operand")
   (match_operand:VF1 1 "register_operand")]
  "TARGET_SSE2"
{
  rtx tmp[3];
  tmp[0] = ix86_expand_adjust_ufix_to_sfix_si (operands[1], &tmp[2]);
  tmp[1] = gen_reg_rtx (<sseintvecmode>mode);
  emit_insn (gen_fix_trunc<mode><sseintvecmodelower>2 (tmp[1], tmp[0]));
  emit_insn (gen_xor<sseintvecmodelower>3 (operands[0], tmp[1], tmp[2]));
  DONE;
})

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel double-precision floating point conversion operations
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_insn "sse2_cvtpi2pd"
  [(set (match_operand:V2DF 0 "register_operand" "=x,x")
	(float:V2DF (match_operand:V2SI 1 "nonimmediate_operand" "y,m")))]
  "TARGET_SSE2"
  "cvtpi2pd\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "unit" "mmx,*")
   (set_attr "prefix_data16" "1,*")
   (set_attr "mode" "V2DF")])

(define_insn "sse2_cvtpd2pi"
  [(set (match_operand:V2SI 0 "register_operand" "=y")
	(unspec:V2SI [(match_operand:V2DF 1 "nonimmediate_operand" "xm")]
		     UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE2"
  "cvtpd2pi\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "unit" "mmx")
   (set_attr "bdver1_decode" "double")
   (set_attr "btver2_decode" "direct")
   (set_attr "prefix_data16" "1")
   (set_attr "mode" "DI")])

(define_insn "sse2_cvttpd2pi"
  [(set (match_operand:V2SI 0 "register_operand" "=y")
	(fix:V2SI (match_operand:V2DF 1 "nonimmediate_operand" "xm")))]
  "TARGET_SSE2"
  "cvttpd2pi\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "unit" "mmx")
   (set_attr "bdver1_decode" "double")
   (set_attr "prefix_data16" "1")
   (set_attr "mode" "TI")])

(define_insn "sse2_cvtsi2sd"
  [(set (match_operand:V2DF 0 "register_operand" "=x,x,x")
	(vec_merge:V2DF
	  (vec_duplicate:V2DF
	    (float:DF (match_operand:SI 2 "nonimmediate_operand" "r,m,rm")))
	  (match_operand:V2DF 1 "register_operand" "0,0,x")
	  (const_int 1)))]
  "TARGET_SSE2"
  "@
   cvtsi2sd\t{%2, %0|%0, %2}
   cvtsi2sd\t{%2, %0|%0, %2}
   vcvtsi2sd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,noavx,avx")
   (set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,direct,*")
   (set_attr "amdfam10_decode" "vector,double,*")
   (set_attr "bdver1_decode" "double,direct,*")
   (set_attr "btver2_decode" "double,double,double")
   (set_attr "prefix" "orig,orig,vex")
   (set_attr "mode" "DF")])

(define_insn "sse2_cvtsi2sdq"
  [(set (match_operand:V2DF 0 "register_operand" "=x,x,x")
	(vec_merge:V2DF
	  (vec_duplicate:V2DF
	    (float:DF (match_operand:DI 2 "nonimmediate_operand" "r,m,rm")))
	  (match_operand:V2DF 1 "register_operand" "0,0,x")
	  (const_int 1)))]
  "TARGET_SSE2 && TARGET_64BIT"
  "@
   cvtsi2sdq\t{%2, %0|%0, %2}
   cvtsi2sdq\t{%2, %0|%0, %2}
   vcvtsi2sdq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,noavx,avx")
   (set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,direct,*")
   (set_attr "amdfam10_decode" "vector,double,*")
   (set_attr "bdver1_decode" "double,direct,*")
   (set_attr "length_vex" "*,*,4")
   (set_attr "prefix_rex" "1,1,*")
   (set_attr "prefix" "orig,orig,vex")
   (set_attr "mode" "DF")])

(define_insn "sse2_cvtsd2si"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(unspec:SI
	  [(vec_select:DF
	     (match_operand:V2DF 1 "nonimmediate_operand" "x,m")
	     (parallel [(const_int 0)]))]
	  UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE2"
  "%vcvtsd2si\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "btver2_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "SI")])

(define_insn "sse2_cvtsd2si_2"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(unspec:SI [(match_operand:DF 1 "nonimmediate_operand" "x,m")]
		   UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE2"
  "%vcvtsd2si\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "amdfam10_decode" "double,double")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "SI")])

(define_insn "sse2_cvtsd2siq"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(unspec:DI
	  [(vec_select:DF
	     (match_operand:V2DF 1 "nonimmediate_operand" "x,m")
	     (parallel [(const_int 0)]))]
	  UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE2 && TARGET_64BIT"
  "%vcvtsd2si{q}\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "DI")])

(define_insn "sse2_cvtsd2siq_2"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(unspec:DI [(match_operand:DF 1 "nonimmediate_operand" "x,m")]
		   UNSPEC_FIX_NOTRUNC))]
  "TARGET_SSE2 && TARGET_64BIT"
  "%vcvtsd2si{q}\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "amdfam10_decode" "double,double")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "DI")])

(define_insn "sse2_cvttsd2si"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(fix:SI
	  (vec_select:DF
	    (match_operand:V2DF 1 "nonimmediate_operand" "x,m")
	    (parallel [(const_int 0)]))))]
  "TARGET_SSE2"
  "%vcvttsd2si\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "amdfam10_decode" "double,double")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "btver2_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "SI")])

(define_insn "sse2_cvttsd2siq"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(fix:DI
	  (vec_select:DF
	    (match_operand:V2DF 1 "nonimmediate_operand" "x,m")
	    (parallel [(const_int 0)]))))]
  "TARGET_SSE2 && TARGET_64BIT"
  "%vcvttsd2si{q}\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseicvt")
   (set_attr "athlon_decode" "double,vector")
   (set_attr "amdfam10_decode" "double,double")
   (set_attr "bdver1_decode" "double,double")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "DI")])

(define_insn "floatv4siv4df2"
  [(set (match_operand:V4DF 0 "register_operand" "=x")
	(float:V4DF (match_operand:V4SI 1 "nonimmediate_operand" "xm")))]
  "TARGET_AVX"
  "vcvtdq2pd\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

(define_insn "avx_cvtdq2pd256_2"
  [(set (match_operand:V4DF 0 "register_operand" "=x")
	(float:V4DF
	  (vec_select:V4SI
	    (match_operand:V8SI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)]))))]
  "TARGET_AVX"
  "vcvtdq2pd\t{%x1, %0|%0, %x1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

(define_insn "sse2_cvtdq2pd"
  [(set (match_operand:V2DF 0 "register_operand" "=x")
	(float:V2DF
	  (vec_select:V2SI
	    (match_operand:V4SI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)]))))]
  "TARGET_SSE2"
  "%vcvtdq2pd\t{%1, %0|%0, %q1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "maybe_vex")
   (set_attr "ssememalign" "64")
   (set_attr "mode" "V2DF")])

(define_insn "avx_cvtpd2dq256"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(unspec:V4SI [(match_operand:V4DF 1 "nonimmediate_operand" "xm")]
		     UNSPEC_FIX_NOTRUNC))]
  "TARGET_AVX"
  "vcvtpd2dq{y}\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_expand "avx_cvtpd2dq256_2"
  [(set (match_operand:V8SI 0 "register_operand")
	(vec_concat:V8SI
	  (unspec:V4SI [(match_operand:V4DF 1 "nonimmediate_operand")]
		       UNSPEC_FIX_NOTRUNC)
	  (match_dup 2)))]
  "TARGET_AVX"
  "operands[2] = CONST0_RTX (V4SImode);")

(define_insn "*avx_cvtpd2dq256_2"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(vec_concat:V8SI
	  (unspec:V4SI [(match_operand:V4DF 1 "nonimmediate_operand" "xm")]
		       UNSPEC_FIX_NOTRUNC)
	  (match_operand:V4SI 2 "const0_operand")))]
  "TARGET_AVX"
  "vcvtpd2dq{y}\t{%1, %x0|%x0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "OI")])

(define_expand "sse2_cvtpd2dq"
  [(set (match_operand:V4SI 0 "register_operand")
	(vec_concat:V4SI
	  (unspec:V2SI [(match_operand:V2DF 1 "nonimmediate_operand")]
		       UNSPEC_FIX_NOTRUNC)
	  (match_dup 2)))]
  "TARGET_SSE2"
  "operands[2] = CONST0_RTX (V2SImode);")

(define_insn "*sse2_cvtpd2dq"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(vec_concat:V4SI
	  (unspec:V2SI [(match_operand:V2DF 1 "nonimmediate_operand" "xm")]
		       UNSPEC_FIX_NOTRUNC)
	  (match_operand:V2SI 2 "const0_operand")))]
  "TARGET_SSE2"
{
  if (TARGET_AVX)
    return "vcvtpd2dq{x}\t{%1, %0|%0, %1}";
  else
    return "cvtpd2dq\t{%1, %0|%0, %1}";
}
  [(set_attr "type" "ssecvt")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")
   (set_attr "amdfam10_decode" "double")
   (set_attr "athlon_decode" "vector")
   (set_attr "bdver1_decode" "double")])

(define_insn "fix_truncv4dfv4si2"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(fix:V4SI (match_operand:V4DF 1 "nonimmediate_operand" "xm")))]
  "TARGET_AVX"
  "vcvttpd2dq{y}\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_expand "avx_cvttpd2dq256_2"
  [(set (match_operand:V8SI 0 "register_operand")
	(vec_concat:V8SI
	  (fix:V4SI (match_operand:V4DF 1 "nonimmediate_operand"))
	  (match_dup 2)))]
  "TARGET_AVX"
  "operands[2] = CONST0_RTX (V4SImode);")

(define_insn "*avx_cvttpd2dq256_2"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(vec_concat:V8SI
	  (fix:V4SI (match_operand:V4DF 1 "nonimmediate_operand" "xm"))
	  (match_operand:V4SI 2 "const0_operand")))]
  "TARGET_AVX"
  "vcvttpd2dq{y}\t{%1, %x0|%x0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "OI")])

(define_expand "sse2_cvttpd2dq"
  [(set (match_operand:V4SI 0 "register_operand")
	(vec_concat:V4SI
	  (fix:V2SI (match_operand:V2DF 1 "nonimmediate_operand"))
	  (match_dup 2)))]
  "TARGET_SSE2"
  "operands[2] = CONST0_RTX (V2SImode);")

(define_insn "*sse2_cvttpd2dq"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(vec_concat:V4SI
	  (fix:V2SI (match_operand:V2DF 1 "nonimmediate_operand" "xm"))
	  (match_operand:V2SI 2 "const0_operand")))]
  "TARGET_SSE2"
{
  if (TARGET_AVX)
    return "vcvttpd2dq{x}\t{%1, %0|%0, %1}";
  else
    return "cvttpd2dq\t{%1, %0|%0, %1}";
}
  [(set_attr "type" "ssecvt")
   (set_attr "amdfam10_decode" "double")
   (set_attr "athlon_decode" "vector")
   (set_attr "bdver1_decode" "double")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "sse2_cvtsd2ss"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x,x")
	(vec_merge:V4SF
	  (vec_duplicate:V4SF
	    (float_truncate:V2SF
	      (match_operand:V2DF 2 "nonimmediate_operand" "x,m,xm")))
	  (match_operand:V4SF 1 "register_operand" "0,0,x")
	  (const_int 1)))]
  "TARGET_SSE2"
  "@
   cvtsd2ss\t{%2, %0|%0, %2}
   cvtsd2ss\t{%2, %0|%0, %2}
   vcvtsd2ss\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,noavx,avx")
   (set_attr "type" "ssecvt")
   (set_attr "athlon_decode" "vector,double,*")
   (set_attr "amdfam10_decode" "vector,double,*")
   (set_attr "bdver1_decode" "direct,direct,*")
   (set_attr "btver2_decode" "double,double,double")
   (set_attr "prefix" "orig,orig,vex")
   (set_attr "mode" "SF")])

(define_insn "sse2_cvtss2sd"
  [(set (match_operand:V2DF 0 "register_operand" "=x,x,x")
	(vec_merge:V2DF
	  (float_extend:V2DF
	    (vec_select:V2SF
	      (match_operand:V4SF 2 "nonimmediate_operand" "x,m,xm")
	      (parallel [(const_int 0) (const_int 1)])))
	  (match_operand:V2DF 1 "register_operand" "0,0,x")
	  (const_int 1)))]
  "TARGET_SSE2"
  "@
   cvtss2sd\t{%2, %0|%0, %2}
   cvtss2sd\t{%2, %0|%0, %2}
   vcvtss2sd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,noavx,avx")
   (set_attr "type" "ssecvt")
   (set_attr "amdfam10_decode" "vector,double,*")
   (set_attr "athlon_decode" "direct,direct,*")
   (set_attr "bdver1_decode" "direct,direct,*")
   (set_attr "btver2_decode" "double,double,double")
   (set_attr "prefix" "orig,orig,vex")
   (set_attr "mode" "DF")])

(define_insn "avx_cvtpd2ps256"
  [(set (match_operand:V4SF 0 "register_operand" "=x")
	(float_truncate:V4SF
	  (match_operand:V4DF 1 "nonimmediate_operand" "xm")))]
  "TARGET_AVX"
  "vcvtpd2ps{y}\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "V4SF")])

(define_expand "sse2_cvtpd2ps"
  [(set (match_operand:V4SF 0 "register_operand")
	(vec_concat:V4SF
	  (float_truncate:V2SF
	    (match_operand:V2DF 1 "nonimmediate_operand"))
	  (match_dup 2)))]
  "TARGET_SSE2"
  "operands[2] = CONST0_RTX (V2SFmode);")

(define_insn "*sse2_cvtpd2ps"
  [(set (match_operand:V4SF 0 "register_operand" "=x")
	(vec_concat:V4SF
	  (float_truncate:V2SF
	    (match_operand:V2DF 1 "nonimmediate_operand" "xm"))
	  (match_operand:V2SF 2 "const0_operand")))]
  "TARGET_SSE2"
{
  if (TARGET_AVX)
    return "vcvtpd2ps{x}\t{%1, %0|%0, %1}";
  else
    return "cvtpd2ps\t{%1, %0|%0, %1}";
}
  [(set_attr "type" "ssecvt")
   (set_attr "amdfam10_decode" "double")
   (set_attr "athlon_decode" "vector")
   (set_attr "bdver1_decode" "double")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "V4SF")])

(define_insn "avx_cvtps2pd256"
  [(set (match_operand:V4DF 0 "register_operand" "=x")
	(float_extend:V4DF
	  (match_operand:V4SF 1 "nonimmediate_operand" "xm")))]
  "TARGET_AVX"
  "vcvtps2pd\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

(define_insn "*avx_cvtps2pd256_2"
  [(set (match_operand:V4DF 0 "register_operand" "=x")
	(float_extend:V4DF
	  (vec_select:V4SF
	    (match_operand:V8SF 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)]))))]
  "TARGET_AVX"
  "vcvtps2pd\t{%x1, %0|%0, %x1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

(define_insn "sse2_cvtps2pd"
  [(set (match_operand:V2DF 0 "register_operand" "=x")
	(float_extend:V2DF
	  (vec_select:V2SF
	    (match_operand:V4SF 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)]))))]
  "TARGET_SSE2"
  "%vcvtps2pd\t{%1, %0|%0, %q1}"
  [(set_attr "type" "ssecvt")
   (set_attr "amdfam10_decode" "direct")
   (set_attr "athlon_decode" "double")
   (set_attr "bdver1_decode" "double")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "V2DF")])

(define_expand "vec_unpacks_hi_v4sf"
  [(set (match_dup 2)
   (vec_select:V4SF
     (vec_concat:V8SF
       (match_dup 2)
       (match_operand:V4SF 1 "nonimmediate_operand"))
     (parallel [(const_int 6) (const_int 7)
		(const_int 2) (const_int 3)])))
  (set (match_operand:V2DF 0 "register_operand")
   (float_extend:V2DF
     (vec_select:V2SF
       (match_dup 2)
       (parallel [(const_int 0) (const_int 1)]))))]
  "TARGET_SSE2"
  "operands[2] = gen_reg_rtx (V4SFmode);")

(define_expand "vec_unpacks_hi_v8sf"
  [(set (match_dup 2)
	(vec_select:V4SF
	  (match_operand:V8SF 1 "nonimmediate_operand")
	  (parallel [(const_int 4) (const_int 5)
		     (const_int 6) (const_int 7)])))
   (set (match_operand:V4DF 0 "register_operand")
	(float_extend:V4DF
	  (match_dup 2)))]
  "TARGET_AVX"
  "operands[2] = gen_reg_rtx (V4SFmode);")

(define_expand "vec_unpacks_lo_v4sf"
  [(set (match_operand:V2DF 0 "register_operand")
	(float_extend:V2DF
	  (vec_select:V2SF
	    (match_operand:V4SF 1 "nonimmediate_operand")
	    (parallel [(const_int 0) (const_int 1)]))))]
  "TARGET_SSE2")

(define_expand "vec_unpacks_lo_v8sf"
  [(set (match_operand:V4DF 0 "register_operand")
	(float_extend:V4DF
	  (vec_select:V4SF
	    (match_operand:V8SF 1 "nonimmediate_operand")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)]))))]
  "TARGET_AVX")

(define_mode_attr sseunpackfltmode
  [(V8HI "V4SF") (V4SI "V2DF") (V16HI "V8SF") (V8SI "V4DF")])

(define_expand "vec_unpacks_float_hi_<mode>"
  [(match_operand:<sseunpackfltmode> 0 "register_operand")
   (match_operand:VI2_AVX2 1 "register_operand")]
  "TARGET_SSE2"
{
  rtx tmp = gen_reg_rtx (<sseunpackmode>mode);

  emit_insn (gen_vec_unpacks_hi_<mode> (tmp, operands[1]));
  emit_insn (gen_rtx_SET (VOIDmode, operands[0],
			  gen_rtx_FLOAT (<sseunpackfltmode>mode, tmp)));
  DONE;
})

(define_expand "vec_unpacks_float_lo_<mode>"
  [(match_operand:<sseunpackfltmode> 0 "register_operand")
   (match_operand:VI2_AVX2 1 "register_operand")]
  "TARGET_SSE2"
{
  rtx tmp = gen_reg_rtx (<sseunpackmode>mode);

  emit_insn (gen_vec_unpacks_lo_<mode> (tmp, operands[1]));
  emit_insn (gen_rtx_SET (VOIDmode, operands[0],
			  gen_rtx_FLOAT (<sseunpackfltmode>mode, tmp)));
  DONE;
})

(define_expand "vec_unpacku_float_hi_<mode>"
  [(match_operand:<sseunpackfltmode> 0 "register_operand")
   (match_operand:VI2_AVX2 1 "register_operand")]
  "TARGET_SSE2"
{
  rtx tmp = gen_reg_rtx (<sseunpackmode>mode);

  emit_insn (gen_vec_unpacku_hi_<mode> (tmp, operands[1]));
  emit_insn (gen_rtx_SET (VOIDmode, operands[0],
			  gen_rtx_FLOAT (<sseunpackfltmode>mode, tmp)));
  DONE;
})

(define_expand "vec_unpacku_float_lo_<mode>"
  [(match_operand:<sseunpackfltmode> 0 "register_operand")
   (match_operand:VI2_AVX2 1 "register_operand")]
  "TARGET_SSE2"
{
  rtx tmp = gen_reg_rtx (<sseunpackmode>mode);

  emit_insn (gen_vec_unpacku_lo_<mode> (tmp, operands[1]));
  emit_insn (gen_rtx_SET (VOIDmode, operands[0],
			  gen_rtx_FLOAT (<sseunpackfltmode>mode, tmp)));
  DONE;
})

(define_expand "vec_unpacks_float_hi_v4si"
  [(set (match_dup 2)
	(vec_select:V4SI
	  (match_operand:V4SI 1 "nonimmediate_operand")
	  (parallel [(const_int 2) (const_int 3)
		     (const_int 2) (const_int 3)])))
   (set (match_operand:V2DF 0 "register_operand")
	(float:V2DF
	  (vec_select:V2SI
	  (match_dup 2)
	    (parallel [(const_int 0) (const_int 1)]))))]
  "TARGET_SSE2"
  "operands[2] = gen_reg_rtx (V4SImode);")

(define_expand "vec_unpacks_float_lo_v4si"
  [(set (match_operand:V2DF 0 "register_operand")
	(float:V2DF
	  (vec_select:V2SI
	    (match_operand:V4SI 1 "nonimmediate_operand")
	    (parallel [(const_int 0) (const_int 1)]))))]
  "TARGET_SSE2")

(define_expand "vec_unpacks_float_hi_v8si"
  [(set (match_dup 2)
	(vec_select:V4SI
	  (match_operand:V8SI 1 "nonimmediate_operand")
	  (parallel [(const_int 4) (const_int 5)
		     (const_int 6) (const_int 7)])))
   (set (match_operand:V4DF 0 "register_operand")
	(float:V4DF
	  (match_dup 2)))]
  "TARGET_AVX"
  "operands[2] = gen_reg_rtx (V4SImode);")

(define_expand "vec_unpacks_float_lo_v8si"
  [(set (match_operand:V4DF 0 "register_operand")
	(float:V4DF
	  (vec_select:V4SI
	    (match_operand:V8SI 1 "nonimmediate_operand")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)]))))]
  "TARGET_AVX")

(define_expand "vec_unpacku_float_hi_v4si"
  [(set (match_dup 5)
	(vec_select:V4SI
	  (match_operand:V4SI 1 "nonimmediate_operand")
	  (parallel [(const_int 2) (const_int 3)
		     (const_int 2) (const_int 3)])))
   (set (match_dup 6)
	(float:V2DF
	  (vec_select:V2SI
	  (match_dup 5)
	    (parallel [(const_int 0) (const_int 1)]))))
   (set (match_dup 7)
	(lt:V2DF (match_dup 6) (match_dup 3)))
   (set (match_dup 8)
	(and:V2DF (match_dup 7) (match_dup 4)))
   (set (match_operand:V2DF 0 "register_operand")
	(plus:V2DF (match_dup 6) (match_dup 8)))]
  "TARGET_SSE2"
{
  REAL_VALUE_TYPE TWO32r;
  rtx x;
  int i;

  real_ldexp (&TWO32r, &dconst1, 32);
  x = const_double_from_real_value (TWO32r, DFmode);

  operands[3] = force_reg (V2DFmode, CONST0_RTX (V2DFmode));
  operands[4] = force_reg (V2DFmode,
			   ix86_build_const_vector (V2DFmode, 1, x));

  operands[5] = gen_reg_rtx (V4SImode);

  for (i = 6; i < 9; i++)
    operands[i] = gen_reg_rtx (V2DFmode);
})

(define_expand "vec_unpacku_float_lo_v4si"
  [(set (match_dup 5)
	(float:V2DF
	  (vec_select:V2SI
	    (match_operand:V4SI 1 "nonimmediate_operand")
	    (parallel [(const_int 0) (const_int 1)]))))
   (set (match_dup 6)
	(lt:V2DF (match_dup 5) (match_dup 3)))
   (set (match_dup 7)
	(and:V2DF (match_dup 6) (match_dup 4)))
   (set (match_operand:V2DF 0 "register_operand")
	(plus:V2DF (match_dup 5) (match_dup 7)))]
  "TARGET_SSE2"
{
  REAL_VALUE_TYPE TWO32r;
  rtx x;
  int i;

  real_ldexp (&TWO32r, &dconst1, 32);
  x = const_double_from_real_value (TWO32r, DFmode);

  operands[3] = force_reg (V2DFmode, CONST0_RTX (V2DFmode));
  operands[4] = force_reg (V2DFmode,
			   ix86_build_const_vector (V2DFmode, 1, x));

  for (i = 5; i < 8; i++)
    operands[i] = gen_reg_rtx (V2DFmode);
})

(define_expand "vec_unpacku_float_hi_v8si"
  [(match_operand:V4DF 0 "register_operand")
   (match_operand:V8SI 1 "register_operand")]
  "TARGET_AVX"
{
  REAL_VALUE_TYPE TWO32r;
  rtx x, tmp[6];
  int i;

  real_ldexp (&TWO32r, &dconst1, 32);
  x = const_double_from_real_value (TWO32r, DFmode);

  tmp[0] = force_reg (V4DFmode, CONST0_RTX (V4DFmode));
  tmp[1] = force_reg (V4DFmode, ix86_build_const_vector (V4DFmode, 1, x));
  tmp[5] = gen_reg_rtx (V4SImode);

  for (i = 2; i < 5; i++)
    tmp[i] = gen_reg_rtx (V4DFmode);
  emit_insn (gen_vec_extract_hi_v8si (tmp[5], operands[1]));
  emit_insn (gen_floatv4siv4df2 (tmp[2], tmp[5]));
  emit_insn (gen_rtx_SET (VOIDmode, tmp[3],
			  gen_rtx_LT (V4DFmode, tmp[2], tmp[0])));
  emit_insn (gen_andv4df3 (tmp[4], tmp[3], tmp[1]));
  emit_insn (gen_addv4df3 (operands[0], tmp[2], tmp[4]));
  DONE;
})

(define_expand "vec_unpacku_float_lo_v8si"
  [(match_operand:V4DF 0 "register_operand")
   (match_operand:V8SI 1 "nonimmediate_operand")]
  "TARGET_AVX"
{
  REAL_VALUE_TYPE TWO32r;
  rtx x, tmp[5];
  int i;

  real_ldexp (&TWO32r, &dconst1, 32);
  x = const_double_from_real_value (TWO32r, DFmode);

  tmp[0] = force_reg (V4DFmode, CONST0_RTX (V4DFmode));
  tmp[1] = force_reg (V4DFmode, ix86_build_const_vector (V4DFmode, 1, x));

  for (i = 2; i < 5; i++)
    tmp[i] = gen_reg_rtx (V4DFmode);
  emit_insn (gen_avx_cvtdq2pd256_2 (tmp[2], operands[1]));
  emit_insn (gen_rtx_SET (VOIDmode, tmp[3],
			  gen_rtx_LT (V4DFmode, tmp[2], tmp[0])));
  emit_insn (gen_andv4df3 (tmp[4], tmp[3], tmp[1]));
  emit_insn (gen_addv4df3 (operands[0], tmp[2], tmp[4]));
  DONE;
})

(define_expand "vec_pack_trunc_v4df"
  [(set (match_dup 3)
	(float_truncate:V4SF
	  (match_operand:V4DF 1 "nonimmediate_operand")))
   (set (match_dup 4)
	(float_truncate:V4SF
	  (match_operand:V4DF 2 "nonimmediate_operand")))
   (set (match_operand:V8SF 0 "register_operand")
	(vec_concat:V8SF
	  (match_dup 3)
	  (match_dup 4)))]
  "TARGET_AVX"
{
  operands[3] = gen_reg_rtx (V4SFmode);
  operands[4] = gen_reg_rtx (V4SFmode);
})

(define_expand "vec_pack_trunc_v2df"
  [(match_operand:V4SF 0 "register_operand")
   (match_operand:V2DF 1 "nonimmediate_operand")
   (match_operand:V2DF 2 "nonimmediate_operand")]
  "TARGET_SSE2"
{
  rtx tmp0, tmp1;

  if (TARGET_AVX && !TARGET_PREFER_AVX128)
    {
      tmp0 = gen_reg_rtx (V4DFmode);
      tmp1 = force_reg (V2DFmode, operands[1]);

      emit_insn (gen_avx_vec_concatv4df (tmp0, tmp1, operands[2]));
      emit_insn (gen_avx_cvtpd2ps256 (operands[0], tmp0));
    }
  else
    {
      tmp0 = gen_reg_rtx (V4SFmode);
      tmp1 = gen_reg_rtx (V4SFmode);

      emit_insn (gen_sse2_cvtpd2ps (tmp0, operands[1]));
      emit_insn (gen_sse2_cvtpd2ps (tmp1, operands[2]));
      emit_insn (gen_sse_movlhps (operands[0], tmp0, tmp1));
    }
  DONE;
})

(define_expand "vec_pack_sfix_trunc_v4df"
  [(match_operand:V8SI 0 "register_operand")
   (match_operand:V4DF 1 "nonimmediate_operand")
   (match_operand:V4DF 2 "nonimmediate_operand")]
  "TARGET_AVX"
{
  rtx r1, r2;

  r1 = gen_reg_rtx (V4SImode);
  r2 = gen_reg_rtx (V4SImode);

  emit_insn (gen_fix_truncv4dfv4si2 (r1, operands[1]));
  emit_insn (gen_fix_truncv4dfv4si2 (r2, operands[2]));
  emit_insn (gen_avx_vec_concatv8si (operands[0], r1, r2));
  DONE;
})

(define_expand "vec_pack_sfix_trunc_v2df"
  [(match_operand:V4SI 0 "register_operand")
   (match_operand:V2DF 1 "nonimmediate_operand")
   (match_operand:V2DF 2 "nonimmediate_operand")]
  "TARGET_SSE2"
{
  rtx tmp0, tmp1;

  if (TARGET_AVX && !TARGET_PREFER_AVX128)
    {
      tmp0 = gen_reg_rtx (V4DFmode);
      tmp1 = force_reg (V2DFmode, operands[1]);

      emit_insn (gen_avx_vec_concatv4df (tmp0, tmp1, operands[2]));
      emit_insn (gen_fix_truncv4dfv4si2 (operands[0], tmp0));
    }
  else
    {
      tmp0 = gen_reg_rtx (V4SImode);
      tmp1 = gen_reg_rtx (V4SImode);

      emit_insn (gen_sse2_cvttpd2dq (tmp0, operands[1]));
      emit_insn (gen_sse2_cvttpd2dq (tmp1, operands[2]));
      emit_insn
       (gen_vec_interleave_lowv2di (gen_lowpart (V2DImode, operands[0]),
				    gen_lowpart (V2DImode, tmp0),
				    gen_lowpart (V2DImode, tmp1)));
    }
  DONE;
})

(define_mode_attr ssepackfltmode
  [(V4DF "V8SI") (V2DF "V4SI")])

(define_expand "vec_pack_ufix_trunc_<mode>"
  [(match_operand:<ssepackfltmode> 0 "register_operand")
   (match_operand:VF2 1 "register_operand")
   (match_operand:VF2 2 "register_operand")]
  "TARGET_SSE2"
{
  rtx tmp[7];
  tmp[0] = ix86_expand_adjust_ufix_to_sfix_si (operands[1], &tmp[2]);
  tmp[1] = ix86_expand_adjust_ufix_to_sfix_si (operands[2], &tmp[3]);
  tmp[4] = gen_reg_rtx (<ssepackfltmode>mode);
  emit_insn (gen_vec_pack_sfix_trunc_<mode> (tmp[4], tmp[0], tmp[1]));
  if (<ssepackfltmode>mode == V4SImode || TARGET_AVX2)
    {
      tmp[5] = gen_reg_rtx (<ssepackfltmode>mode);
      ix86_expand_vec_extract_even_odd (tmp[5], tmp[2], tmp[3], 0);
    }
  else
    {
      tmp[5] = gen_reg_rtx (V8SFmode);
      ix86_expand_vec_extract_even_odd (tmp[5], gen_lowpart (V8SFmode, tmp[2]),
					gen_lowpart (V8SFmode, tmp[3]), 0);
      tmp[5] = gen_lowpart (V8SImode, tmp[5]);
    }
  tmp[6] = expand_simple_binop (<ssepackfltmode>mode, XOR, tmp[4], tmp[5],
				operands[0], 0, OPTAB_DIRECT);
  if (tmp[6] != operands[0])
    emit_move_insn (operands[0], tmp[6]);
  DONE;
})

(define_expand "vec_pack_sfix_v4df"
  [(match_operand:V8SI 0 "register_operand")
   (match_operand:V4DF 1 "nonimmediate_operand")
   (match_operand:V4DF 2 "nonimmediate_operand")]
  "TARGET_AVX"
{
  rtx r1, r2;

  r1 = gen_reg_rtx (V4SImode);
  r2 = gen_reg_rtx (V4SImode);

  emit_insn (gen_avx_cvtpd2dq256 (r1, operands[1]));
  emit_insn (gen_avx_cvtpd2dq256 (r2, operands[2]));
  emit_insn (gen_avx_vec_concatv8si (operands[0], r1, r2));
  DONE;
})

(define_expand "vec_pack_sfix_v2df"
  [(match_operand:V4SI 0 "register_operand")
   (match_operand:V2DF 1 "nonimmediate_operand")
   (match_operand:V2DF 2 "nonimmediate_operand")]
  "TARGET_SSE2"
{
  rtx tmp0, tmp1;

  if (TARGET_AVX && !TARGET_PREFER_AVX128)
    {
      tmp0 = gen_reg_rtx (V4DFmode);
      tmp1 = force_reg (V2DFmode, operands[1]);

      emit_insn (gen_avx_vec_concatv4df (tmp0, tmp1, operands[2]));
      emit_insn (gen_avx_cvtpd2dq256 (operands[0], tmp0));
    }
  else
    {
      tmp0 = gen_reg_rtx (V4SImode);
      tmp1 = gen_reg_rtx (V4SImode);

      emit_insn (gen_sse2_cvtpd2dq (tmp0, operands[1]));
      emit_insn (gen_sse2_cvtpd2dq (tmp1, operands[2]));
      emit_insn
       (gen_vec_interleave_lowv2di (gen_lowpart (V2DImode, operands[0]),
				    gen_lowpart (V2DImode, tmp0),
				    gen_lowpart (V2DImode, tmp1)));
    }
  DONE;
})

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel single-precision floating point element swizzling
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_expand "sse_movhlps_exp"
  [(set (match_operand:V4SF 0 "nonimmediate_operand")
	(vec_select:V4SF
	  (vec_concat:V8SF
	    (match_operand:V4SF 1 "nonimmediate_operand")
	    (match_operand:V4SF 2 "nonimmediate_operand"))
	  (parallel [(const_int 6)
		     (const_int 7)
		     (const_int 2)
		     (const_int 3)])))]
  "TARGET_SSE"
{
  rtx dst = ix86_fixup_binary_operands (UNKNOWN, V4SFmode, operands);

  emit_insn (gen_sse_movhlps (dst, operands[1], operands[2]));

  /* Fix up the destination if needed.  */
  if (dst != operands[0])
    emit_move_insn (operands[0], dst);

  DONE;
})

(define_insn "sse_movhlps"
  [(set (match_operand:V4SF 0 "nonimmediate_operand"     "=x,x,x,x,m")
	(vec_select:V4SF
	  (vec_concat:V8SF
	    (match_operand:V4SF 1 "nonimmediate_operand" " 0,x,0,x,0")
	    (match_operand:V4SF 2 "nonimmediate_operand" " x,x,o,o,x"))
	  (parallel [(const_int 6)
		     (const_int 7)
		     (const_int 2)
		     (const_int 3)])))]
  "TARGET_SSE && !(MEM_P (operands[1]) && MEM_P (operands[2]))"
  "@
   movhlps\t{%2, %0|%0, %2}
   vmovhlps\t{%2, %1, %0|%0, %1, %2}
   movlps\t{%H2, %0|%0, %H2}
   vmovlps\t{%H2, %1, %0|%0, %1, %H2}
   %vmovhps\t{%2, %0|%0, %2}"
  [(set_attr "isa" "noavx,avx,noavx,avx,*")
   (set_attr "type" "ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix" "orig,vex,orig,vex,maybe_vex")
   (set_attr "mode" "V4SF,V4SF,V2SF,V2SF,V2SF")])

(define_expand "sse_movlhps_exp"
  [(set (match_operand:V4SF 0 "nonimmediate_operand")
	(vec_select:V4SF
	  (vec_concat:V8SF
	    (match_operand:V4SF 1 "nonimmediate_operand")
	    (match_operand:V4SF 2 "nonimmediate_operand"))
	  (parallel [(const_int 0)
		     (const_int 1)
		     (const_int 4)
		     (const_int 5)])))]
  "TARGET_SSE"
{
  rtx dst = ix86_fixup_binary_operands (UNKNOWN, V4SFmode, operands);

  emit_insn (gen_sse_movlhps (dst, operands[1], operands[2]));

  /* Fix up the destination if needed.  */
  if (dst != operands[0])
    emit_move_insn (operands[0], dst);

  DONE;
})

(define_insn "sse_movlhps"
  [(set (match_operand:V4SF 0 "nonimmediate_operand"     "=x,x,x,x,o")
	(vec_select:V4SF
	  (vec_concat:V8SF
	    (match_operand:V4SF 1 "nonimmediate_operand" " 0,x,0,x,0")
	    (match_operand:V4SF 2 "nonimmediate_operand" " x,x,m,m,x"))
	  (parallel [(const_int 0)
		     (const_int 1)
		     (const_int 4)
		     (const_int 5)])))]
  "TARGET_SSE && ix86_binary_operator_ok (UNKNOWN, V4SFmode, operands)"
  "@
   movlhps\t{%2, %0|%0, %2}
   vmovlhps\t{%2, %1, %0|%0, %1, %2}
   movhps\t{%2, %0|%0, %2}
   vmovhps\t{%2, %1, %0|%0, %1, %2}
   %vmovlps\t{%2, %H0|%H0, %2}"
  [(set_attr "isa" "noavx,avx,noavx,avx,*")
   (set_attr "type" "ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix" "orig,vex,orig,vex,maybe_vex")
   (set_attr "mode" "V4SF,V4SF,V2SF,V2SF,V2SF")])

;; Recall that the 256-bit unpck insns only shuffle within their lanes.
(define_insn "avx_unpckhps256"
  [(set (match_operand:V8SF 0 "register_operand" "=x")
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_operand:V8SF 1 "register_operand" "x")
	    (match_operand:V8SF 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 2) (const_int 10)
		     (const_int 3) (const_int 11)
		     (const_int 6) (const_int 14)
		     (const_int 7) (const_int 15)])))]
  "TARGET_AVX"
  "vunpckhps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V8SF")])

(define_expand "vec_interleave_highv8sf"
  [(set (match_dup 3)
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_operand:V8SF 1 "register_operand" "x")
	    (match_operand:V8SF 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0) (const_int 8)
		     (const_int 1) (const_int 9)
		     (const_int 4) (const_int 12)
		     (const_int 5) (const_int 13)])))
   (set (match_dup 4)
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_dup 1)
	    (match_dup 2))
	  (parallel [(const_int 2) (const_int 10)
		     (const_int 3) (const_int 11)
		     (const_int 6) (const_int 14)
		     (const_int 7) (const_int 15)])))
   (set (match_operand:V8SF 0 "register_operand")
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_dup 3)
	    (match_dup 4))
	  (parallel [(const_int 4) (const_int 5)
		     (const_int 6) (const_int 7)
		     (const_int 12) (const_int 13)
		     (const_int 14) (const_int 15)])))]
 "TARGET_AVX"
{
  operands[3] = gen_reg_rtx (V8SFmode);
  operands[4] = gen_reg_rtx (V8SFmode);
})

(define_insn "vec_interleave_highv4sf"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x")
	(vec_select:V4SF
	  (vec_concat:V8SF
	    (match_operand:V4SF 1 "register_operand" "0,x")
	    (match_operand:V4SF 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 2) (const_int 6)
		     (const_int 3) (const_int 7)])))]
  "TARGET_SSE"
  "@
   unpckhps\t{%2, %0|%0, %2}
   vunpckhps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V4SF")])

;; Recall that the 256-bit unpck insns only shuffle within their lanes.
(define_insn "avx_unpcklps256"
  [(set (match_operand:V8SF 0 "register_operand" "=x")
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_operand:V8SF 1 "register_operand" "x")
	    (match_operand:V8SF 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0) (const_int 8)
		     (const_int 1) (const_int 9)
		     (const_int 4) (const_int 12)
		     (const_int 5) (const_int 13)])))]
  "TARGET_AVX"
  "vunpcklps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V8SF")])

(define_expand "vec_interleave_lowv8sf"
  [(set (match_dup 3)
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_operand:V8SF 1 "register_operand" "x")
	    (match_operand:V8SF 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0) (const_int 8)
		     (const_int 1) (const_int 9)
		     (const_int 4) (const_int 12)
		     (const_int 5) (const_int 13)])))
   (set (match_dup 4)
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_dup 1)
	    (match_dup 2))
	  (parallel [(const_int 2) (const_int 10)
		     (const_int 3) (const_int 11)
		     (const_int 6) (const_int 14)
		     (const_int 7) (const_int 15)])))
   (set (match_operand:V8SF 0 "register_operand")
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_dup 3)
	    (match_dup 4))
	  (parallel [(const_int 0) (const_int 1)
		     (const_int 2) (const_int 3)
		     (const_int 8) (const_int 9)
		     (const_int 10) (const_int 11)])))]
 "TARGET_AVX"
{
  operands[3] = gen_reg_rtx (V8SFmode);
  operands[4] = gen_reg_rtx (V8SFmode);
})

(define_insn "vec_interleave_lowv4sf"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x")
	(vec_select:V4SF
	  (vec_concat:V8SF
	    (match_operand:V4SF 1 "register_operand" "0,x")
	    (match_operand:V4SF 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 0) (const_int 4)
		     (const_int 1) (const_int 5)])))]
  "TARGET_SSE"
  "@
   unpcklps\t{%2, %0|%0, %2}
   vunpcklps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V4SF")])

;; These are modeled with the same vec_concat as the others so that we
;; capture users of shufps that can use the new instructions
(define_insn "avx_movshdup256"
  [(set (match_operand:V8SF 0 "register_operand" "=x")
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_operand:V8SF 1 "nonimmediate_operand" "xm")
	    (match_dup 1))
	  (parallel [(const_int 1) (const_int 1)
		     (const_int 3) (const_int 3)
		     (const_int 5) (const_int 5)
		     (const_int 7) (const_int 7)])))]
  "TARGET_AVX"
  "vmovshdup\t{%1, %0|%0, %1}"
  [(set_attr "type" "sse")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V8SF")])

(define_insn "sse3_movshdup"
  [(set (match_operand:V4SF 0 "register_operand" "=x")
	(vec_select:V4SF
	  (vec_concat:V8SF
	    (match_operand:V4SF 1 "nonimmediate_operand" "xm")
	    (match_dup 1))
	  (parallel [(const_int 1)
		     (const_int 1)
		     (const_int 7)
		     (const_int 7)])))]
  "TARGET_SSE3"
  "%vmovshdup\t{%1, %0|%0, %1}"
  [(set_attr "type" "sse")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "V4SF")])

(define_insn "avx_movsldup256"
  [(set (match_operand:V8SF 0 "register_operand" "=x")
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_operand:V8SF 1 "nonimmediate_operand" "xm")
	    (match_dup 1))
	  (parallel [(const_int 0) (const_int 0)
		     (const_int 2) (const_int 2)
		     (const_int 4) (const_int 4)
		     (const_int 6) (const_int 6)])))]
  "TARGET_AVX"
  "vmovsldup\t{%1, %0|%0, %1}"
  [(set_attr "type" "sse")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V8SF")])

(define_insn "sse3_movsldup"
  [(set (match_operand:V4SF 0 "register_operand" "=x")
	(vec_select:V4SF
	  (vec_concat:V8SF
	    (match_operand:V4SF 1 "nonimmediate_operand" "xm")
	    (match_dup 1))
	  (parallel [(const_int 0)
		     (const_int 0)
		     (const_int 6)
		     (const_int 6)])))]
  "TARGET_SSE3"
  "%vmovsldup\t{%1, %0|%0, %1}"
  [(set_attr "type" "sse")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "V4SF")])

(define_expand "avx_shufps256"
  [(match_operand:V8SF 0 "register_operand")
   (match_operand:V8SF 1 "register_operand")
   (match_operand:V8SF 2 "nonimmediate_operand")
   (match_operand:SI 3 "const_int_operand")]
  "TARGET_AVX"
{
  int mask = INTVAL (operands[3]);
  emit_insn (gen_avx_shufps256_1 (operands[0], operands[1], operands[2],
				  GEN_INT ((mask >> 0) & 3),
				  GEN_INT ((mask >> 2) & 3),
				  GEN_INT (((mask >> 4) & 3) + 8),
				  GEN_INT (((mask >> 6) & 3) + 8),
				  GEN_INT (((mask >> 0) & 3) + 4),
				  GEN_INT (((mask >> 2) & 3) + 4),
				  GEN_INT (((mask >> 4) & 3) + 12),
				  GEN_INT (((mask >> 6) & 3) + 12)));
  DONE;
})

;; One bit in mask selects 2 elements.
(define_insn "avx_shufps256_1"
  [(set (match_operand:V8SF 0 "register_operand" "=x")
	(vec_select:V8SF
	  (vec_concat:V16SF
	    (match_operand:V8SF 1 "register_operand" "x")
	    (match_operand:V8SF 2 "nonimmediate_operand" "xm"))
	  (parallel [(match_operand 3  "const_0_to_3_operand"  )
		     (match_operand 4  "const_0_to_3_operand"  )
		     (match_operand 5  "const_8_to_11_operand" )
		     (match_operand 6  "const_8_to_11_operand" )
		     (match_operand 7  "const_4_to_7_operand"  )
		     (match_operand 8  "const_4_to_7_operand"  )
		     (match_operand 9  "const_12_to_15_operand")
		     (match_operand 10 "const_12_to_15_operand")])))]
  "TARGET_AVX
   && (INTVAL (operands[3]) == (INTVAL (operands[7]) - 4)
       && INTVAL (operands[4]) == (INTVAL (operands[8]) - 4)
       && INTVAL (operands[5]) == (INTVAL (operands[9]) - 4)
       && INTVAL (operands[6]) == (INTVAL (operands[10]) - 4))"
{
  int mask;
  mask = INTVAL (operands[3]);
  mask |= INTVAL (operands[4]) << 2;
  mask |= (INTVAL (operands[5]) - 8) << 4;
  mask |= (INTVAL (operands[6]) - 8) << 6;
  operands[3] = GEN_INT (mask);

  return "vshufps\t{%3, %2, %1, %0|%0, %1, %2, %3}";
}
  [(set_attr "type" "sseshuf")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V8SF")])

(define_expand "sse_shufps"
  [(match_operand:V4SF 0 "register_operand")
   (match_operand:V4SF 1 "register_operand")
   (match_operand:V4SF 2 "nonimmediate_operand")
   (match_operand:SI 3 "const_int_operand")]
  "TARGET_SSE"
{
  int mask = INTVAL (operands[3]);
  emit_insn (gen_sse_shufps_v4sf (operands[0], operands[1], operands[2],
			       GEN_INT ((mask >> 0) & 3),
			       GEN_INT ((mask >> 2) & 3),
			       GEN_INT (((mask >> 4) & 3) + 4),
			       GEN_INT (((mask >> 6) & 3) + 4)));
  DONE;
})

(define_insn "sse_shufps_<mode>"
  [(set (match_operand:VI4F_128 0 "register_operand" "=x,x")
	(vec_select:VI4F_128
	  (vec_concat:<ssedoublevecmode>
	    (match_operand:VI4F_128 1 "register_operand" "0,x")
	    (match_operand:VI4F_128 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(match_operand 3 "const_0_to_3_operand")
		     (match_operand 4 "const_0_to_3_operand")
		     (match_operand 5 "const_4_to_7_operand")
		     (match_operand 6 "const_4_to_7_operand")])))]
  "TARGET_SSE"
{
  int mask = 0;
  mask |= INTVAL (operands[3]) << 0;
  mask |= INTVAL (operands[4]) << 2;
  mask |= (INTVAL (operands[5]) - 4) << 4;
  mask |= (INTVAL (operands[6]) - 4) << 6;
  operands[3] = GEN_INT (mask);

  switch (which_alternative)
    {
    case 0:
      return "shufps\t{%3, %2, %0|%0, %2, %3}";
    case 1:
      return "vshufps\t{%3, %2, %1, %0|%0, %1, %2, %3}";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseshuf")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V4SF")])

(define_insn "sse_storehps"
  [(set (match_operand:V2SF 0 "nonimmediate_operand" "=m,x,x")
	(vec_select:V2SF
	  (match_operand:V4SF 1 "nonimmediate_operand" "x,x,o")
	  (parallel [(const_int 2) (const_int 3)])))]
  "TARGET_SSE"
  "@
   %vmovhps\t{%1, %0|%0, %1}
   %vmovhlps\t{%1, %d0|%d0, %1}
   %vmovlps\t{%H1, %d0|%d0, %H1}"
  [(set_attr "type" "ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "V2SF,V4SF,V2SF")])

(define_expand "sse_loadhps_exp"
  [(set (match_operand:V4SF 0 "nonimmediate_operand")
	(vec_concat:V4SF
	  (vec_select:V2SF
	    (match_operand:V4SF 1 "nonimmediate_operand")
	    (parallel [(const_int 0) (const_int 1)]))
	  (match_operand:V2SF 2 "nonimmediate_operand")))]
  "TARGET_SSE"
{
  rtx dst = ix86_fixup_binary_operands (UNKNOWN, V4SFmode, operands);

  emit_insn (gen_sse_loadhps (dst, operands[1], operands[2]));

  /* Fix up the destination if needed.  */
  if (dst != operands[0])
    emit_move_insn (operands[0], dst);

  DONE;
})

(define_insn "sse_loadhps"
  [(set (match_operand:V4SF 0 "nonimmediate_operand"     "=x,x,x,x,o")
	(vec_concat:V4SF
	  (vec_select:V2SF
	    (match_operand:V4SF 1 "nonimmediate_operand" " 0,x,0,x,0")
	    (parallel [(const_int 0) (const_int 1)]))
	  (match_operand:V2SF 2 "nonimmediate_operand"   " m,m,x,x,x")))]
  "TARGET_SSE"
  "@
   movhps\t{%2, %0|%0, %2}
   vmovhps\t{%2, %1, %0|%0, %1, %2}
   movlhps\t{%2, %0|%0, %2}
   vmovlhps\t{%2, %1, %0|%0, %1, %2}
   %vmovlps\t{%2, %H0|%H0, %2}"
  [(set_attr "isa" "noavx,avx,noavx,avx,*")
   (set_attr "type" "ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix" "orig,vex,orig,vex,maybe_vex")
   (set_attr "mode" "V2SF,V2SF,V4SF,V4SF,V2SF")])

(define_insn "sse_storelps"
  [(set (match_operand:V2SF 0 "nonimmediate_operand"   "=m,x,x")
	(vec_select:V2SF
	  (match_operand:V4SF 1 "nonimmediate_operand" " x,x,m")
	  (parallel [(const_int 0) (const_int 1)])))]
  "TARGET_SSE"
  "@
   %vmovlps\t{%1, %0|%0, %1}
   %vmovaps\t{%1, %0|%0, %1}
   %vmovlps\t{%1, %d0|%d0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "V2SF,V4SF,V2SF")])

(define_expand "sse_loadlps_exp"
  [(set (match_operand:V4SF 0 "nonimmediate_operand")
	(vec_concat:V4SF
	  (match_operand:V2SF 2 "nonimmediate_operand")
	  (vec_select:V2SF
	    (match_operand:V4SF 1 "nonimmediate_operand")
	    (parallel [(const_int 2) (const_int 3)]))))]
  "TARGET_SSE"
{
  rtx dst = ix86_fixup_binary_operands (UNKNOWN, V4SFmode, operands);

  emit_insn (gen_sse_loadlps (dst, operands[1], operands[2]));

  /* Fix up the destination if needed.  */
  if (dst != operands[0])
    emit_move_insn (operands[0], dst);

  DONE;
})

(define_insn "sse_loadlps"
  [(set (match_operand:V4SF 0 "nonimmediate_operand"     "=x,x,x,x,m")
	(vec_concat:V4SF
	  (match_operand:V2SF 2 "nonimmediate_operand"   " 0,x,m,m,x")
	  (vec_select:V2SF
	    (match_operand:V4SF 1 "nonimmediate_operand" " x,x,0,x,0")
	    (parallel [(const_int 2) (const_int 3)]))))]
  "TARGET_SSE"
  "@
   shufps\t{$0xe4, %1, %0|%0, %1, 0xe4}
   vshufps\t{$0xe4, %1, %2, %0|%0, %2, %1, 0xe4}
   movlps\t{%2, %0|%0, %2}
   vmovlps\t{%2, %1, %0|%0, %1, %2}
   %vmovlps\t{%2, %0|%0, %2}"
  [(set_attr "isa" "noavx,avx,noavx,avx,*")
   (set_attr "type" "sseshuf,sseshuf,ssemov,ssemov,ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "length_immediate" "1,1,*,*,*")
   (set_attr "prefix" "orig,vex,orig,vex,maybe_vex")
   (set_attr "mode" "V4SF,V4SF,V2SF,V2SF,V2SF")])

(define_insn "sse_movss"
  [(set (match_operand:V4SF 0 "register_operand"   "=x,x")
	(vec_merge:V4SF
	  (match_operand:V4SF 2 "register_operand" " x,x")
	  (match_operand:V4SF 1 "register_operand" " 0,x")
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   movss\t{%2, %0|%0, %2}
   vmovss\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssemov")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "SF")])

(define_insn "avx2_vec_dup<mode>"
  [(set (match_operand:VF1 0 "register_operand" "=x")
	(vec_duplicate:VF1
	  (vec_select:SF
	    (match_operand:V4SF 1 "register_operand" "x")
	    (parallel [(const_int 0)]))))]
  "TARGET_AVX2"
  "vbroadcastss\t{%1, %0|%0, %1}"
  [(set_attr "type" "sselog1")
    (set_attr "prefix" "vex")
    (set_attr "mode" "<MODE>")])

(define_insn "avx2_vec_dupv8sf_1"
  [(set (match_operand:V8SF 0 "register_operand" "=x")
	(vec_duplicate:V8SF
	  (vec_select:SF
	    (match_operand:V8SF 1 "register_operand" "x")
	    (parallel [(const_int 0)]))))]
  "TARGET_AVX2"
  "vbroadcastss\t{%x1, %0|%0, %x1}"
  [(set_attr "type" "sselog1")
    (set_attr "prefix" "vex")
    (set_attr "mode" "V8SF")])

(define_insn "vec_dupv4sf"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x,x")
	(vec_duplicate:V4SF
	  (match_operand:SF 1 "nonimmediate_operand" "x,m,0")))]
  "TARGET_SSE"
  "@
   vshufps\t{$0, %1, %1, %0|%0, %1, %1, 0}
   vbroadcastss\t{%1, %0|%0, %1}
   shufps\t{$0, %0, %0|%0, %0, 0}"
  [(set_attr "isa" "avx,avx,noavx")
   (set_attr "type" "sseshuf1,ssemov,sseshuf1")
   (set_attr "length_immediate" "1,0,1")
   (set_attr "prefix_extra" "0,1,*")
   (set_attr "prefix" "vex,vex,orig")
   (set_attr "mode" "V4SF")])

;; Although insertps takes register source, we prefer
;; unpcklps with register source since it is shorter.
(define_insn "*vec_concatv2sf_sse4_1"
  [(set (match_operand:V2SF 0 "register_operand"     "=x,x,x,x,x,*y ,*y")
	(vec_concat:V2SF
	  (match_operand:SF 1 "nonimmediate_operand" " 0,x,0,x,m, 0 , m")
	  (match_operand:SF 2 "vector_move_operand"  " x,x,m,m,C,*ym, C")))]
  "TARGET_SSE4_1"
  "@
   unpcklps\t{%2, %0|%0, %2}
   vunpcklps\t{%2, %1, %0|%0, %1, %2}
   insertps\t{$0x10, %2, %0|%0, %2, 0x10}
   vinsertps\t{$0x10, %2, %1, %0|%0, %1, %2, 0x10}
   %vmovss\t{%1, %0|%0, %1}
   punpckldq\t{%2, %0|%0, %2}
   movd\t{%1, %0|%0, %1}"
  [(set_attr "isa" "noavx,avx,noavx,avx,*,*,*")
   (set_attr "type" "sselog,sselog,sselog,sselog,ssemov,mmxcvt,mmxmov")
   (set_attr "prefix_data16" "*,*,1,*,*,*,*")
   (set_attr "prefix_extra" "*,*,1,1,*,*,*")
   (set_attr "length_immediate" "*,*,1,1,*,*,*")
   (set_attr "prefix" "orig,vex,orig,vex,maybe_vex,orig,orig")
   (set_attr "mode" "V4SF,V4SF,V4SF,V4SF,SF,DI,DI")])

;; ??? In theory we can match memory for the MMX alternative, but allowing
;; nonimmediate_operand for operand 2 and *not* allowing memory for the SSE
;; alternatives pretty much forces the MMX alternative to be chosen.
(define_insn "*vec_concatv2sf_sse"
  [(set (match_operand:V2SF 0 "register_operand"     "=x,x,*y,*y")
	(vec_concat:V2SF
	  (match_operand:SF 1 "nonimmediate_operand" " 0,m, 0, m")
	  (match_operand:SF 2 "reg_or_0_operand"     " x,C,*y, C")))]
  "TARGET_SSE"
  "@
   unpcklps\t{%2, %0|%0, %2}
   movss\t{%1, %0|%0, %1}
   punpckldq\t{%2, %0|%0, %2}
   movd\t{%1, %0|%0, %1}"
  [(set_attr "type" "sselog,ssemov,mmxcvt,mmxmov")
   (set_attr "mode" "V4SF,SF,DI,DI")])

(define_insn "*vec_concatv4sf"
  [(set (match_operand:V4SF 0 "register_operand"       "=x,x,x,x")
	(vec_concat:V4SF
	  (match_operand:V2SF 1 "register_operand"     " 0,x,0,x")
	  (match_operand:V2SF 2 "nonimmediate_operand" " x,x,m,m")))]
  "TARGET_SSE"
  "@
   movlhps\t{%2, %0|%0, %2}
   vmovlhps\t{%2, %1, %0|%0, %1, %2}
   movhps\t{%2, %0|%0, %2}
   vmovhps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx,noavx,avx")
   (set_attr "type" "ssemov")
   (set_attr "prefix" "orig,vex,orig,vex")
   (set_attr "mode" "V4SF,V4SF,V2SF,V2SF")])

(define_expand "vec_init<mode>"
  [(match_operand:V_128 0 "register_operand")
   (match_operand 1)]
  "TARGET_SSE"
{
  ix86_expand_vector_init (false, operands[0], operands[1]);
  DONE;
})

;; Avoid combining registers from different units in a single alternative,
;; see comment above inline_secondary_memory_needed function in i386.c
(define_insn "vec_set<mode>_0"
  [(set (match_operand:VI4F_128 0 "nonimmediate_operand"
	  "=x,x,x ,x,x,x,x  ,x  ,m ,m   ,m")
	(vec_merge:VI4F_128
	  (vec_duplicate:VI4F_128
	    (match_operand:<ssescalarmode> 2 "general_operand"
	  " x,m,*r,m,x,x,*rm,*rm,!x,!*re,!*fF"))
	  (match_operand:VI4F_128 1 "vector_move_operand"
	  " C,C,C ,C,0,x,0  ,x  ,0 ,0   ,0")
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   %vinsertps\t{$0xe, %d2, %0|%0, %d2, 0xe}
   %vmov<ssescalarmodesuffix>\t{%2, %0|%0, %2}
   %vmovd\t{%2, %0|%0, %2}
   movss\t{%2, %0|%0, %2}
   movss\t{%2, %0|%0, %2}
   vmovss\t{%2, %1, %0|%0, %1, %2}
   pinsrd\t{$0, %2, %0|%0, %2, 0}
   vpinsrd\t{$0, %2, %1, %0|%0, %1, %2, 0}
   #
   #
   #"
  [(set_attr "isa" "sse4,sse2,sse2,noavx,noavx,avx,sse4_noavx,avx,*,*,*")
   (set (attr "type")
     (cond [(eq_attr "alternative" "0,6,7")
	      (const_string "sselog")
	    (eq_attr "alternative" "9")
	      (const_string "imov")
	    (eq_attr "alternative" "10")
	      (const_string "fmov")
	   ]
	   (const_string "ssemov")))
   (set_attr "prefix_extra" "*,*,*,*,*,*,1,1,*,*,*")
   (set_attr "length_immediate" "*,*,*,*,*,*,1,1,*,*,*")
   (set_attr "prefix" "maybe_vex,maybe_vex,maybe_vex,orig,orig,vex,orig,vex,*,*,*")
   (set_attr "mode" "SF,<ssescalarmode>,SI,SF,SF,SF,TI,TI,*,*,*")])

;; A subset is vec_setv4sf.
(define_insn "*vec_setv4sf_sse4_1"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x")
	(vec_merge:V4SF
	  (vec_duplicate:V4SF
	    (match_operand:SF 2 "nonimmediate_operand" "xm,xm"))
	  (match_operand:V4SF 1 "register_operand" "0,x")
	  (match_operand:SI 3 "const_int_operand")))]
  "TARGET_SSE4_1
   && ((unsigned) exact_log2 (INTVAL (operands[3]))
       < GET_MODE_NUNITS (V4SFmode))"
{
  operands[3] = GEN_INT (exact_log2 (INTVAL (operands[3])) << 4);
  switch (which_alternative)
    {
    case 0:
      return "insertps\t{%3, %2, %0|%0, %2, %3}";
    case 1:
      return "vinsertps\t{%3, %2, %1, %0|%0, %1, %2, %3}";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V4SF")])

(define_insn "sse4_1_insertps"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x")
	(unspec:V4SF [(match_operand:V4SF 2 "nonimmediate_operand" "xm,xm")
		      (match_operand:V4SF 1 "register_operand" "0,x")
		      (match_operand:SI 3 "const_0_to_255_operand" "n,n")]
		     UNSPEC_INSERTPS))]
  "TARGET_SSE4_1"
{
  if (MEM_P (operands[2]))
    {
      unsigned count_s = INTVAL (operands[3]) >> 6;
      if (count_s)
	operands[3] = GEN_INT (INTVAL (operands[3]) & 0x3f);
      operands[2] = adjust_address_nv (operands[2], SFmode, count_s * 4);
    }
  switch (which_alternative)
    {
    case 0:
      return "insertps\t{%3, %2, %0|%0, %2, %3}";
    case 1:
      return "vinsertps\t{%3, %2, %1, %0|%0, %1, %2, %3}";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V4SF")])

(define_split
  [(set (match_operand:VI4F_128 0 "memory_operand")
	(vec_merge:VI4F_128
	  (vec_duplicate:VI4F_128
	    (match_operand:<ssescalarmode> 1 "nonmemory_operand"))
	  (match_dup 0)
	  (const_int 1)))]
  "TARGET_SSE && reload_completed"
  [(const_int 0)]
{
  emit_move_insn (adjust_address (operands[0], <ssescalarmode>mode, 0),
		  operands[1]);
  DONE;
})

(define_expand "vec_set<mode>"
  [(match_operand:V 0 "register_operand")
   (match_operand:<ssescalarmode> 1 "register_operand")
   (match_operand 2 "const_int_operand")]
  "TARGET_SSE"
{
  ix86_expand_vector_set (false, operands[0], operands[1],
			  INTVAL (operands[2]));
  DONE;
})

(define_insn_and_split "*vec_extractv4sf_0"
  [(set (match_operand:SF 0 "nonimmediate_operand" "=x,m,f,r")
	(vec_select:SF
	  (match_operand:V4SF 1 "nonimmediate_operand" "xm,x,m,m")
	  (parallel [(const_int 0)])))]
  "TARGET_SSE && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
  rtx op1 = operands[1];
  if (REG_P (op1))
    op1 = gen_rtx_REG (SFmode, REGNO (op1));
  else
    op1 = gen_lowpart (SFmode, op1);
  emit_move_insn (operands[0], op1);
  DONE;
})

(define_insn_and_split "*sse4_1_extractps"
  [(set (match_operand:SF 0 "nonimmediate_operand" "=rm,x,x")
	(vec_select:SF
	  (match_operand:V4SF 1 "register_operand" "x,0,x")
	  (parallel [(match_operand:SI 2 "const_0_to_3_operand" "n,n,n")])))]
  "TARGET_SSE4_1"
  "@
   %vextractps\t{%2, %1, %0|%0, %1, %2}
   #
   #"
  "&& reload_completed && SSE_REG_P (operands[0])"
  [(const_int 0)]
{
  rtx dest = gen_rtx_REG (V4SFmode, REGNO (operands[0]));
  switch (INTVAL (operands[2]))
    {
    case 1:
    case 3:
      emit_insn (gen_sse_shufps_v4sf (dest, operands[1], operands[1],
				      operands[2], operands[2],
				      GEN_INT (INTVAL (operands[2]) + 4),
				      GEN_INT (INTVAL (operands[2]) + 4)));
      break;
    case 2:
      emit_insn (gen_vec_interleave_highv4sf (dest, operands[1], operands[1]));
      break;
    default:
      /* 0 should be handled by the *vec_extractv4sf_0 pattern above.  */
      gcc_unreachable ();
    }
  DONE;
}
  [(set_attr "isa" "*,noavx,avx")
   (set_attr "type" "sselog,*,*")
   (set_attr "prefix_data16" "1,*,*")
   (set_attr "prefix_extra" "1,*,*")
   (set_attr "length_immediate" "1,*,*")
   (set_attr "prefix" "maybe_vex,*,*")
   (set_attr "mode" "V4SF,*,*")])

(define_insn_and_split "*vec_extract_v4sf_mem"
  [(set (match_operand:SF 0 "register_operand" "=x,*r,f")
       (vec_select:SF
	 (match_operand:V4SF 1 "memory_operand" "o,o,o")
	 (parallel [(match_operand 2 "const_0_to_3_operand" "n,n,n")])))]
  "TARGET_SSE"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
  int i = INTVAL (operands[2]);

  emit_move_insn (operands[0], adjust_address (operands[1], SFmode, i*4));
  DONE;
})

(define_expand "avx_vextractf128<mode>"
  [(match_operand:<ssehalfvecmode> 0 "nonimmediate_operand")
   (match_operand:V_256 1 "register_operand")
   (match_operand:SI 2 "const_0_to_1_operand")]
  "TARGET_AVX"
{
  rtx (*insn)(rtx, rtx);

  switch (INTVAL (operands[2]))
    {
    case 0:
      insn = gen_vec_extract_lo_<mode>;
      break;
    case 1:
      insn = gen_vec_extract_hi_<mode>;
      break;
    default:
      gcc_unreachable ();
    }

  emit_insn (insn (operands[0], operands[1]));
  DONE;
})

(define_insn_and_split "vec_extract_lo_<mode>"
  [(set (match_operand:<ssehalfvecmode> 0 "nonimmediate_operand" "=x,m")
	(vec_select:<ssehalfvecmode>
	  (match_operand:VI8F_256 1 "nonimmediate_operand" "xm,x")
	  (parallel [(const_int 0) (const_int 1)])))]
  "TARGET_AVX && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
  rtx op1 = operands[1];
  if (REG_P (op1))
    op1 = gen_rtx_REG (<ssehalfvecmode>mode, REGNO (op1));
  else
    op1 = gen_lowpart (<ssehalfvecmode>mode, op1);
  emit_move_insn (operands[0], op1);
  DONE;
})

(define_insn "vec_extract_hi_<mode>"
  [(set (match_operand:<ssehalfvecmode> 0 "nonimmediate_operand" "=x,m")
	(vec_select:<ssehalfvecmode>
	  (match_operand:VI8F_256 1 "register_operand" "x,x")
	  (parallel [(const_int 2) (const_int 3)])))]
  "TARGET_AVX"
  "vextract<i128>\t{$0x1, %1, %0|%0, %1, 0x1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "none,store")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn_and_split "vec_extract_lo_<mode>"
  [(set (match_operand:<ssehalfvecmode> 0 "nonimmediate_operand" "=x,m")
	(vec_select:<ssehalfvecmode>
	  (match_operand:VI4F_256 1 "nonimmediate_operand" "xm,x")
	  (parallel [(const_int 0) (const_int 1)
		     (const_int 2) (const_int 3)])))]
  "TARGET_AVX && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
  rtx op1 = operands[1];
  if (REG_P (op1))
    op1 = gen_rtx_REG (<ssehalfvecmode>mode, REGNO (op1));
  else
    op1 = gen_lowpart (<ssehalfvecmode>mode, op1);
  emit_move_insn (operands[0], op1);
  DONE;
})

(define_insn "vec_extract_hi_<mode>"
  [(set (match_operand:<ssehalfvecmode> 0 "nonimmediate_operand" "=x,m")
	(vec_select:<ssehalfvecmode>
	  (match_operand:VI4F_256 1 "register_operand" "x,x")
	  (parallel [(const_int 4) (const_int 5)
		     (const_int 6) (const_int 7)])))]
  "TARGET_AVX"
  "vextract<i128>\t{$0x1, %1, %0|%0, %1, 0x1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "none,store")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn_and_split "vec_extract_lo_v16hi"
  [(set (match_operand:V8HI 0 "nonimmediate_operand" "=x,m")
	(vec_select:V8HI
	  (match_operand:V16HI 1 "nonimmediate_operand" "xm,x")
	  (parallel [(const_int 0) (const_int 1)
		     (const_int 2) (const_int 3)
		     (const_int 4) (const_int 5)
		     (const_int 6) (const_int 7)])))]
  "TARGET_AVX && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
  rtx op1 = operands[1];
  if (REG_P (op1))
    op1 = gen_rtx_REG (V8HImode, REGNO (op1));
  else
    op1 = gen_lowpart (V8HImode, op1);
  emit_move_insn (operands[0], op1);
  DONE;
})

(define_insn "vec_extract_hi_v16hi"
  [(set (match_operand:V8HI 0 "nonimmediate_operand" "=x,m")
	(vec_select:V8HI
	  (match_operand:V16HI 1 "register_operand" "x,x")
	  (parallel [(const_int 8) (const_int 9)
		     (const_int 10) (const_int 11)
		     (const_int 12) (const_int 13)
		     (const_int 14) (const_int 15)])))]
  "TARGET_AVX"
  "vextract%~128\t{$0x1, %1, %0|%0, %1, 0x1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "none,store")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn_and_split "vec_extract_lo_v32qi"
  [(set (match_operand:V16QI 0 "nonimmediate_operand" "=x,m")
	(vec_select:V16QI
	  (match_operand:V32QI 1 "nonimmediate_operand" "xm,x")
	  (parallel [(const_int 0) (const_int 1)
		     (const_int 2) (const_int 3)
		     (const_int 4) (const_int 5)
		     (const_int 6) (const_int 7)
		     (const_int 8) (const_int 9)
		     (const_int 10) (const_int 11)
		     (const_int 12) (const_int 13)
		     (const_int 14) (const_int 15)])))]
  "TARGET_AVX && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
  rtx op1 = operands[1];
  if (REG_P (op1))
    op1 = gen_rtx_REG (V16QImode, REGNO (op1));
  else
    op1 = gen_lowpart (V16QImode, op1);
  emit_move_insn (operands[0], op1);
  DONE;
})

(define_insn "vec_extract_hi_v32qi"
  [(set (match_operand:V16QI 0 "nonimmediate_operand" "=x,m")
	(vec_select:V16QI
	  (match_operand:V32QI 1 "register_operand" "x,x")
	  (parallel [(const_int 16) (const_int 17)
		     (const_int 18) (const_int 19)
		     (const_int 20) (const_int 21)
		     (const_int 22) (const_int 23)
		     (const_int 24) (const_int 25)
		     (const_int 26) (const_int 27)
		     (const_int 28) (const_int 29)
		     (const_int 30) (const_int 31)])))]
  "TARGET_AVX"
  "vextract%~128\t{$0x1, %1, %0|%0, %1, 0x1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "none,store")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

;; Modes handled by vec_extract patterns.
(define_mode_iterator VEC_EXTRACT_MODE
  [(V32QI "TARGET_AVX") V16QI
   (V16HI "TARGET_AVX") V8HI
   (V8SI "TARGET_AVX") V4SI
   (V4DI "TARGET_AVX") V2DI
   (V8SF "TARGET_AVX") V4SF
   (V4DF "TARGET_AVX") V2DF])

(define_expand "vec_extract<mode>"
  [(match_operand:<ssescalarmode> 0 "register_operand")
   (match_operand:VEC_EXTRACT_MODE 1 "register_operand")
   (match_operand 2 "const_int_operand")]
  "TARGET_SSE"
{
  ix86_expand_vector_extract (false, operands[0], operands[1],
			      INTVAL (operands[2]));
  DONE;
})

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel double-precision floating point element swizzling
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Recall that the 256-bit unpck insns only shuffle within their lanes.
(define_insn "avx_unpckhpd256"
  [(set (match_operand:V4DF 0 "register_operand" "=x")
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_operand:V4DF 1 "register_operand" "x")
	    (match_operand:V4DF 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 1) (const_int 5)
		     (const_int 3) (const_int 7)])))]
  "TARGET_AVX"
  "vunpckhpd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

(define_expand "vec_interleave_highv4df"
  [(set (match_dup 3)
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_operand:V4DF 1 "register_operand" "x")
	    (match_operand:V4DF 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0) (const_int 4)
		     (const_int 2) (const_int 6)])))
   (set (match_dup 4)
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_dup 1)
	    (match_dup 2))
	  (parallel [(const_int 1) (const_int 5)
		     (const_int 3) (const_int 7)])))
   (set (match_operand:V4DF 0 "register_operand")
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_dup 3)
	    (match_dup 4))
	  (parallel [(const_int 2) (const_int 3)
		     (const_int 6) (const_int 7)])))]
 "TARGET_AVX"
{
  operands[3] = gen_reg_rtx (V4DFmode);
  operands[4] = gen_reg_rtx (V4DFmode);
})


(define_expand "vec_interleave_highv2df"
  [(set (match_operand:V2DF 0 "register_operand")
	(vec_select:V2DF
	  (vec_concat:V4DF
	    (match_operand:V2DF 1 "nonimmediate_operand")
	    (match_operand:V2DF 2 "nonimmediate_operand"))
	  (parallel [(const_int 1)
		     (const_int 3)])))]
  "TARGET_SSE2"
{
  if (!ix86_vec_interleave_v2df_operator_ok (operands, 1))
    operands[2] = force_reg (V2DFmode, operands[2]);
})

(define_insn "*vec_interleave_highv2df"
  [(set (match_operand:V2DF 0 "nonimmediate_operand"     "=x,x,x,x,x,m")
	(vec_select:V2DF
	  (vec_concat:V4DF
	    (match_operand:V2DF 1 "nonimmediate_operand" " 0,x,o,o,o,x")
	    (match_operand:V2DF 2 "nonimmediate_operand" " x,x,1,0,x,0"))
	  (parallel [(const_int 1)
		     (const_int 3)])))]
  "TARGET_SSE2 && ix86_vec_interleave_v2df_operator_ok (operands, 1)"
  "@
   unpckhpd\t{%2, %0|%0, %2}
   vunpckhpd\t{%2, %1, %0|%0, %1, %2}
   %vmovddup\t{%H1, %0|%0, %H1}
   movlpd\t{%H1, %0|%0, %H1}
   vmovlpd\t{%H1, %2, %0|%0, %2, %H1}
   %vmovhpd\t{%1, %0|%0, %1}"
  [(set_attr "isa" "noavx,avx,sse3,noavx,avx,*")
   (set_attr "type" "sselog,sselog,sselog,ssemov,ssemov,ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix_data16" "*,*,*,1,*,1")
   (set_attr "prefix" "orig,vex,maybe_vex,orig,vex,maybe_vex")
   (set_attr "mode" "V2DF,V2DF,DF,V1DF,V1DF,V1DF")])

;; Recall that the 256-bit unpck insns only shuffle within their lanes.
(define_expand "avx_movddup256"
  [(set (match_operand:V4DF 0 "register_operand")
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_operand:V4DF 1 "nonimmediate_operand")
	    (match_dup 1))
	  (parallel [(const_int 0) (const_int 4)
		     (const_int 2) (const_int 6)])))]
  "TARGET_AVX")

(define_expand "avx_unpcklpd256"
  [(set (match_operand:V4DF 0 "register_operand")
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_operand:V4DF 1 "register_operand")
	    (match_operand:V4DF 2 "nonimmediate_operand"))
	  (parallel [(const_int 0) (const_int 4)
		     (const_int 2) (const_int 6)])))]
  "TARGET_AVX")

(define_insn "*avx_unpcklpd256"
  [(set (match_operand:V4DF 0 "register_operand"         "=x,x")
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_operand:V4DF 1 "nonimmediate_operand" " x,m")
	    (match_operand:V4DF 2 "nonimmediate_operand" "xm,1"))
	  (parallel [(const_int 0) (const_int 4)
		     (const_int 2) (const_int 6)])))]
  "TARGET_AVX"
  "@
   vunpcklpd\t{%2, %1, %0|%0, %1, %2}
   vmovddup\t{%1, %0|%0, %1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

(define_expand "vec_interleave_lowv4df"
  [(set (match_dup 3)
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_operand:V4DF 1 "register_operand" "x")
	    (match_operand:V4DF 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0) (const_int 4)
		     (const_int 2) (const_int 6)])))
   (set (match_dup 4)
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_dup 1)
	    (match_dup 2))
	  (parallel [(const_int 1) (const_int 5)
		     (const_int 3) (const_int 7)])))
   (set (match_operand:V4DF 0 "register_operand")
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_dup 3)
	    (match_dup 4))
	  (parallel [(const_int 0) (const_int 1)
		     (const_int 4) (const_int 5)])))]
 "TARGET_AVX"
{
  operands[3] = gen_reg_rtx (V4DFmode);
  operands[4] = gen_reg_rtx (V4DFmode);
})

(define_expand "vec_interleave_lowv2df"
  [(set (match_operand:V2DF 0 "register_operand")
	(vec_select:V2DF
	  (vec_concat:V4DF
	    (match_operand:V2DF 1 "nonimmediate_operand")
	    (match_operand:V2DF 2 "nonimmediate_operand"))
	  (parallel [(const_int 0)
		     (const_int 2)])))]
  "TARGET_SSE2"
{
  if (!ix86_vec_interleave_v2df_operator_ok (operands, 0))
    operands[1] = force_reg (V2DFmode, operands[1]);
})

(define_insn "*vec_interleave_lowv2df"
  [(set (match_operand:V2DF 0 "nonimmediate_operand"     "=x,x,x,x,x,o")
	(vec_select:V2DF
	  (vec_concat:V4DF
	    (match_operand:V2DF 1 "nonimmediate_operand" " 0,x,m,0,x,0")
	    (match_operand:V2DF 2 "nonimmediate_operand" " x,x,1,m,m,x"))
	  (parallel [(const_int 0)
		     (const_int 2)])))]
  "TARGET_SSE2 && ix86_vec_interleave_v2df_operator_ok (operands, 0)"
  "@
   unpcklpd\t{%2, %0|%0, %2}
   vunpcklpd\t{%2, %1, %0|%0, %1, %2}
   %vmovddup\t{%1, %0|%0, %1}
   movhpd\t{%2, %0|%0, %2}
   vmovhpd\t{%2, %1, %0|%0, %1, %2}
   %vmovlpd\t{%2, %H0|%H0, %2}"
  [(set_attr "isa" "noavx,avx,sse3,noavx,avx,*")
   (set_attr "type" "sselog,sselog,sselog,ssemov,ssemov,ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix_data16" "*,*,*,1,*,1")
   (set_attr "prefix" "orig,vex,maybe_vex,orig,vex,maybe_vex")
   (set_attr "mode" "V2DF,V2DF,DF,V1DF,V1DF,V1DF")])

(define_split
  [(set (match_operand:V2DF 0 "memory_operand")
	(vec_select:V2DF
	  (vec_concat:V4DF
	    (match_operand:V2DF 1 "register_operand")
	    (match_dup 1))
	  (parallel [(const_int 0)
		     (const_int 2)])))]
  "TARGET_SSE3 && reload_completed"
  [(const_int 0)]
{
  rtx low = gen_rtx_REG (DFmode, REGNO (operands[1]));
  emit_move_insn (adjust_address (operands[0], DFmode, 0), low);
  emit_move_insn (adjust_address (operands[0], DFmode, 8), low);
  DONE;
})

(define_split
  [(set (match_operand:V2DF 0 "register_operand")
	(vec_select:V2DF
	  (vec_concat:V4DF
	    (match_operand:V2DF 1 "memory_operand")
	    (match_dup 1))
	  (parallel [(match_operand:SI 2 "const_0_to_1_operand")
		     (match_operand:SI 3 "const_int_operand")])))]
  "TARGET_SSE3 && INTVAL (operands[2]) + 2 == INTVAL (operands[3])"
  [(set (match_dup 0) (vec_duplicate:V2DF (match_dup 1)))]
{
  operands[1] = adjust_address (operands[1], DFmode, INTVAL (operands[2]) * 8);
})

(define_expand "avx_shufpd256"
  [(match_operand:V4DF 0 "register_operand")
   (match_operand:V4DF 1 "register_operand")
   (match_operand:V4DF 2 "nonimmediate_operand")
   (match_operand:SI 3 "const_int_operand")]
  "TARGET_AVX"
{
  int mask = INTVAL (operands[3]);
  emit_insn (gen_avx_shufpd256_1 (operands[0], operands[1], operands[2],
				   GEN_INT (mask & 1),
				   GEN_INT (mask & 2 ? 5 : 4),
				   GEN_INT (mask & 4 ? 3 : 2),
				   GEN_INT (mask & 8 ? 7 : 6)));
  DONE;
})

(define_insn "avx_shufpd256_1"
  [(set (match_operand:V4DF 0 "register_operand" "=x")
	(vec_select:V4DF
	  (vec_concat:V8DF
	    (match_operand:V4DF 1 "register_operand" "x")
	    (match_operand:V4DF 2 "nonimmediate_operand" "xm"))
	  (parallel [(match_operand 3 "const_0_to_1_operand")
		     (match_operand 4 "const_4_to_5_operand")
		     (match_operand 5 "const_2_to_3_operand")
		     (match_operand 6 "const_6_to_7_operand")])))]
  "TARGET_AVX"
{
  int mask;
  mask = INTVAL (operands[3]);
  mask |= (INTVAL (operands[4]) - 4) << 1;
  mask |= (INTVAL (operands[5]) - 2) << 2;
  mask |= (INTVAL (operands[6]) - 6) << 3;
  operands[3] = GEN_INT (mask);

  return "vshufpd\t{%3, %2, %1, %0|%0, %1, %2, %3}";
}
  [(set_attr "type" "sseshuf")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

(define_expand "sse2_shufpd"
  [(match_operand:V2DF 0 "register_operand")
   (match_operand:V2DF 1 "register_operand")
   (match_operand:V2DF 2 "nonimmediate_operand")
   (match_operand:SI 3 "const_int_operand")]
  "TARGET_SSE2"
{
  int mask = INTVAL (operands[3]);
  emit_insn (gen_sse2_shufpd_v2df (operands[0], operands[1], operands[2],
				GEN_INT (mask & 1),
				GEN_INT (mask & 2 ? 3 : 2)));
  DONE;
})

;; punpcklqdq and punpckhqdq are shorter than shufpd.
(define_insn "avx2_interleave_highv4di"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(vec_select:V4DI
	  (vec_concat:V8DI
	    (match_operand:V4DI 1 "register_operand" "x")
	    (match_operand:V4DI 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 1)
		     (const_int 5)
		     (const_int 3)
		     (const_int 7)])))]
  "TARGET_AVX2"
  "vpunpckhqdq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_interleave_highv2di"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(vec_select:V2DI
	  (vec_concat:V4DI
	    (match_operand:V2DI 1 "register_operand" "0,x")
	    (match_operand:V2DI 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 1)
		     (const_int 3)])))]
  "TARGET_SSE2"
  "@
   punpckhqdq\t{%2, %0|%0, %2}
   vpunpckhqdq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_interleave_lowv4di"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(vec_select:V4DI
	  (vec_concat:V8DI
	    (match_operand:V4DI 1 "register_operand" "x")
	    (match_operand:V4DI 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0)
		     (const_int 4)
		     (const_int 2)
		     (const_int 6)])))]
  "TARGET_AVX2"
  "vpunpcklqdq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_interleave_lowv2di"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(vec_select:V2DI
	  (vec_concat:V4DI
	    (match_operand:V2DI 1 "register_operand" "0,x")
	    (match_operand:V2DI 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 0)
		     (const_int 2)])))]
  "TARGET_SSE2"
  "@
   punpcklqdq\t{%2, %0|%0, %2}
   vpunpcklqdq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "sse2_shufpd_<mode>"
  [(set (match_operand:VI8F_128 0 "register_operand" "=x,x")
	(vec_select:VI8F_128
	  (vec_concat:<ssedoublevecmode>
	    (match_operand:VI8F_128 1 "register_operand" "0,x")
	    (match_operand:VI8F_128 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(match_operand 3 "const_0_to_1_operand")
		     (match_operand 4 "const_2_to_3_operand")])))]
  "TARGET_SSE2"
{
  int mask;
  mask = INTVAL (operands[3]);
  mask |= (INTVAL (operands[4]) - 2) << 1;
  operands[3] = GEN_INT (mask);

  switch (which_alternative)
    {
    case 0:
      return "shufpd\t{%3, %2, %0|%0, %2, %3}";
    case 1:
      return "vshufpd\t{%3, %2, %1, %0|%0, %1, %2, %3}";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseshuf")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "V2DF")])

;; Avoid combining registers from different units in a single alternative,
;; see comment above inline_secondary_memory_needed function in i386.c
(define_insn "sse2_storehpd"
  [(set (match_operand:DF 0 "nonimmediate_operand"     "=m,x,x,x,*f,r")
	(vec_select:DF
	  (match_operand:V2DF 1 "nonimmediate_operand" " x,0,x,o,o,o")
	  (parallel [(const_int 1)])))]
  "TARGET_SSE2 && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "@
   %vmovhpd\t{%1, %0|%0, %1}
   unpckhpd\t%0, %0
   vunpckhpd\t{%d1, %0|%0, %d1}
   #
   #
   #"
  [(set_attr "isa" "*,noavx,avx,*,*,*")
   (set_attr "type" "ssemov,sselog1,sselog1,ssemov,fmov,imov")
   (set (attr "prefix_data16")
     (if_then_else
       (and (eq_attr "alternative" "0")
	    (not (match_test "TARGET_AVX")))
       (const_string "1")
       (const_string "*")))
   (set_attr "prefix" "maybe_vex,orig,vex,*,*,*")
   (set_attr "mode" "V1DF,V1DF,V2DF,DF,DF,DF")])

(define_split
  [(set (match_operand:DF 0 "register_operand")
	(vec_select:DF
	  (match_operand:V2DF 1 "memory_operand")
	  (parallel [(const_int 1)])))]
  "TARGET_SSE2 && reload_completed"
  [(set (match_dup 0) (match_dup 1))]
  "operands[1] = adjust_address (operands[1], DFmode, 8);")

(define_insn "*vec_extractv2df_1_sse"
  [(set (match_operand:DF 0 "nonimmediate_operand" "=m,x,x")
	(vec_select:DF
	  (match_operand:V2DF 1 "nonimmediate_operand" "x,x,o")
	  (parallel [(const_int 1)])))]
  "!TARGET_SSE2 && TARGET_SSE
   && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "@
   movhps\t{%1, %0|%0, %1}
   movhlps\t{%1, %0|%0, %1}
   movlps\t{%H1, %0|%0, %H1}"
  [(set_attr "type" "ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "mode" "V2SF,V4SF,V2SF")])

;; Avoid combining registers from different units in a single alternative,
;; see comment above inline_secondary_memory_needed function in i386.c
(define_insn "sse2_storelpd"
  [(set (match_operand:DF 0 "nonimmediate_operand"     "=m,x,x,*f,r")
	(vec_select:DF
	  (match_operand:V2DF 1 "nonimmediate_operand" " x,x,m,m,m")
	  (parallel [(const_int 0)])))]
  "TARGET_SSE2 && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "@
   %vmovlpd\t{%1, %0|%0, %1}
   #
   #
   #
   #"
  [(set_attr "type" "ssemov,ssemov,ssemov,fmov,imov")
   (set_attr "prefix_data16" "1,*,*,*,*")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "V1DF,DF,DF,DF,DF")])

(define_split
  [(set (match_operand:DF 0 "register_operand")
	(vec_select:DF
	  (match_operand:V2DF 1 "nonimmediate_operand")
	  (parallel [(const_int 0)])))]
  "TARGET_SSE2 && reload_completed"
  [(const_int 0)]
{
  rtx op1 = operands[1];
  if (REG_P (op1))
    op1 = gen_rtx_REG (DFmode, REGNO (op1));
  else
    op1 = gen_lowpart (DFmode, op1);
  emit_move_insn (operands[0], op1);
  DONE;
})

(define_insn "*vec_extractv2df_0_sse"
  [(set (match_operand:DF 0 "nonimmediate_operand" "=m,x,x")
	(vec_select:DF
	  (match_operand:V2DF 1 "nonimmediate_operand" "x,x,m")
	  (parallel [(const_int 0)])))]
  "!TARGET_SSE2 && TARGET_SSE
   && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "@
   movlps\t{%1, %0|%0, %1}
   movaps\t{%1, %0|%0, %1}
   movlps\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "mode" "V2SF,V4SF,V2SF")])

(define_expand "sse2_loadhpd_exp"
  [(set (match_operand:V2DF 0 "nonimmediate_operand")
	(vec_concat:V2DF
	  (vec_select:DF
	    (match_operand:V2DF 1 "nonimmediate_operand")
	    (parallel [(const_int 0)]))
	  (match_operand:DF 2 "nonimmediate_operand")))]
  "TARGET_SSE2"
{
  rtx dst = ix86_fixup_binary_operands (UNKNOWN, V2DFmode, operands);

  emit_insn (gen_sse2_loadhpd (dst, operands[1], operands[2]));

  /* Fix up the destination if needed.  */
  if (dst != operands[0])
    emit_move_insn (operands[0], dst);

  DONE;
})

;; Avoid combining registers from different units in a single alternative,
;; see comment above inline_secondary_memory_needed function in i386.c
(define_insn "sse2_loadhpd"
  [(set (match_operand:V2DF 0 "nonimmediate_operand"
	  "=x,x,x,x,o,o ,o")
	(vec_concat:V2DF
	  (vec_select:DF
	    (match_operand:V2DF 1 "nonimmediate_operand"
	  " 0,x,0,x,0,0 ,0")
	    (parallel [(const_int 0)]))
	  (match_operand:DF 2 "nonimmediate_operand"
	  " m,m,x,x,x,*f,r")))]
  "TARGET_SSE2 && !(MEM_P (operands[1]) && MEM_P (operands[2]))"
  "@
   movhpd\t{%2, %0|%0, %2}
   vmovhpd\t{%2, %1, %0|%0, %1, %2}
   unpcklpd\t{%2, %0|%0, %2}
   vunpcklpd\t{%2, %1, %0|%0, %1, %2}
   #
   #
   #"
  [(set_attr "isa" "noavx,avx,noavx,avx,*,*,*")
   (set_attr "type" "ssemov,ssemov,sselog,sselog,ssemov,fmov,imov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix_data16" "1,*,*,*,*,*,*")
   (set_attr "prefix" "orig,vex,orig,vex,*,*,*")
   (set_attr "mode" "V1DF,V1DF,V2DF,V2DF,DF,DF,DF")])

(define_split
  [(set (match_operand:V2DF 0 "memory_operand")
	(vec_concat:V2DF
	  (vec_select:DF (match_dup 0) (parallel [(const_int 0)]))
	  (match_operand:DF 1 "register_operand")))]
  "TARGET_SSE2 && reload_completed"
  [(set (match_dup 0) (match_dup 1))]
  "operands[0] = adjust_address (operands[0], DFmode, 8);")

(define_expand "sse2_loadlpd_exp"
  [(set (match_operand:V2DF 0 "nonimmediate_operand")
	(vec_concat:V2DF
	  (match_operand:DF 2 "nonimmediate_operand")
	  (vec_select:DF
	    (match_operand:V2DF 1 "nonimmediate_operand")
	    (parallel [(const_int 1)]))))]
  "TARGET_SSE2"
{
  rtx dst = ix86_fixup_binary_operands (UNKNOWN, V2DFmode, operands);

  emit_insn (gen_sse2_loadlpd (dst, operands[1], operands[2]));

  /* Fix up the destination if needed.  */
  if (dst != operands[0])
    emit_move_insn (operands[0], dst);

  DONE;
})

;; Avoid combining registers from different units in a single alternative,
;; see comment above inline_secondary_memory_needed function in i386.c
(define_insn "sse2_loadlpd"
  [(set (match_operand:V2DF 0 "nonimmediate_operand"
	  "=x,x,x,x,x,x,x,x,m,m ,m")
	(vec_concat:V2DF
	  (match_operand:DF 2 "nonimmediate_operand"
	  " m,m,m,x,x,0,0,x,x,*f,r")
	  (vec_select:DF
	    (match_operand:V2DF 1 "vector_move_operand"
	  " C,0,x,0,x,x,o,o,0,0 ,0")
	    (parallel [(const_int 1)]))))]
  "TARGET_SSE2 && !(MEM_P (operands[1]) && MEM_P (operands[2]))"
  "@
   %vmovsd\t{%2, %0|%0, %2}
   movlpd\t{%2, %0|%0, %2}
   vmovlpd\t{%2, %1, %0|%0, %1, %2}
   movsd\t{%2, %0|%0, %2}
   vmovsd\t{%2, %1, %0|%0, %1, %2}
   shufpd\t{$2, %1, %0|%0, %1, 2}
   movhpd\t{%H1, %0|%0, %H1}
   vmovhpd\t{%H1, %2, %0|%0, %2, %H1}
   #
   #
   #"
  [(set_attr "isa" "*,noavx,avx,noavx,avx,noavx,noavx,avx,*,*,*")
   (set (attr "type")
     (cond [(eq_attr "alternative" "5")
	      (const_string "sselog")
	    (eq_attr "alternative" "9")
	      (const_string "fmov")
	    (eq_attr "alternative" "10")
	      (const_string "imov")
	   ]
	   (const_string "ssemov")))
   (set_attr "ssememalign" "64")
   (set_attr "prefix_data16" "*,1,*,*,*,*,1,*,*,*,*")
   (set_attr "length_immediate" "*,*,*,*,*,1,*,*,*,*,*")
   (set_attr "prefix" "maybe_vex,orig,vex,orig,vex,orig,orig,vex,*,*,*")
   (set_attr "mode" "DF,V1DF,V1DF,V1DF,V1DF,V2DF,V1DF,V1DF,DF,DF,DF")])

(define_split
  [(set (match_operand:V2DF 0 "memory_operand")
	(vec_concat:V2DF
	  (match_operand:DF 1 "register_operand")
	  (vec_select:DF (match_dup 0) (parallel [(const_int 1)]))))]
  "TARGET_SSE2 && reload_completed"
  [(set (match_dup 0) (match_dup 1))]
  "operands[0] = adjust_address (operands[0], DFmode, 0);")

(define_insn "sse2_movsd"
  [(set (match_operand:V2DF 0 "nonimmediate_operand"   "=x,x,x,x,m,x,x,x,o")
	(vec_merge:V2DF
	  (match_operand:V2DF 2 "nonimmediate_operand" " x,x,m,m,x,0,0,x,0")
	  (match_operand:V2DF 1 "nonimmediate_operand" " 0,x,0,x,0,x,o,o,x")
	  (const_int 1)))]
  "TARGET_SSE2"
  "@
   movsd\t{%2, %0|%0, %2}
   vmovsd\t{%2, %1, %0|%0, %1, %2}
   movlpd\t{%2, %0|%0, %2}
   vmovlpd\t{%2, %1, %0|%0, %1, %2}
   %vmovlpd\t{%2, %0|%0, %2}
   shufpd\t{$2, %1, %0|%0, %1, 2}
   movhps\t{%H1, %0|%0, %H1}
   vmovhps\t{%H1, %2, %0|%0, %2, %H1}
   %vmovhps\t{%1, %H0|%H0, %1}"
  [(set_attr "isa" "noavx,avx,noavx,avx,*,noavx,noavx,avx,*")
   (set (attr "type")
     (if_then_else
       (eq_attr "alternative" "5")
       (const_string "sselog")
       (const_string "ssemov")))
   (set (attr "prefix_data16")
     (if_then_else
       (and (eq_attr "alternative" "2,4")
	    (not (match_test "TARGET_AVX")))
       (const_string "1")
       (const_string "*")))
   (set_attr "length_immediate" "*,*,*,*,*,1,*,*,*")
   (set_attr "ssememalign" "64")
   (set_attr "prefix" "orig,vex,orig,vex,maybe_vex,orig,orig,vex,maybe_vex")
   (set_attr "mode" "DF,DF,V1DF,V1DF,V1DF,V2DF,V1DF,V1DF,V1DF")])

(define_insn "vec_dupv2df"
  [(set (match_operand:V2DF 0 "register_operand"     "=x,x")
	(vec_duplicate:V2DF
	  (match_operand:DF 1 "nonimmediate_operand" " 0,xm")))]
  "TARGET_SSE2"
  "@
   unpcklpd\t%0, %0
   %vmovddup\t{%1, %0|%0, %1}"
  [(set_attr "isa" "noavx,sse3")
   (set_attr "type" "sselog1")
   (set_attr "prefix" "orig,maybe_vex")
   (set_attr "mode" "V2DF,DF")])

(define_insn "*vec_concatv2df"
  [(set (match_operand:V2DF 0 "register_operand"     "=x,x,x,x,x,x,x,x")
	(vec_concat:V2DF
	  (match_operand:DF 1 "nonimmediate_operand" " 0,x,m,0,x,m,0,0")
	  (match_operand:DF 2 "vector_move_operand"  " x,x,1,m,m,C,x,m")))]
  "TARGET_SSE"
  "@
   unpcklpd\t{%2, %0|%0, %2}
   vunpcklpd\t{%2, %1, %0|%0, %1, %2}
   %vmovddup\t{%1, %0|%0, %1}
   movhpd\t{%2, %0|%0, %2}
   vmovhpd\t{%2, %1, %0|%0, %1, %2}
   %vmovsd\t{%1, %0|%0, %1}
   movlhps\t{%2, %0|%0, %2}
   movhps\t{%2, %0|%0, %2}"
  [(set_attr "isa" "sse2_noavx,avx,sse3,sse2_noavx,avx,sse2,noavx,noavx")
   (set (attr "type")
     (if_then_else
       (eq_attr "alternative" "0,1,2")
       (const_string "sselog")
       (const_string "ssemov")))
   (set_attr "prefix_data16" "*,*,*,1,*,*,*,*")
   (set_attr "prefix" "orig,vex,maybe_vex,orig,vex,maybe_vex,orig,orig")
   (set_attr "mode" "V2DF,V2DF,DF,V1DF,V1DF,DF,V4SF,V2SF")])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel integral arithmetic
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_expand "neg<mode>2"
  [(set (match_operand:VI_AVX2 0 "register_operand")
	(minus:VI_AVX2
	  (match_dup 2)
	  (match_operand:VI_AVX2 1 "nonimmediate_operand")))]
  "TARGET_SSE2"
  "operands[2] = force_reg (<MODE>mode, CONST0_RTX (<MODE>mode));")

(define_expand "<plusminus_insn><mode>3"
  [(set (match_operand:VI_AVX2 0 "register_operand")
	(plusminus:VI_AVX2
	  (match_operand:VI_AVX2 1 "nonimmediate_operand")
	  (match_operand:VI_AVX2 2 "nonimmediate_operand")))]
  "TARGET_SSE2"
  "ix86_fixup_binary_operands_no_copy (<CODE>, <MODE>mode, operands);")

(define_insn "*<plusminus_insn><mode>3"
  [(set (match_operand:VI_AVX2 0 "register_operand" "=x,x")
	(plusminus:VI_AVX2
	  (match_operand:VI_AVX2 1 "nonimmediate_operand" "<comm>0,x")
	  (match_operand:VI_AVX2 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE2 && ix86_binary_operator_ok (<CODE>, <MODE>mode, operands)"
  "@
   p<plusminus_mnemonic><ssemodesuffix>\t{%2, %0|%0, %2}
   vp<plusminus_mnemonic><ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_expand "<sse2_avx2>_<plusminus_insn><mode>3"
  [(set (match_operand:VI12_AVX2 0 "register_operand")
	(sat_plusminus:VI12_AVX2
	  (match_operand:VI12_AVX2 1 "nonimmediate_operand")
	  (match_operand:VI12_AVX2 2 "nonimmediate_operand")))]
  "TARGET_SSE2"
  "ix86_fixup_binary_operands_no_copy (<CODE>, <MODE>mode, operands);")

(define_insn "*<sse2_avx2>_<plusminus_insn><mode>3"
  [(set (match_operand:VI12_AVX2 0 "register_operand" "=x,x")
	(sat_plusminus:VI12_AVX2
	  (match_operand:VI12_AVX2 1 "nonimmediate_operand" "<comm>0,x")
	  (match_operand:VI12_AVX2 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE2 && ix86_binary_operator_ok (<CODE>, <MODE>mode, operands)"
  "@
   p<plusminus_mnemonic><ssemodesuffix>\t{%2, %0|%0, %2}
   vp<plusminus_mnemonic><ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_expand "mul<mode>3"
  [(set (match_operand:VI1_AVX2 0 "register_operand")
	(mult:VI1_AVX2 (match_operand:VI1_AVX2 1 "register_operand")
		       (match_operand:VI1_AVX2 2 "register_operand")))]
  "TARGET_SSE2"
{
  ix86_expand_vecop_qihi (MULT, operands[0], operands[1], operands[2]);
  DONE;
})

(define_expand "mul<mode>3"
  [(set (match_operand:VI2_AVX2 0 "register_operand")
	(mult:VI2_AVX2 (match_operand:VI2_AVX2 1 "nonimmediate_operand")
		       (match_operand:VI2_AVX2 2 "nonimmediate_operand")))]
  "TARGET_SSE2"
  "ix86_fixup_binary_operands_no_copy (MULT, <MODE>mode, operands);")

(define_insn "*mul<mode>3"
  [(set (match_operand:VI2_AVX2 0 "register_operand" "=x,x")
	(mult:VI2_AVX2 (match_operand:VI2_AVX2 1 "nonimmediate_operand" "%0,x")
		       (match_operand:VI2_AVX2 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE2 && ix86_binary_operator_ok (MULT, <MODE>mode, operands)"
  "@
   pmullw\t{%2, %0|%0, %2}
   vpmullw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseimul")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_expand "<s>mul<mode>3_highpart"
  [(set (match_operand:VI2_AVX2 0 "register_operand")
	(truncate:VI2_AVX2
	  (lshiftrt:<ssedoublemode>
	    (mult:<ssedoublemode>
	      (any_extend:<ssedoublemode>
		(match_operand:VI2_AVX2 1 "nonimmediate_operand"))
	      (any_extend:<ssedoublemode>
		(match_operand:VI2_AVX2 2 "nonimmediate_operand")))
	    (const_int 16))))]
  "TARGET_SSE2"
  "ix86_fixup_binary_operands_no_copy (MULT, <MODE>mode, operands);")

(define_insn "*<s>mul<mode>3_highpart"
  [(set (match_operand:VI2_AVX2 0 "register_operand" "=x,x")
	(truncate:VI2_AVX2
	  (lshiftrt:<ssedoublemode>
	    (mult:<ssedoublemode>
	      (any_extend:<ssedoublemode>
		(match_operand:VI2_AVX2 1 "nonimmediate_operand" "%0,x"))
	      (any_extend:<ssedoublemode>
		(match_operand:VI2_AVX2 2 "nonimmediate_operand" "xm,xm")))
	    (const_int 16))))]
  "TARGET_SSE2 && ix86_binary_operator_ok (MULT, <MODE>mode, operands)"
  "@
   pmulh<u>w\t{%2, %0|%0, %2}
   vpmulh<u>w\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseimul")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_expand "vec_widen_umult_even_v8si"
  [(set (match_operand:V4DI 0 "register_operand")
	(mult:V4DI
	  (zero_extend:V4DI
	    (vec_select:V4SI
	      (match_operand:V8SI 1 "nonimmediate_operand")
	      (parallel [(const_int 0) (const_int 2)
			 (const_int 4) (const_int 6)])))
	  (zero_extend:V4DI
	    (vec_select:V4SI
	      (match_operand:V8SI 2 "nonimmediate_operand")
	      (parallel [(const_int 0) (const_int 2)
			 (const_int 4) (const_int 6)])))))]
  "TARGET_AVX2"
  "ix86_fixup_binary_operands_no_copy (MULT, V8SImode, operands);")

(define_insn "*vec_widen_umult_even_v8si"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(mult:V4DI
	  (zero_extend:V4DI
	    (vec_select:V4SI
	      (match_operand:V8SI 1 "nonimmediate_operand" "%x")
	      (parallel [(const_int 0) (const_int 2)
			 (const_int 4) (const_int 6)])))
	  (zero_extend:V4DI
	    (vec_select:V4SI
	      (match_operand:V8SI 2 "nonimmediate_operand" "xm")
	      (parallel [(const_int 0) (const_int 2)
			 (const_int 4) (const_int 6)])))))]
  "TARGET_AVX2 && ix86_binary_operator_ok (MULT, V8SImode, operands)"
  "vpmuludq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseimul")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_expand "vec_widen_umult_even_v4si"
  [(set (match_operand:V2DI 0 "register_operand")
	(mult:V2DI
	  (zero_extend:V2DI
	    (vec_select:V2SI
	      (match_operand:V4SI 1 "nonimmediate_operand")
	      (parallel [(const_int 0) (const_int 2)])))
	  (zero_extend:V2DI
	    (vec_select:V2SI
	      (match_operand:V4SI 2 "nonimmediate_operand")
	      (parallel [(const_int 0) (const_int 2)])))))]
  "TARGET_SSE2"
  "ix86_fixup_binary_operands_no_copy (MULT, V4SImode, operands);")

(define_insn "*vec_widen_umult_even_v4si"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(mult:V2DI
	  (zero_extend:V2DI
	    (vec_select:V2SI
	      (match_operand:V4SI 1 "nonimmediate_operand" "%0,x")
	      (parallel [(const_int 0) (const_int 2)])))
	  (zero_extend:V2DI
	    (vec_select:V2SI
	      (match_operand:V4SI 2 "nonimmediate_operand" "xm,xm")
	      (parallel [(const_int 0) (const_int 2)])))))]
  "TARGET_SSE2 && ix86_binary_operator_ok (MULT, V4SImode, operands)"
  "@
   pmuludq\t{%2, %0|%0, %2}
   vpmuludq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseimul")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_expand "vec_widen_smult_even_v8si"
  [(set (match_operand:V4DI 0 "register_operand")
	(mult:V4DI
	  (sign_extend:V4DI
	    (vec_select:V4SI
	      (match_operand:V8SI 1 "nonimmediate_operand")
	      (parallel [(const_int 0) (const_int 2)
			 (const_int 4) (const_int 6)])))
	  (sign_extend:V4DI
	    (vec_select:V4SI
	      (match_operand:V8SI 2 "nonimmediate_operand")
	      (parallel [(const_int 0) (const_int 2)
			 (const_int 4) (const_int 6)])))))]
  "TARGET_AVX2"
  "ix86_fixup_binary_operands_no_copy (MULT, V8SImode, operands);")

(define_insn "*vec_widen_smult_even_v8si"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(mult:V4DI
	  (sign_extend:V4DI
	    (vec_select:V4SI
	      (match_operand:V8SI 1 "nonimmediate_operand" "x")
	      (parallel [(const_int 0) (const_int 2)
			 (const_int 4) (const_int 6)])))
	  (sign_extend:V4DI
	    (vec_select:V4SI
	      (match_operand:V8SI 2 "nonimmediate_operand" "xm")
	      (parallel [(const_int 0) (const_int 2)
			 (const_int 4) (const_int 6)])))))]
  "TARGET_AVX2 && ix86_binary_operator_ok (MULT, V8SImode, operands)"
  "vpmuldq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "avx")
   (set_attr "type" "sseimul")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_expand "sse4_1_mulv2siv2di3"
  [(set (match_operand:V2DI 0 "register_operand")
	(mult:V2DI
	  (sign_extend:V2DI
	    (vec_select:V2SI
	      (match_operand:V4SI 1 "nonimmediate_operand")
	      (parallel [(const_int 0) (const_int 2)])))
	  (sign_extend:V2DI
	    (vec_select:V2SI
	      (match_operand:V4SI 2 "nonimmediate_operand")
	      (parallel [(const_int 0) (const_int 2)])))))]
  "TARGET_SSE4_1"
  "ix86_fixup_binary_operands_no_copy (MULT, V4SImode, operands);")

(define_insn "*sse4_1_mulv2siv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(mult:V2DI
	  (sign_extend:V2DI
	    (vec_select:V2SI
	      (match_operand:V4SI 1 "nonimmediate_operand" "%0,x")
	      (parallel [(const_int 0) (const_int 2)])))
	  (sign_extend:V2DI
	    (vec_select:V2SI
	      (match_operand:V4SI 2 "nonimmediate_operand" "xm,xm")
	      (parallel [(const_int 0) (const_int 2)])))))]
  "TARGET_SSE4_1 && ix86_binary_operator_ok (MULT, V4SImode, operands)"
  "@
   pmuldq\t{%2, %0|%0, %2}
   vpmuldq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseimul")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_expand "avx2_pmaddwd"
  [(set (match_operand:V8SI 0 "register_operand")
	(plus:V8SI
	  (mult:V8SI
	    (sign_extend:V8SI
	      (vec_select:V8HI
		(match_operand:V16HI 1 "nonimmediate_operand")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)
			   (const_int 8) (const_int 10)
			   (const_int 12) (const_int 14)])))
	    (sign_extend:V8SI
	      (vec_select:V8HI
		(match_operand:V16HI 2 "nonimmediate_operand")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)
			   (const_int 8) (const_int 10)
			   (const_int 12) (const_int 14)]))))
	  (mult:V8SI
	    (sign_extend:V8SI
	      (vec_select:V8HI (match_dup 1)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)
			   (const_int 9) (const_int 11)
			   (const_int 13) (const_int 15)])))
	    (sign_extend:V8SI
	      (vec_select:V8HI (match_dup 2)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)
			   (const_int 9) (const_int 11)
			   (const_int 13) (const_int 15)]))))))]
  "TARGET_AVX2"
  "ix86_fixup_binary_operands_no_copy (MULT, V16HImode, operands);")

(define_insn "*avx2_pmaddwd"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(plus:V8SI
	  (mult:V8SI
	    (sign_extend:V8SI
	      (vec_select:V8HI
		(match_operand:V16HI 1 "nonimmediate_operand" "%x")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)
			   (const_int 8) (const_int 10)
			   (const_int 12) (const_int 14)])))
	    (sign_extend:V8SI
	      (vec_select:V8HI
		(match_operand:V16HI 2 "nonimmediate_operand" "xm")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)
			   (const_int 8) (const_int 10)
			   (const_int 12) (const_int 14)]))))
	  (mult:V8SI
	    (sign_extend:V8SI
	      (vec_select:V8HI (match_dup 1)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)
			   (const_int 9) (const_int 11)
			   (const_int 13) (const_int 15)])))
	    (sign_extend:V8SI
	      (vec_select:V8HI (match_dup 2)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)
			   (const_int 9) (const_int 11)
			   (const_int 13) (const_int 15)]))))))]
  "TARGET_AVX2 && ix86_binary_operator_ok (MULT, V16HImode, operands)"
  "vpmaddwd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseiadd")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_expand "sse2_pmaddwd"
  [(set (match_operand:V4SI 0 "register_operand")
	(plus:V4SI
	  (mult:V4SI
	    (sign_extend:V4SI
	      (vec_select:V4HI
		(match_operand:V8HI 1 "nonimmediate_operand")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)])))
	    (sign_extend:V4SI
	      (vec_select:V4HI
		(match_operand:V8HI 2 "nonimmediate_operand")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)]))))
	  (mult:V4SI
	    (sign_extend:V4SI
	      (vec_select:V4HI (match_dup 1)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)])))
	    (sign_extend:V4SI
	      (vec_select:V4HI (match_dup 2)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)]))))))]
  "TARGET_SSE2"
  "ix86_fixup_binary_operands_no_copy (MULT, V8HImode, operands);")

(define_insn "*sse2_pmaddwd"
  [(set (match_operand:V4SI 0 "register_operand" "=x,x")
	(plus:V4SI
	  (mult:V4SI
	    (sign_extend:V4SI
	      (vec_select:V4HI
		(match_operand:V8HI 1 "nonimmediate_operand" "%0,x")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)])))
	    (sign_extend:V4SI
	      (vec_select:V4HI
		(match_operand:V8HI 2 "nonimmediate_operand" "xm,xm")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)]))))
	  (mult:V4SI
	    (sign_extend:V4SI
	      (vec_select:V4HI (match_dup 1)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)])))
	    (sign_extend:V4SI
	      (vec_select:V4HI (match_dup 2)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)]))))))]
  "TARGET_SSE2 && ix86_binary_operator_ok (MULT, V8HImode, operands)"
  "@
   pmaddwd\t{%2, %0|%0, %2}
   vpmaddwd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "atom_unit" "simul")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_expand "mul<mode>3"
  [(set (match_operand:VI4_AVX2 0 "register_operand")
	(mult:VI4_AVX2
	  (match_operand:VI4_AVX2 1 "nonimmediate_operand")
	  (match_operand:VI4_AVX2 2 "nonimmediate_operand")))]
  "TARGET_SSE2"
{
  if (TARGET_SSE4_1)
    {
      if (CONSTANT_P (operands[2]))
	operands[2] = validize_mem (force_const_mem (<MODE>mode, operands[2]));
      ix86_fixup_binary_operands_no_copy (MULT, <MODE>mode, operands);
    }
  else
    {
      ix86_expand_sse2_mulv4si3 (operands[0], operands[1], operands[2]);
      DONE;
    }
})

(define_insn "*<sse4_1_avx2>_mul<mode>3"
  [(set (match_operand:VI4_AVX2 0 "register_operand" "=x,x")
	(mult:VI4_AVX2
	  (match_operand:VI4_AVX2 1 "nonimmediate_operand" "%0,x")
	  (match_operand:VI4_AVX2 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE4_1 && ix86_binary_operator_ok (MULT, <MODE>mode, operands)"
  "@
   pmulld\t{%2, %0|%0, %2}
   vpmulld\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseimul")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "vector,vector")
   (set_attr "mode" "<sseinsnmode>")])

(define_expand "mul<mode>3"
  [(set (match_operand:VI8_AVX2 0 "register_operand")
	(mult:VI8_AVX2 (match_operand:VI8_AVX2 1 "register_operand")
		       (match_operand:VI8_AVX2 2 "register_operand")))]
  "TARGET_SSE2"
{
  ix86_expand_sse2_mulvxdi3 (operands[0], operands[1], operands[2]);
  DONE;
})

(define_expand "vec_widen_<s>mult_hi_<mode>"
  [(match_operand:<sseunpackmode> 0 "register_operand")
   (any_extend:<sseunpackmode>
     (match_operand:VI124_AVX2 1 "register_operand"))
   (match_operand:VI124_AVX2 2 "register_operand")]
  "TARGET_SSE2"
{
  ix86_expand_mul_widen_hilo (operands[0], operands[1], operands[2],
			      <u_bool>, true);
  DONE;
})

(define_expand "vec_widen_<s>mult_lo_<mode>"
  [(match_operand:<sseunpackmode> 0 "register_operand")
   (any_extend:<sseunpackmode>
     (match_operand:VI124_AVX2 1 "register_operand"))
   (match_operand:VI124_AVX2 2 "register_operand")]
  "TARGET_SSE2"
{
  ix86_expand_mul_widen_hilo (operands[0], operands[1], operands[2],
			      <u_bool>, false);
  DONE;
})

;; Most widen_<s>mult_even_<mode> can be handled directly from other
;; named patterns, but signed V4SI needs special help for plain SSE2.
(define_expand "vec_widen_smult_even_v4si"
  [(match_operand:V2DI 0 "register_operand")
   (match_operand:V4SI 1 "register_operand")
   (match_operand:V4SI 2 "register_operand")]
  "TARGET_SSE2"
{
  ix86_expand_mul_widen_evenodd (operands[0], operands[1], operands[2],
				 false, false);
  DONE;
})

(define_expand "vec_widen_<s>mult_odd_<mode>"
  [(match_operand:<sseunpackmode> 0 "register_operand")
   (any_extend:<sseunpackmode>
     (match_operand:VI4_AVX2 1 "register_operand"))
   (match_operand:VI4_AVX2 2 "register_operand")]
  "TARGET_SSE2"
{
  ix86_expand_mul_widen_evenodd (operands[0], operands[1], operands[2],
				 <u_bool>, true);
  DONE;
})

(define_expand "sdot_prod<mode>"
  [(match_operand:<sseunpackmode> 0 "register_operand")
   (match_operand:VI2_AVX2 1 "register_operand")
   (match_operand:VI2_AVX2 2 "register_operand")
   (match_operand:<sseunpackmode> 3 "register_operand")]
  "TARGET_SSE2"
{
  rtx t = gen_reg_rtx (<sseunpackmode>mode);
  emit_insn (gen_<sse2_avx2>_pmaddwd (t, operands[1], operands[2]));
  emit_insn (gen_rtx_SET (VOIDmode, operands[0],
			  gen_rtx_PLUS (<sseunpackmode>mode,
					operands[3], t)));
  DONE;
})

;; Normally we use widen_mul_even/odd, but combine can't quite get it all
;; back together when madd is available.
(define_expand "sdot_prodv4si"
  [(match_operand:V2DI 0 "register_operand")
   (match_operand:V4SI 1 "register_operand")
   (match_operand:V4SI 2 "register_operand")
   (match_operand:V2DI 3 "register_operand")]
  "TARGET_XOP"
{
  rtx t = gen_reg_rtx (V2DImode);
  emit_insn (gen_xop_pmacsdqh (t, operands[1], operands[2], operands[3]));
  emit_insn (gen_xop_pmacsdql (operands[0], operands[1], operands[2], t));
  DONE;
})

(define_insn "ashr<mode>3"
  [(set (match_operand:VI24_AVX2 0 "register_operand" "=x,x")
	(ashiftrt:VI24_AVX2
	  (match_operand:VI24_AVX2 1 "register_operand" "0,x")
	  (match_operand:SI 2 "nonmemory_operand" "xN,xN")))]
  "TARGET_SSE2"
  "@
   psra<ssemodesuffix>\t{%2, %0|%0, %2}
   vpsra<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseishft")
   (set (attr "length_immediate")
     (if_then_else (match_operand 2 "const_int_operand")
       (const_string "1")
       (const_string "0")))
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "<shift_insn><mode>3"
  [(set (match_operand:VI248_AVX2 0 "register_operand" "=x,x")
	(any_lshift:VI248_AVX2
	  (match_operand:VI248_AVX2 1 "register_operand" "0,x")
	  (match_operand:SI 2 "nonmemory_operand" "xN,xN")))]
  "TARGET_SSE2"
  "@
   p<vshift><ssemodesuffix>\t{%2, %0|%0, %2}
   vp<vshift><ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseishft")
   (set (attr "length_immediate")
     (if_then_else (match_operand 2 "const_int_operand")
       (const_string "1")
       (const_string "0")))
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_expand "vec_shl_<mode>"
  [(set (match_operand:VI_128 0 "register_operand")
	(ashift:V1TI
	 (match_operand:VI_128 1 "register_operand")
	 (match_operand:SI 2 "const_0_to_255_mul_8_operand")))]
  "TARGET_SSE2"
{
  operands[0] = gen_lowpart (V1TImode, operands[0]);
  operands[1] = gen_lowpart (V1TImode, operands[1]);
})

(define_insn "<sse2_avx2>_ashl<mode>3"
  [(set (match_operand:VIMAX_AVX2 0 "register_operand" "=x,x")
	(ashift:VIMAX_AVX2
	 (match_operand:VIMAX_AVX2 1 "register_operand" "0,x")
	 (match_operand:SI 2 "const_0_to_255_mul_8_operand" "n,n")))]
  "TARGET_SSE2"
{
  operands[2] = GEN_INT (INTVAL (operands[2]) / 8);

  switch (which_alternative)
    {
    case 0:
      return "pslldq\t{%2, %0|%0, %2}";
    case 1:
      return "vpslldq\t{%2, %1, %0|%0, %1, %2}";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseishft")
   (set_attr "length_immediate" "1")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_expand "vec_shr_<mode>"
  [(set (match_operand:VI_128 0 "register_operand")
	(lshiftrt:V1TI
	 (match_operand:VI_128 1 "register_operand")
	 (match_operand:SI 2 "const_0_to_255_mul_8_operand")))]
  "TARGET_SSE2"
{
  operands[0] = gen_lowpart (V1TImode, operands[0]);
  operands[1] = gen_lowpart (V1TImode, operands[1]);
})

(define_insn "<sse2_avx2>_lshr<mode>3"
  [(set (match_operand:VIMAX_AVX2 0 "register_operand" "=x,x")
	(lshiftrt:VIMAX_AVX2
	 (match_operand:VIMAX_AVX2 1 "register_operand" "0,x")
	 (match_operand:SI 2 "const_0_to_255_mul_8_operand" "n,n")))]
  "TARGET_SSE2"
{
  operands[2] = GEN_INT (INTVAL (operands[2]) / 8);

  switch (which_alternative)
    {
    case 0:
      return "psrldq\t{%2, %0|%0, %2}";
    case 1:
      return "vpsrldq\t{%2, %1, %0|%0, %1, %2}";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseishft")
   (set_attr "length_immediate" "1")
   (set_attr "atom_unit" "sishuf")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])


(define_expand "<code><mode>3"
  [(set (match_operand:VI124_256 0 "register_operand")
	(maxmin:VI124_256
	  (match_operand:VI124_256 1 "nonimmediate_operand")
	  (match_operand:VI124_256 2 "nonimmediate_operand")))]
  "TARGET_AVX2"
  "ix86_fixup_binary_operands_no_copy (<CODE>, <MODE>mode, operands);")

(define_insn "*avx2_<code><mode>3"
  [(set (match_operand:VI124_256 0 "register_operand" "=x")
	(maxmin:VI124_256
	  (match_operand:VI124_256 1 "nonimmediate_operand" "%x")
	  (match_operand:VI124_256 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX2 && ix86_binary_operator_ok (<CODE>, <MODE>mode, operands)"
  "vp<maxmin_int><ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseiadd")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_expand "<code><mode>3"
  [(set (match_operand:VI8_AVX2 0 "register_operand")
	(maxmin:VI8_AVX2
	  (match_operand:VI8_AVX2 1 "register_operand")
	  (match_operand:VI8_AVX2 2 "register_operand")))]
  "TARGET_SSE4_2"
{
  enum rtx_code code;
  rtx xops[6];
  bool ok;

  xops[0] = operands[0];

  if (<CODE> == SMAX || <CODE> == UMAX)
    {
      xops[1] = operands[1];
      xops[2] = operands[2];
    }
  else
    {
      xops[1] = operands[2];
      xops[2] = operands[1];
    }

  code = (<CODE> == UMAX || <CODE> == UMIN) ? GTU : GT;

  xops[3] = gen_rtx_fmt_ee (code, VOIDmode, operands[1], operands[2]);
  xops[4] = operands[1];
  xops[5] = operands[2];

  ok = ix86_expand_int_vcond (xops);
  gcc_assert (ok);
  DONE;
})

(define_expand "<code><mode>3"
  [(set (match_operand:VI124_128 0 "register_operand")
	(smaxmin:VI124_128
	  (match_operand:VI124_128 1 "nonimmediate_operand")
	  (match_operand:VI124_128 2 "nonimmediate_operand")))]
  "TARGET_SSE2"
{
  if (TARGET_SSE4_1 || <MODE>mode == V8HImode)
    ix86_fixup_binary_operands_no_copy (<CODE>, <MODE>mode, operands);
  else
    {
      rtx xops[6];
      bool ok;

      xops[0] = operands[0];
      operands[1] = force_reg (<MODE>mode, operands[1]);
      operands[2] = force_reg (<MODE>mode, operands[2]);

      if (<CODE> == SMAX)
	{
	  xops[1] = operands[1];
	  xops[2] = operands[2];
	}
      else
	{
	  xops[1] = operands[2];
	  xops[2] = operands[1];
	}

      xops[3] = gen_rtx_GT (VOIDmode, operands[1], operands[2]);
      xops[4] = operands[1];
      xops[5] = operands[2];

      ok = ix86_expand_int_vcond (xops);
      gcc_assert (ok);
      DONE;
    }
})

(define_insn "*sse4_1_<code><mode>3"
  [(set (match_operand:VI14_128 0 "register_operand" "=x,x")
	(smaxmin:VI14_128
	  (match_operand:VI14_128 1 "nonimmediate_operand" "%0,x")
	  (match_operand:VI14_128 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE4_1 && ix86_binary_operator_ok (<CODE>, <MODE>mode, operands)"
  "@
   p<maxmin_int><ssemodesuffix>\t{%2, %0|%0, %2}
   vp<maxmin_int><ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "prefix_extra" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "*<code>v8hi3"
  [(set (match_operand:V8HI 0 "register_operand" "=x,x")
	(smaxmin:V8HI
	  (match_operand:V8HI 1 "nonimmediate_operand" "%0,x")
	  (match_operand:V8HI 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE2 && ix86_binary_operator_ok (<CODE>, V8HImode, operands)"
  "@
   p<maxmin_int>w\t{%2, %0|%0, %2}
   vp<maxmin_int>w\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "*,1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_expand "<code><mode>3"
  [(set (match_operand:VI124_128 0 "register_operand")
	(umaxmin:VI124_128
	  (match_operand:VI124_128 1 "nonimmediate_operand")
	  (match_operand:VI124_128 2 "nonimmediate_operand")))]
  "TARGET_SSE2"
{
  if (TARGET_SSE4_1 || <MODE>mode == V16QImode)
    ix86_fixup_binary_operands_no_copy (<CODE>, <MODE>mode, operands);
  else if (<CODE> == UMAX && <MODE>mode == V8HImode)
    {
      rtx op0 = operands[0], op2 = operands[2], op3 = op0;
      operands[1] = force_reg (<MODE>mode, operands[1]);
      if (rtx_equal_p (op3, op2))
	op3 = gen_reg_rtx (V8HImode);
      emit_insn (gen_sse2_ussubv8hi3 (op3, operands[1], op2));
      emit_insn (gen_addv8hi3 (op0, op3, op2));
      DONE;
    }
  else
    {
      rtx xops[6];
      bool ok;

      operands[1] = force_reg (<MODE>mode, operands[1]);
      operands[2] = force_reg (<MODE>mode, operands[2]);

      xops[0] = operands[0];

      if (<CODE> == UMAX)
	{
	  xops[1] = operands[1];
	  xops[2] = operands[2];
	}
      else
	{
	  xops[1] = operands[2];
	  xops[2] = operands[1];
	}

      xops[3] = gen_rtx_GTU (VOIDmode, operands[1], operands[2]);
      xops[4] = operands[1];
      xops[5] = operands[2];

      ok = ix86_expand_int_vcond (xops);
      gcc_assert (ok);
      DONE;
    }
})

(define_insn "*sse4_1_<code><mode>3"
  [(set (match_operand:VI24_128 0 "register_operand" "=x,x")
	(umaxmin:VI24_128
	  (match_operand:VI24_128 1 "nonimmediate_operand" "%0,x")
	  (match_operand:VI24_128 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE4_1 && ix86_binary_operator_ok (<CODE>, <MODE>mode, operands)"
  "@
   p<maxmin_int><ssemodesuffix>\t{%2, %0|%0, %2}
   vp<maxmin_int><ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "prefix_extra" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "*<code>v16qi3"
  [(set (match_operand:V16QI 0 "register_operand" "=x,x")
	(umaxmin:V16QI
	  (match_operand:V16QI 1 "nonimmediate_operand" "%0,x")
	  (match_operand:V16QI 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE2 && ix86_binary_operator_ok (<CODE>, V16QImode, operands)"
  "@
   p<maxmin_int>b\t{%2, %0|%0, %2}
   vp<maxmin_int>b\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "*,1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel integral comparisons
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_expand "avx2_eq<mode>3"
  [(set (match_operand:VI_256 0 "register_operand")
	(eq:VI_256
	  (match_operand:VI_256 1 "nonimmediate_operand")
	  (match_operand:VI_256 2 "nonimmediate_operand")))]
  "TARGET_AVX2"
  "ix86_fixup_binary_operands_no_copy (EQ, <MODE>mode, operands);")

(define_insn "*avx2_eq<mode>3"
  [(set (match_operand:VI_256 0 "register_operand" "=x")
	(eq:VI_256
	  (match_operand:VI_256 1 "nonimmediate_operand" "%x")
	  (match_operand:VI_256 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX2 && ix86_binary_operator_ok (EQ, <MODE>mode, operands)"
  "vpcmpeq<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "ssecmp")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "*sse4_1_eqv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(eq:V2DI
	  (match_operand:V2DI 1 "nonimmediate_operand" "%0,x")
	  (match_operand:V2DI 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE4_1 && ix86_binary_operator_ok (EQ, V2DImode, operands)"
  "@
   pcmpeqq\t{%2, %0|%0, %2}
   vpcmpeqq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssecmp")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "*sse2_eq<mode>3"
  [(set (match_operand:VI124_128 0 "register_operand" "=x,x")
	(eq:VI124_128
	  (match_operand:VI124_128 1 "nonimmediate_operand" "%0,x")
	  (match_operand:VI124_128 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE2 && !TARGET_XOP
   && ix86_binary_operator_ok (EQ, <MODE>mode, operands)"
  "@
   pcmpeq<ssemodesuffix>\t{%2, %0|%0, %2}
   vpcmpeq<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssecmp")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_expand "sse2_eq<mode>3"
  [(set (match_operand:VI124_128 0 "register_operand")
	(eq:VI124_128
	  (match_operand:VI124_128 1 "nonimmediate_operand")
	  (match_operand:VI124_128 2 "nonimmediate_operand")))]
  "TARGET_SSE2 && !TARGET_XOP "
  "ix86_fixup_binary_operands_no_copy (EQ, <MODE>mode, operands);")

(define_expand "sse4_1_eqv2di3"
  [(set (match_operand:V2DI 0 "register_operand")
	(eq:V2DI
	  (match_operand:V2DI 1 "nonimmediate_operand")
	  (match_operand:V2DI 2 "nonimmediate_operand")))]
  "TARGET_SSE4_1"
  "ix86_fixup_binary_operands_no_copy (EQ, V2DImode, operands);")

(define_insn "sse4_2_gtv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(gt:V2DI
	  (match_operand:V2DI 1 "register_operand" "0,x")
	  (match_operand:V2DI 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE4_2"
  "@
   pcmpgtq\t{%2, %0|%0, %2}
   vpcmpgtq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssecmp")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_gt<mode>3"
  [(set (match_operand:VI_256 0 "register_operand" "=x")
	(gt:VI_256
	  (match_operand:VI_256 1 "register_operand" "x")
	  (match_operand:VI_256 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX2"
  "vpcmpgt<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "ssecmp")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "sse2_gt<mode>3"
  [(set (match_operand:VI124_128 0 "register_operand" "=x,x")
	(gt:VI124_128
	  (match_operand:VI124_128 1 "register_operand" "0,x")
	  (match_operand:VI124_128 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE2 && !TARGET_XOP"
  "@
   pcmpgt<ssemodesuffix>\t{%2, %0|%0, %2}
   vpcmpgt<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssecmp")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_expand "vcond<V_256:mode><VI_256:mode>"
  [(set (match_operand:V_256 0 "register_operand")
	(if_then_else:V_256
	  (match_operator 3 ""
	    [(match_operand:VI_256 4 "nonimmediate_operand")
	     (match_operand:VI_256 5 "general_operand")])
	  (match_operand:V_256 1)
	  (match_operand:V_256 2)))]
  "TARGET_AVX2
   && (GET_MODE_NUNITS (<V_256:MODE>mode)
       == GET_MODE_NUNITS (<VI_256:MODE>mode))"
{
  bool ok = ix86_expand_int_vcond (operands);
  gcc_assert (ok);
  DONE;
})

(define_expand "vcond<V_128:mode><VI124_128:mode>"
  [(set (match_operand:V_128 0 "register_operand")
	(if_then_else:V_128
	  (match_operator 3 ""
	    [(match_operand:VI124_128 4 "nonimmediate_operand")
	     (match_operand:VI124_128 5 "general_operand")])
	  (match_operand:V_128 1)
	  (match_operand:V_128 2)))]
  "TARGET_SSE2
   && (GET_MODE_NUNITS (<V_128:MODE>mode)
       == GET_MODE_NUNITS (<VI124_128:MODE>mode))"
{
  bool ok = ix86_expand_int_vcond (operands);
  gcc_assert (ok);
  DONE;
})

(define_expand "vcond<VI8F_128:mode>v2di"
  [(set (match_operand:VI8F_128 0 "register_operand")
	(if_then_else:VI8F_128
	  (match_operator 3 ""
	    [(match_operand:V2DI 4 "nonimmediate_operand")
	     (match_operand:V2DI 5 "general_operand")])
	  (match_operand:VI8F_128 1)
	  (match_operand:VI8F_128 2)))]
  "TARGET_SSE4_2"
{
  bool ok = ix86_expand_int_vcond (operands);
  gcc_assert (ok);
  DONE;
})

(define_expand "vcondu<V_256:mode><VI_256:mode>"
  [(set (match_operand:V_256 0 "register_operand")
	(if_then_else:V_256
	  (match_operator 3 ""
	    [(match_operand:VI_256 4 "nonimmediate_operand")
	     (match_operand:VI_256 5 "nonimmediate_operand")])
	  (match_operand:V_256 1 "general_operand")
	  (match_operand:V_256 2 "general_operand")))]
  "TARGET_AVX2
   && (GET_MODE_NUNITS (<V_256:MODE>mode)
       == GET_MODE_NUNITS (<VI_256:MODE>mode))"
{
  bool ok = ix86_expand_int_vcond (operands);
  gcc_assert (ok);
  DONE;
})

(define_expand "vcondu<V_128:mode><VI124_128:mode>"
  [(set (match_operand:V_128 0 "register_operand")
	(if_then_else:V_128
	  (match_operator 3 ""
	    [(match_operand:VI124_128 4 "nonimmediate_operand")
	     (match_operand:VI124_128 5 "nonimmediate_operand")])
	  (match_operand:V_128 1 "general_operand")
	  (match_operand:V_128 2 "general_operand")))]
  "TARGET_SSE2
   && (GET_MODE_NUNITS (<V_128:MODE>mode)
       == GET_MODE_NUNITS (<VI124_128:MODE>mode))"
{
  bool ok = ix86_expand_int_vcond (operands);
  gcc_assert (ok);
  DONE;
})

(define_expand "vcondu<VI8F_128:mode>v2di"
  [(set (match_operand:VI8F_128 0 "register_operand")
	(if_then_else:VI8F_128
	  (match_operator 3 ""
	    [(match_operand:V2DI 4 "nonimmediate_operand")
	     (match_operand:V2DI 5 "nonimmediate_operand")])
	  (match_operand:VI8F_128 1 "general_operand")
	  (match_operand:VI8F_128 2 "general_operand")))]
  "TARGET_SSE4_2"
{
  bool ok = ix86_expand_int_vcond (operands);
  gcc_assert (ok);
  DONE;
})

(define_mode_iterator VEC_PERM_AVX2
  [V16QI V8HI V4SI V2DI V4SF V2DF
   (V32QI "TARGET_AVX2") (V16HI "TARGET_AVX2")
   (V8SI "TARGET_AVX2") (V4DI "TARGET_AVX2")
   (V8SF "TARGET_AVX2") (V4DF "TARGET_AVX2")])

(define_expand "vec_perm<mode>"
  [(match_operand:VEC_PERM_AVX2 0 "register_operand")
   (match_operand:VEC_PERM_AVX2 1 "register_operand")
   (match_operand:VEC_PERM_AVX2 2 "register_operand")
   (match_operand:<sseintvecmode> 3 "register_operand")]
  "TARGET_SSSE3 || TARGET_AVX || TARGET_XOP"
{
  ix86_expand_vec_perm (operands);
  DONE;
})

(define_mode_iterator VEC_PERM_CONST
  [(V4SF "TARGET_SSE") (V4SI "TARGET_SSE")
   (V2DF "TARGET_SSE") (V2DI "TARGET_SSE")
   (V16QI "TARGET_SSE2") (V8HI "TARGET_SSE2")
   (V8SF "TARGET_AVX") (V4DF "TARGET_AVX")
   (V8SI "TARGET_AVX") (V4DI "TARGET_AVX")
   (V32QI "TARGET_AVX2") (V16HI "TARGET_AVX2")])

(define_expand "vec_perm_const<mode>"
  [(match_operand:VEC_PERM_CONST 0 "register_operand")
   (match_operand:VEC_PERM_CONST 1 "register_operand")
   (match_operand:VEC_PERM_CONST 2 "register_operand")
   (match_operand:<sseintvecmode> 3)]
  ""
{
  if (ix86_expand_vec_perm_const (operands))
    DONE;
  else
    FAIL;
})

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel bitwise logical operations
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_expand "one_cmpl<mode>2"
  [(set (match_operand:VI 0 "register_operand")
	(xor:VI (match_operand:VI 1 "nonimmediate_operand")
		(match_dup 2)))]
  "TARGET_SSE"
{
  int i, n = GET_MODE_NUNITS (<MODE>mode);
  rtvec v = rtvec_alloc (n);

  for (i = 0; i < n; ++i)
    RTVEC_ELT (v, i) = constm1_rtx;

  operands[2] = force_reg (<MODE>mode, gen_rtx_CONST_VECTOR (<MODE>mode, v));
})

(define_expand "<sse2_avx2>_andnot<mode>3"
  [(set (match_operand:VI_AVX2 0 "register_operand")
	(and:VI_AVX2
	  (not:VI_AVX2 (match_operand:VI_AVX2 1 "register_operand"))
	  (match_operand:VI_AVX2 2 "nonimmediate_operand")))]
  "TARGET_SSE2")

(define_insn "*andnot<mode>3"
  [(set (match_operand:VI 0 "register_operand" "=x,x")
	(and:VI
	  (not:VI (match_operand:VI 1 "register_operand" "0,x"))
	  (match_operand:VI 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE"
{
  static char buf[32];
  const char *ops;
  const char *tmp;

  switch (get_attr_mode (insn))
    {
    case MODE_OI:
      gcc_assert (TARGET_AVX2);
    case MODE_TI:
      gcc_assert (TARGET_SSE2);

      tmp = "pandn";
      break;

   case MODE_V8SF:
      gcc_assert (TARGET_AVX);
   case MODE_V4SF:
      gcc_assert (TARGET_SSE);

      tmp = "andnps";
      break;

   default:
      gcc_unreachable ();
   }

  switch (which_alternative)
    {
    case 0:
      ops = "%s\t{%%2, %%0|%%0, %%2}";
      break;
    case 1:
      ops = "v%s\t{%%2, %%1, %%0|%%0, %%1, %%2}";
      break;
    default:
      gcc_unreachable ();
    }

  snprintf (buf, sizeof (buf), ops, tmp);
  return buf;
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set (attr "prefix_data16")
     (if_then_else
       (and (eq_attr "alternative" "0")
	    (eq_attr "mode" "TI"))
       (const_string "1")
       (const_string "*")))
   (set_attr "prefix" "orig,vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "<ssePSmode>")
	       (match_test "TARGET_AVX2")
		 (const_string "<sseinsnmode>")
	       (match_test "TARGET_AVX")
		 (if_then_else
		   (match_test "GET_MODE_SIZE (<MODE>mode) > 16")
		   (const_string "V8SF")
		   (const_string "<sseinsnmode>"))
	       (ior (not (match_test "TARGET_SSE2"))
		    (match_test "optimize_function_for_size_p (cfun)"))
		 (const_string "V4SF")
	      ]
	      (const_string "<sseinsnmode>")))])

(define_expand "<code><mode>3"
  [(set (match_operand:VI 0 "register_operand")
	(any_logic:VI
	  (match_operand:VI 1 "nonimmediate_or_const_vector_operand")
	  (match_operand:VI 2 "nonimmediate_or_const_vector_operand")))]
  "TARGET_SSE"
{
  ix86_expand_vector_logical_operator (<CODE>, <MODE>mode, operands);
  DONE;
})

(define_insn "*<code><mode>3"
  [(set (match_operand:VI 0 "register_operand" "=x,x")
	(any_logic:VI
	  (match_operand:VI 1 "nonimmediate_operand" "%0,x")
	  (match_operand:VI 2 "nonimmediate_operand" "xm,xm")))]
  "TARGET_SSE
   && ix86_binary_operator_ok (<CODE>, <MODE>mode, operands)"
{
  static char buf[32];
  const char *ops;
  const char *tmp;

  switch (get_attr_mode (insn))
    {
    case MODE_OI:
      gcc_assert (TARGET_AVX2);
    case MODE_TI:
      gcc_assert (TARGET_SSE2);

      tmp = "p<logic>";
      break;

   case MODE_V8SF:
      gcc_assert (TARGET_AVX);
   case MODE_V4SF:
      gcc_assert (TARGET_SSE);

      tmp = "<logic>ps";
      break;

   default:
      gcc_unreachable ();
   }

  switch (which_alternative)
    {
    case 0:
      ops = "%s\t{%%2, %%0|%%0, %%2}";
      break;
    case 1:
      ops = "v%s\t{%%2, %%1, %%0|%%0, %%1, %%2}";
      break;
    default:
      gcc_unreachable ();
    }

  snprintf (buf, sizeof (buf), ops, tmp);
  return buf;
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set (attr "prefix_data16")
     (if_then_else
       (and (eq_attr "alternative" "0")
	    (eq_attr "mode" "TI"))
       (const_string "1")
       (const_string "*")))
   (set_attr "prefix" "orig,vex")
   (set (attr "mode")
	(cond [(match_test "TARGET_SSE_PACKED_SINGLE_INSN_OPTIMAL")
		 (const_string "<ssePSmode>")
	       (match_test "TARGET_AVX2")
		 (const_string "<sseinsnmode>")
	       (match_test "TARGET_AVX")
		 (if_then_else
		   (match_test "GET_MODE_SIZE (<MODE>mode) > 16")
		   (const_string "V8SF")
		   (const_string "<sseinsnmode>"))
	       (ior (not (match_test "TARGET_SSE2"))
		    (match_test "optimize_function_for_size_p (cfun)"))
		 (const_string "V4SF")
	      ]
	      (const_string "<sseinsnmode>")))])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Parallel integral element swizzling
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_expand "vec_pack_trunc_<mode>"
  [(match_operand:<ssepackmode> 0 "register_operand")
   (match_operand:VI248_AVX2 1 "register_operand")
   (match_operand:VI248_AVX2 2 "register_operand")]
  "TARGET_SSE2"
{
  rtx op1 = gen_lowpart (<ssepackmode>mode, operands[1]);
  rtx op2 = gen_lowpart (<ssepackmode>mode, operands[2]);
  ix86_expand_vec_extract_even_odd (operands[0], op1, op2, 0);
  DONE;
})

(define_insn "<sse2_avx2>_packsswb"
  [(set (match_operand:VI1_AVX2 0 "register_operand" "=x,x")
	(vec_concat:VI1_AVX2
	  (ss_truncate:<ssehalfvecmode>
	    (match_operand:<sseunpackmode> 1 "register_operand" "0,x"))
	  (ss_truncate:<ssehalfvecmode>
	    (match_operand:<sseunpackmode> 2 "nonimmediate_operand" "xm,xm"))))]
  "TARGET_SSE2"
  "@
   packsswb\t{%2, %0|%0, %2}
   vpacksswb\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "<sse2_avx2>_packssdw"
  [(set (match_operand:VI2_AVX2 0 "register_operand" "=x,x")
	(vec_concat:VI2_AVX2
	  (ss_truncate:<ssehalfvecmode>
	    (match_operand:<sseunpackmode> 1 "register_operand" "0,x"))
	  (ss_truncate:<ssehalfvecmode>
	    (match_operand:<sseunpackmode> 2 "nonimmediate_operand" "xm,xm"))))]
  "TARGET_SSE2"
  "@
   packssdw\t{%2, %0|%0, %2}
   vpackssdw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "<sse2_avx2>_packuswb"
  [(set (match_operand:VI1_AVX2 0 "register_operand" "=x,x")
	(vec_concat:VI1_AVX2
	  (us_truncate:<ssehalfvecmode>
	    (match_operand:<sseunpackmode> 1 "register_operand" "0,x"))
	  (us_truncate:<ssehalfvecmode>
	    (match_operand:<sseunpackmode> 2 "nonimmediate_operand" "xm,xm"))))]
  "TARGET_SSE2"
  "@
   packuswb\t{%2, %0|%0, %2}
   vpackuswb\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "avx2_interleave_highv32qi"
  [(set (match_operand:V32QI 0 "register_operand" "=x")
	(vec_select:V32QI
	  (vec_concat:V64QI
	    (match_operand:V32QI 1 "register_operand" "x")
	    (match_operand:V32QI 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 8)  (const_int 40)
		     (const_int 9)  (const_int 41)
		     (const_int 10) (const_int 42)
		     (const_int 11) (const_int 43)
		     (const_int 12) (const_int 44)
		     (const_int 13) (const_int 45)
		     (const_int 14) (const_int 46)
		     (const_int 15) (const_int 47)
		     (const_int 24) (const_int 56)
		     (const_int 25) (const_int 57)
		     (const_int 26) (const_int 58)
		     (const_int 27) (const_int 59)
		     (const_int 28) (const_int 60)
		     (const_int 29) (const_int 61)
		     (const_int 30) (const_int 62)
		     (const_int 31) (const_int 63)])))]
  "TARGET_AVX2"
  "vpunpckhbw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_interleave_highv16qi"
  [(set (match_operand:V16QI 0 "register_operand" "=x,x")
	(vec_select:V16QI
	  (vec_concat:V32QI
	    (match_operand:V16QI 1 "register_operand" "0,x")
	    (match_operand:V16QI 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 8)  (const_int 24)
		     (const_int 9)  (const_int 25)
		     (const_int 10) (const_int 26)
		     (const_int 11) (const_int 27)
		     (const_int 12) (const_int 28)
		     (const_int 13) (const_int 29)
		     (const_int 14) (const_int 30)
		     (const_int 15) (const_int 31)])))]
  "TARGET_SSE2"
  "@
   punpckhbw\t{%2, %0|%0, %2}
   vpunpckhbw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_interleave_lowv32qi"
  [(set (match_operand:V32QI 0 "register_operand" "=x")
	(vec_select:V32QI
	  (vec_concat:V64QI
	    (match_operand:V32QI 1 "register_operand" "x")
	    (match_operand:V32QI 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0) (const_int 32)
		     (const_int 1) (const_int 33)
		     (const_int 2) (const_int 34)
		     (const_int 3) (const_int 35)
		     (const_int 4) (const_int 36)
		     (const_int 5) (const_int 37)
		     (const_int 6) (const_int 38)
		     (const_int 7) (const_int 39)
		     (const_int 16) (const_int 48)
		     (const_int 17) (const_int 49)
		     (const_int 18) (const_int 50)
		     (const_int 19) (const_int 51)
		     (const_int 20) (const_int 52)
		     (const_int 21) (const_int 53)
		     (const_int 22) (const_int 54)
		     (const_int 23) (const_int 55)])))]
  "TARGET_AVX2"
  "vpunpcklbw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_interleave_lowv16qi"
  [(set (match_operand:V16QI 0 "register_operand" "=x,x")
	(vec_select:V16QI
	  (vec_concat:V32QI
	    (match_operand:V16QI 1 "register_operand" "0,x")
	    (match_operand:V16QI 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 0) (const_int 16)
		     (const_int 1) (const_int 17)
		     (const_int 2) (const_int 18)
		     (const_int 3) (const_int 19)
		     (const_int 4) (const_int 20)
		     (const_int 5) (const_int 21)
		     (const_int 6) (const_int 22)
		     (const_int 7) (const_int 23)])))]
  "TARGET_SSE2"
  "@
   punpcklbw\t{%2, %0|%0, %2}
   vpunpcklbw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_interleave_highv16hi"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(vec_select:V16HI
	  (vec_concat:V32HI
	    (match_operand:V16HI 1 "register_operand" "x")
	    (match_operand:V16HI 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 4) (const_int 20)
		     (const_int 5) (const_int 21)
		     (const_int 6) (const_int 22)
		     (const_int 7) (const_int 23)
		     (const_int 12) (const_int 28)
		     (const_int 13) (const_int 29)
		     (const_int 14) (const_int 30)
		     (const_int 15) (const_int 31)])))]
  "TARGET_AVX2"
  "vpunpckhwd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_interleave_highv8hi"
  [(set (match_operand:V8HI 0 "register_operand" "=x,x")
	(vec_select:V8HI
	  (vec_concat:V16HI
	    (match_operand:V8HI 1 "register_operand" "0,x")
	    (match_operand:V8HI 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 4) (const_int 12)
		     (const_int 5) (const_int 13)
		     (const_int 6) (const_int 14)
		     (const_int 7) (const_int 15)])))]
  "TARGET_SSE2"
  "@
   punpckhwd\t{%2, %0|%0, %2}
   vpunpckhwd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_interleave_lowv16hi"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(vec_select:V16HI
	  (vec_concat:V32HI
	    (match_operand:V16HI 1 "register_operand" "x")
	    (match_operand:V16HI 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0) (const_int 16)
		     (const_int 1) (const_int 17)
		     (const_int 2) (const_int 18)
		     (const_int 3) (const_int 19)
		     (const_int 8) (const_int 24)
		     (const_int 9) (const_int 25)
		     (const_int 10) (const_int 26)
		     (const_int 11) (const_int 27)])))]
  "TARGET_AVX2"
  "vpunpcklwd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_interleave_lowv8hi"
  [(set (match_operand:V8HI 0 "register_operand" "=x,x")
	(vec_select:V8HI
	  (vec_concat:V16HI
	    (match_operand:V8HI 1 "register_operand" "0,x")
	    (match_operand:V8HI 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 0) (const_int 8)
		     (const_int 1) (const_int 9)
		     (const_int 2) (const_int 10)
		     (const_int 3) (const_int 11)])))]
  "TARGET_SSE2"
  "@
   punpcklwd\t{%2, %0|%0, %2}
   vpunpcklwd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_interleave_highv8si"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(vec_select:V8SI
	  (vec_concat:V16SI
	    (match_operand:V8SI 1 "register_operand" "x")
	    (match_operand:V8SI 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 2) (const_int 10)
		     (const_int 3) (const_int 11)
		     (const_int 6) (const_int 14)
		     (const_int 7) (const_int 15)])))]
  "TARGET_AVX2"
  "vpunpckhdq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_interleave_highv4si"
  [(set (match_operand:V4SI 0 "register_operand" "=x,x")
	(vec_select:V4SI
	  (vec_concat:V8SI
	    (match_operand:V4SI 1 "register_operand" "0,x")
	    (match_operand:V4SI 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 2) (const_int 6)
		     (const_int 3) (const_int 7)])))]
  "TARGET_SSE2"
  "@
   punpckhdq\t{%2, %0|%0, %2}
   vpunpckhdq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_interleave_lowv8si"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(vec_select:V8SI
	  (vec_concat:V16SI
	    (match_operand:V8SI 1 "register_operand" "x")
	    (match_operand:V8SI 2 "nonimmediate_operand" "xm"))
	  (parallel [(const_int 0) (const_int 8)
		     (const_int 1) (const_int 9)
		     (const_int 4) (const_int 12)
		     (const_int 5) (const_int 13)])))]
  "TARGET_AVX2"
  "vpunpckldq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_interleave_lowv4si"
  [(set (match_operand:V4SI 0 "register_operand" "=x,x")
	(vec_select:V4SI
	  (vec_concat:V8SI
	    (match_operand:V4SI 1 "register_operand" "0,x")
	    (match_operand:V4SI 2 "nonimmediate_operand" "xm,xm"))
	  (parallel [(const_int 0) (const_int 4)
		     (const_int 1) (const_int 5)])))]
  "TARGET_SSE2"
  "@
   punpckldq\t{%2, %0|%0, %2}
   vpunpckldq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_expand "vec_interleave_high<mode>"
  [(match_operand:VI_256 0 "register_operand" "=x")
   (match_operand:VI_256 1 "register_operand" "x")
   (match_operand:VI_256 2 "nonimmediate_operand" "xm")]
 "TARGET_AVX2"
{
  rtx t1 = gen_reg_rtx (<MODE>mode);
  rtx t2 = gen_reg_rtx (<MODE>mode);
  emit_insn (gen_avx2_interleave_low<mode> (t1, operands[1], operands[2]));
  emit_insn (gen_avx2_interleave_high<mode> (t2,  operands[1], operands[2]));
  emit_insn (gen_avx2_permv2ti
	     (gen_lowpart (V4DImode, operands[0]),
	      gen_lowpart (V4DImode, t1),
	      gen_lowpart (V4DImode, t2), GEN_INT (1 + (3 << 4))));
  DONE;
})

(define_expand "vec_interleave_low<mode>"
  [(match_operand:VI_256 0 "register_operand" "=x")
   (match_operand:VI_256 1 "register_operand" "x")
   (match_operand:VI_256 2 "nonimmediate_operand" "xm")]
 "TARGET_AVX2"
{
  rtx t1 = gen_reg_rtx (<MODE>mode);
  rtx t2 = gen_reg_rtx (<MODE>mode);
  emit_insn (gen_avx2_interleave_low<mode> (t1, operands[1], operands[2]));
  emit_insn (gen_avx2_interleave_high<mode> (t2, operands[1], operands[2]));
  emit_insn (gen_avx2_permv2ti
	     (gen_lowpart (V4DImode, operands[0]),
	      gen_lowpart (V4DImode, t1),
	      gen_lowpart (V4DImode, t2), GEN_INT (0 + (2 << 4))));
  DONE;
})

;; Modes handled by pinsr patterns.
(define_mode_iterator PINSR_MODE
  [(V16QI "TARGET_SSE4_1") V8HI
   (V4SI "TARGET_SSE4_1")
   (V2DI "TARGET_SSE4_1 && TARGET_64BIT")])

(define_mode_attr sse2p4_1
  [(V16QI "sse4_1") (V8HI "sse2")
   (V4SI "sse4_1") (V2DI "sse4_1")])

;; sse4_1_pinsrd must come before sse2_loadld since it is preferred.
(define_insn "<sse2p4_1>_pinsr<ssemodesuffix>"
  [(set (match_operand:PINSR_MODE 0 "register_operand" "=x,x,x,x")
	(vec_merge:PINSR_MODE
	  (vec_duplicate:PINSR_MODE
	    (match_operand:<ssescalarmode> 2 "nonimmediate_operand" "r,m,r,m"))
	  (match_operand:PINSR_MODE 1 "register_operand" "0,0,x,x")
	  (match_operand:SI 3 "const_int_operand")))]
  "TARGET_SSE2
   && ((unsigned) exact_log2 (INTVAL (operands[3]))
       < GET_MODE_NUNITS (<MODE>mode))"
{
  operands[3] = GEN_INT (exact_log2 (INTVAL (operands[3])));

  switch (which_alternative)
    {
    case 0:
      if (GET_MODE_SIZE (<ssescalarmode>mode) < GET_MODE_SIZE (SImode))
	return "pinsr<ssemodesuffix>\t{%3, %k2, %0|%0, %k2, %3}";
      /* FALLTHRU */
    case 1:
      return "pinsr<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}";
    case 2:
      if (GET_MODE_SIZE (<ssescalarmode>mode) < GET_MODE_SIZE (SImode))
	return "vpinsr<ssemodesuffix>\t{%3, %k2, %1, %0|%0, %1, %k2, %3}";
      /* FALLTHRU */
    case 3:
      return "vpinsr<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "isa" "noavx,noavx,avx,avx")
   (set_attr "type" "sselog")
   (set (attr "prefix_rex")
     (if_then_else
       (and (not (match_test "TARGET_AVX"))
	    (eq (const_string "<MODE>mode") (const_string "V2DImode")))
       (const_string "1")
       (const_string "*")))
   (set (attr "prefix_data16")
     (if_then_else
       (and (not (match_test "TARGET_AVX"))
	    (eq (const_string "<MODE>mode") (const_string "V8HImode")))
       (const_string "1")
       (const_string "*")))
   (set (attr "prefix_extra")
     (if_then_else
       (and (not (match_test "TARGET_AVX"))
	    (eq (const_string "<MODE>mode") (const_string "V8HImode")))
       (const_string "*")
       (const_string "1")))
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,orig,vex,vex")
   (set_attr "mode" "TI")])

(define_insn "*sse4_1_pextrb_<mode>"
  [(set (match_operand:SWI48 0 "register_operand" "=r")
	(zero_extend:SWI48
	  (vec_select:QI
	    (match_operand:V16QI 1 "register_operand" "x")
	    (parallel [(match_operand:SI 2 "const_0_to_15_operand" "n")]))))]
  "TARGET_SSE4_1"
  "%vpextrb\t{%2, %1, %k0|%k0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "*sse4_1_pextrb_memory"
  [(set (match_operand:QI 0 "memory_operand" "=m")
	(vec_select:QI
	  (match_operand:V16QI 1 "register_operand" "x")
	  (parallel [(match_operand:SI 2 "const_0_to_15_operand" "n")])))]
  "TARGET_SSE4_1"
  "%vpextrb\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "*sse2_pextrw_<mode>"
  [(set (match_operand:SWI48 0 "register_operand" "=r")
	(zero_extend:SWI48
	  (vec_select:HI
	    (match_operand:V8HI 1 "register_operand" "x")
	    (parallel [(match_operand:SI 2 "const_0_to_7_operand" "n")]))))]
  "TARGET_SSE2"
  "%vpextrw\t{%2, %1, %k0|%k0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "*sse4_1_pextrw_memory"
  [(set (match_operand:HI 0 "memory_operand" "=m")
	(vec_select:HI
	  (match_operand:V8HI 1 "register_operand" "x")
	  (parallel [(match_operand:SI 2 "const_0_to_7_operand" "n")])))]
  "TARGET_SSE4_1"
  "%vpextrw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "*sse4_1_pextrd"
  [(set (match_operand:SI 0 "nonimmediate_operand" "=rm")
	(vec_select:SI
	  (match_operand:V4SI 1 "register_operand" "x")
	  (parallel [(match_operand:SI 2 "const_0_to_3_operand" "n")])))]
  "TARGET_SSE4_1"
  "%vpextrd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "*sse4_1_pextrd_zext"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(zero_extend:DI
	  (vec_select:SI
	    (match_operand:V4SI 1 "register_operand" "x")
	    (parallel [(match_operand:SI 2 "const_0_to_3_operand" "n")]))))]
  "TARGET_64BIT && TARGET_SSE4_1"
  "%vpextrd\t{%2, %1, %k0|%k0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

;; It must come before *vec_extractv2di_1_rex64 since it is preferred.
(define_insn "*sse4_1_pextrq"
  [(set (match_operand:DI 0 "nonimmediate_operand" "=rm")
	(vec_select:DI
	  (match_operand:V2DI 1 "register_operand" "x")
	  (parallel [(match_operand:SI 2 "const_0_to_1_operand" "n")])))]
  "TARGET_SSE4_1 && TARGET_64BIT"
  "%vpextrq\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_rex" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_expand "avx2_pshufdv3"
  [(match_operand:V8SI 0 "register_operand")
   (match_operand:V8SI 1 "nonimmediate_operand")
   (match_operand:SI 2 "const_0_to_255_operand")]
  "TARGET_AVX2"
{
  int mask = INTVAL (operands[2]);
  emit_insn (gen_avx2_pshufd_1 (operands[0], operands[1],
				GEN_INT ((mask >> 0) & 3),
				GEN_INT ((mask >> 2) & 3),
				GEN_INT ((mask >> 4) & 3),
				GEN_INT ((mask >> 6) & 3),
				GEN_INT (((mask >> 0) & 3) + 4),
				GEN_INT (((mask >> 2) & 3) + 4),
				GEN_INT (((mask >> 4) & 3) + 4),
				GEN_INT (((mask >> 6) & 3) + 4)));
  DONE;
})

(define_insn "avx2_pshufd_1"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(vec_select:V8SI
	  (match_operand:V8SI 1 "nonimmediate_operand" "xm")
	  (parallel [(match_operand 2 "const_0_to_3_operand")
		     (match_operand 3 "const_0_to_3_operand")
		     (match_operand 4 "const_0_to_3_operand")
		     (match_operand 5 "const_0_to_3_operand")
		     (match_operand 6 "const_4_to_7_operand")
		     (match_operand 7 "const_4_to_7_operand")
		     (match_operand 8 "const_4_to_7_operand")
		     (match_operand 9 "const_4_to_7_operand")])))]
  "TARGET_AVX2
   && INTVAL (operands[2]) + 4 == INTVAL (operands[6])
   && INTVAL (operands[3]) + 4 == INTVAL (operands[7])
   && INTVAL (operands[4]) + 4 == INTVAL (operands[8])
   && INTVAL (operands[5]) + 4 == INTVAL (operands[9])"
{
  int mask = 0;
  mask |= INTVAL (operands[2]) << 0;
  mask |= INTVAL (operands[3]) << 2;
  mask |= INTVAL (operands[4]) << 4;
  mask |= INTVAL (operands[5]) << 6;
  operands[2] = GEN_INT (mask);

  return "vpshufd\t{%2, %1, %0|%0, %1, %2}";
}
  [(set_attr "type" "sselog1")
   (set_attr "prefix" "vex")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "OI")])

(define_expand "sse2_pshufd"
  [(match_operand:V4SI 0 "register_operand")
   (match_operand:V4SI 1 "nonimmediate_operand")
   (match_operand:SI 2 "const_int_operand")]
  "TARGET_SSE2"
{
  int mask = INTVAL (operands[2]);
  emit_insn (gen_sse2_pshufd_1 (operands[0], operands[1],
				GEN_INT ((mask >> 0) & 3),
				GEN_INT ((mask >> 2) & 3),
				GEN_INT ((mask >> 4) & 3),
				GEN_INT ((mask >> 6) & 3)));
  DONE;
})

(define_insn "sse2_pshufd_1"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(vec_select:V4SI
	  (match_operand:V4SI 1 "nonimmediate_operand" "xm")
	  (parallel [(match_operand 2 "const_0_to_3_operand")
		     (match_operand 3 "const_0_to_3_operand")
		     (match_operand 4 "const_0_to_3_operand")
		     (match_operand 5 "const_0_to_3_operand")])))]
  "TARGET_SSE2"
{
  int mask = 0;
  mask |= INTVAL (operands[2]) << 0;
  mask |= INTVAL (operands[3]) << 2;
  mask |= INTVAL (operands[4]) << 4;
  mask |= INTVAL (operands[5]) << 6;
  operands[2] = GEN_INT (mask);

  return "%vpshufd\t{%2, %1, %0|%0, %1, %2}";
}
  [(set_attr "type" "sselog1")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "TI")])

(define_expand "avx2_pshuflwv3"
  [(match_operand:V16HI 0 "register_operand")
   (match_operand:V16HI 1 "nonimmediate_operand")
   (match_operand:SI 2 "const_0_to_255_operand")]
  "TARGET_AVX2"
{
  int mask = INTVAL (operands[2]);
  emit_insn (gen_avx2_pshuflw_1 (operands[0], operands[1],
				 GEN_INT ((mask >> 0) & 3),
				 GEN_INT ((mask >> 2) & 3),
				 GEN_INT ((mask >> 4) & 3),
				 GEN_INT ((mask >> 6) & 3),
				 GEN_INT (((mask >> 0) & 3) + 8),
				 GEN_INT (((mask >> 2) & 3) + 8),
				 GEN_INT (((mask >> 4) & 3) + 8),
				 GEN_INT (((mask >> 6) & 3) + 8)));
  DONE;
})

(define_insn "avx2_pshuflw_1"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(vec_select:V16HI
	  (match_operand:V16HI 1 "nonimmediate_operand" "xm")
	  (parallel [(match_operand 2 "const_0_to_3_operand")
		     (match_operand 3 "const_0_to_3_operand")
		     (match_operand 4 "const_0_to_3_operand")
		     (match_operand 5 "const_0_to_3_operand")
		     (const_int 4)
		     (const_int 5)
		     (const_int 6)
		     (const_int 7)
		     (match_operand 6 "const_8_to_11_operand")
		     (match_operand 7 "const_8_to_11_operand")
		     (match_operand 8 "const_8_to_11_operand")
		     (match_operand 9 "const_8_to_11_operand")
		     (const_int 12)
		     (const_int 13)
		     (const_int 14)
		     (const_int 15)])))]
  "TARGET_AVX2
   && INTVAL (operands[2]) + 8 == INTVAL (operands[6])
   && INTVAL (operands[3]) + 8 == INTVAL (operands[7])
   && INTVAL (operands[4]) + 8 == INTVAL (operands[8])
   && INTVAL (operands[5]) + 8 == INTVAL (operands[9])"
{
  int mask = 0;
  mask |= INTVAL (operands[2]) << 0;
  mask |= INTVAL (operands[3]) << 2;
  mask |= INTVAL (operands[4]) << 4;
  mask |= INTVAL (operands[5]) << 6;
  operands[2] = GEN_INT (mask);

  return "vpshuflw\t{%2, %1, %0|%0, %1, %2}";
}
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "OI")])

(define_expand "sse2_pshuflw"
  [(match_operand:V8HI 0 "register_operand")
   (match_operand:V8HI 1 "nonimmediate_operand")
   (match_operand:SI 2 "const_int_operand")]
  "TARGET_SSE2"
{
  int mask = INTVAL (operands[2]);
  emit_insn (gen_sse2_pshuflw_1 (operands[0], operands[1],
				 GEN_INT ((mask >> 0) & 3),
				 GEN_INT ((mask >> 2) & 3),
				 GEN_INT ((mask >> 4) & 3),
				 GEN_INT ((mask >> 6) & 3)));
  DONE;
})

(define_insn "sse2_pshuflw_1"
  [(set (match_operand:V8HI 0 "register_operand" "=x")
	(vec_select:V8HI
	  (match_operand:V8HI 1 "nonimmediate_operand" "xm")
	  (parallel [(match_operand 2 "const_0_to_3_operand")
		     (match_operand 3 "const_0_to_3_operand")
		     (match_operand 4 "const_0_to_3_operand")
		     (match_operand 5 "const_0_to_3_operand")
		     (const_int 4)
		     (const_int 5)
		     (const_int 6)
		     (const_int 7)])))]
  "TARGET_SSE2"
{
  int mask = 0;
  mask |= INTVAL (operands[2]) << 0;
  mask |= INTVAL (operands[3]) << 2;
  mask |= INTVAL (operands[4]) << 4;
  mask |= INTVAL (operands[5]) << 6;
  operands[2] = GEN_INT (mask);

  return "%vpshuflw\t{%2, %1, %0|%0, %1, %2}";
}
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "TI")])

(define_expand "avx2_pshufhwv3"
  [(match_operand:V16HI 0 "register_operand")
   (match_operand:V16HI 1 "nonimmediate_operand")
   (match_operand:SI 2 "const_0_to_255_operand")]
  "TARGET_AVX2"
{
  int mask = INTVAL (operands[2]);
  emit_insn (gen_avx2_pshufhw_1 (operands[0], operands[1],
				 GEN_INT (((mask >> 0) & 3) + 4),
				 GEN_INT (((mask >> 2) & 3) + 4),
				 GEN_INT (((mask >> 4) & 3) + 4),
				 GEN_INT (((mask >> 6) & 3) + 4),
				 GEN_INT (((mask >> 0) & 3) + 12),
				 GEN_INT (((mask >> 2) & 3) + 12),
				 GEN_INT (((mask >> 4) & 3) + 12),
				 GEN_INT (((mask >> 6) & 3) + 12)));
  DONE;
})

(define_insn "avx2_pshufhw_1"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(vec_select:V16HI
	  (match_operand:V16HI 1 "nonimmediate_operand" "xm")
	  (parallel [(const_int 0)
		     (const_int 1)
		     (const_int 2)
		     (const_int 3)
		     (match_operand 2 "const_4_to_7_operand")
		     (match_operand 3 "const_4_to_7_operand")
		     (match_operand 4 "const_4_to_7_operand")
		     (match_operand 5 "const_4_to_7_operand")
		     (const_int 8)
		     (const_int 9)
		     (const_int 10)
		     (const_int 11)
		     (match_operand 6 "const_12_to_15_operand")
		     (match_operand 7 "const_12_to_15_operand")
		     (match_operand 8 "const_12_to_15_operand")
		     (match_operand 9 "const_12_to_15_operand")])))]
  "TARGET_AVX2
   && INTVAL (operands[2]) + 8 == INTVAL (operands[6])
   && INTVAL (operands[3]) + 8 == INTVAL (operands[7])
   && INTVAL (operands[4]) + 8 == INTVAL (operands[8])
   && INTVAL (operands[5]) + 8 == INTVAL (operands[9])"
{
  int mask = 0;
  mask |= (INTVAL (operands[2]) - 4) << 0;
  mask |= (INTVAL (operands[3]) - 4) << 2;
  mask |= (INTVAL (operands[4]) - 4) << 4;
  mask |= (INTVAL (operands[5]) - 4) << 6;
  operands[2] = GEN_INT (mask);

  return "vpshufhw\t{%2, %1, %0|%0, %1, %2}";
}
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "OI")])

(define_expand "sse2_pshufhw"
  [(match_operand:V8HI 0 "register_operand")
   (match_operand:V8HI 1 "nonimmediate_operand")
   (match_operand:SI 2 "const_int_operand")]
  "TARGET_SSE2"
{
  int mask = INTVAL (operands[2]);
  emit_insn (gen_sse2_pshufhw_1 (operands[0], operands[1],
				 GEN_INT (((mask >> 0) & 3) + 4),
				 GEN_INT (((mask >> 2) & 3) + 4),
				 GEN_INT (((mask >> 4) & 3) + 4),
				 GEN_INT (((mask >> 6) & 3) + 4)));
  DONE;
})

(define_insn "sse2_pshufhw_1"
  [(set (match_operand:V8HI 0 "register_operand" "=x")
	(vec_select:V8HI
	  (match_operand:V8HI 1 "nonimmediate_operand" "xm")
	  (parallel [(const_int 0)
		     (const_int 1)
		     (const_int 2)
		     (const_int 3)
		     (match_operand 2 "const_4_to_7_operand")
		     (match_operand 3 "const_4_to_7_operand")
		     (match_operand 4 "const_4_to_7_operand")
		     (match_operand 5 "const_4_to_7_operand")])))]
  "TARGET_SSE2"
{
  int mask = 0;
  mask |= (INTVAL (operands[2]) - 4) << 0;
  mask |= (INTVAL (operands[3]) - 4) << 2;
  mask |= (INTVAL (operands[4]) - 4) << 4;
  mask |= (INTVAL (operands[5]) - 4) << 6;
  operands[2] = GEN_INT (mask);

  return "%vpshufhw\t{%2, %1, %0|%0, %1, %2}";
}
  [(set_attr "type" "sselog")
   (set_attr "prefix_rep" "1")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix" "maybe_vex")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "TI")])

(define_expand "sse2_loadd"
  [(set (match_operand:V4SI 0 "register_operand")
	(vec_merge:V4SI
	  (vec_duplicate:V4SI
	    (match_operand:SI 1 "nonimmediate_operand"))
	  (match_dup 2)
	  (const_int 1)))]
  "TARGET_SSE"
  "operands[2] = CONST0_RTX (V4SImode);")

(define_insn "sse2_loadld"
  [(set (match_operand:V4SI 0 "register_operand"       "=x,Yi,x,x,x")
	(vec_merge:V4SI
	  (vec_duplicate:V4SI
	    (match_operand:SI 2 "nonimmediate_operand" "m ,r ,m,x,x"))
	  (match_operand:V4SI 1 "reg_or_0_operand"     "C ,C ,C,0,x")
	  (const_int 1)))]
  "TARGET_SSE"
  "@
   %vmovd\t{%2, %0|%0, %2}
   %vmovd\t{%2, %0|%0, %2}
   movss\t{%2, %0|%0, %2}
   movss\t{%2, %0|%0, %2}
   vmovss\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "sse2,*,noavx,noavx,avx")
   (set_attr "type" "ssemov")
   (set_attr "prefix" "maybe_vex,maybe_vex,orig,orig,vex")
   (set_attr "mode" "TI,TI,V4SF,SF,SF")])

(define_insn_and_split "sse2_stored"
  [(set (match_operand:SI 0 "nonimmediate_operand" "=xm,r")
	(vec_select:SI
	  (match_operand:V4SI 1 "register_operand" "x,Yi")
	  (parallel [(const_int 0)])))]
  "TARGET_SSE"
  "#"
  "&& reload_completed
   && (TARGET_INTER_UNIT_MOVES
       || MEM_P (operands [0])
       || !GENERAL_REGNO_P (true_regnum (operands [0])))"
  [(set (match_dup 0) (match_dup 1))]
  "operands[1] = gen_rtx_REG (SImode, REGNO (operands[1]));")

(define_insn_and_split "*vec_ext_v4si_mem"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(vec_select:SI
	  (match_operand:V4SI 1 "memory_operand" "o")
	  (parallel [(match_operand 2 "const_0_to_3_operand")])))]
  ""
  "#"
  "reload_completed"
  [(const_int 0)]
{
  int i = INTVAL (operands[2]);

  emit_move_insn (operands[0], adjust_address (operands[1], SImode, i*4));
  DONE;
})

(define_expand "sse_storeq"
  [(set (match_operand:DI 0 "nonimmediate_operand")
	(vec_select:DI
	  (match_operand:V2DI 1 "register_operand")
	  (parallel [(const_int 0)])))]
  "TARGET_SSE")

(define_insn "*sse2_storeq_rex64"
  [(set (match_operand:DI 0 "nonimmediate_operand" "=xm,*r,r")
	(vec_select:DI
	  (match_operand:V2DI 1 "nonimmediate_operand" "x,Yi,o")
	  (parallel [(const_int 0)])))]
  "TARGET_64BIT && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "@
   #
   #
   mov{q}\t{%1, %0|%0, %1}"
  [(set_attr "type" "*,*,imov")
   (set_attr "mode" "*,*,DI")])

(define_insn "*sse2_storeq"
  [(set (match_operand:DI 0 "nonimmediate_operand" "=xm")
	(vec_select:DI
	  (match_operand:V2DI 1 "register_operand" "x")
	  (parallel [(const_int 0)])))]
  "TARGET_SSE"
  "#")

(define_split
  [(set (match_operand:DI 0 "nonimmediate_operand")
	(vec_select:DI
	  (match_operand:V2DI 1 "register_operand")
	  (parallel [(const_int 0)])))]
  "TARGET_SSE
   && reload_completed
   && (TARGET_INTER_UNIT_MOVES
       || MEM_P (operands [0])
       || !GENERAL_REGNO_P (true_regnum (operands [0])))"
  [(set (match_dup 0) (match_dup 1))]
  "operands[1] = gen_rtx_REG (DImode, REGNO (operands[1]));")

(define_insn "*vec_extractv2di_1_rex64"
  [(set (match_operand:DI 0 "nonimmediate_operand"     "=m,x,x,x,r")
	(vec_select:DI
	  (match_operand:V2DI 1 "nonimmediate_operand" " x,0,x,o,o")
	  (parallel [(const_int 1)])))]
  "TARGET_64BIT && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "@
   %vmovhps\t{%1, %0|%0, %1}
   psrldq\t{$8, %0|%0, 8}
   vpsrldq\t{$8, %1, %0|%0, %1, 8}
   %vmovq\t{%H1, %0|%0, %H1}
   mov{q}\t{%H1, %0|%0, %H1}"
  [(set_attr "isa" "*,noavx,avx,*,*")
   (set_attr "type" "ssemov,sseishft1,sseishft1,ssemov,imov")
   (set_attr "length_immediate" "*,1,1,*,*")
   (set_attr "memory" "*,none,none,*,*")
   (set_attr "prefix" "maybe_vex,orig,vex,maybe_vex,orig")
   (set_attr "mode" "V2SF,TI,TI,TI,DI")])

(define_insn "*vec_extractv2di_1"
  [(set (match_operand:DI 0 "nonimmediate_operand"     "=m,x,x,x,x,x")
	(vec_select:DI
	  (match_operand:V2DI 1 "nonimmediate_operand" " x,0,x,o,x,o")
	  (parallel [(const_int 1)])))]
  "!TARGET_64BIT && TARGET_SSE
   && !(MEM_P (operands[0]) && MEM_P (operands[1]))"
  "@
   %vmovhps\t{%1, %0|%0, %1}
   psrldq\t{$8, %0|%0, 8}
   vpsrldq\t{$8, %1, %0|%0, %1, 8}
   %vmovq\t{%H1, %0|%0, %H1}
   movhlps\t{%1, %0|%0, %1}
   movlps\t{%H1, %0|%0, %H1}"
  [(set_attr "isa" "*,sse2_noavx,avx,sse2,noavx,noavx")
   (set_attr "type" "ssemov,sseishft1,sseishft1,ssemov,ssemov,ssemov")
   (set_attr "length_immediate" "*,1,1,*,*,*")
   (set_attr "memory" "*,none,none,*,*,*")
   (set_attr "prefix" "maybe_vex,orig,vex,maybe_vex,orig,orig")
   (set_attr "mode" "V2SF,TI,TI,TI,V4SF,V2SF")])

(define_insn "*vec_dupv4si"
  [(set (match_operand:V4SI 0 "register_operand"     "=x,x,x")
	(vec_duplicate:V4SI
	  (match_operand:SI 1 "nonimmediate_operand" " x,m,0")))]
  "TARGET_SSE"
  "@
   %vpshufd\t{$0, %1, %0|%0, %1, 0}
   vbroadcastss\t{%1, %0|%0, %1}
   shufps\t{$0, %0, %0|%0, %0, 0}"
  [(set_attr "isa" "sse2,avx,noavx")
   (set_attr "type" "sselog1,ssemov,sselog1")
   (set_attr "length_immediate" "1,0,1")
   (set_attr "prefix_extra" "0,1,*")
   (set_attr "prefix" "maybe_vex,vex,orig")
   (set_attr "mode" "TI,V4SF,V4SF")])

(define_insn "*vec_dupv2di"
  [(set (match_operand:V2DI 0 "register_operand"     "=x,x,x,x")
	(vec_duplicate:V2DI
	  (match_operand:DI 1 "nonimmediate_operand" " 0,x,m,0")))]
  "TARGET_SSE"
  "@
   punpcklqdq\t%0, %0
   vpunpcklqdq\t{%d1, %0|%0, %d1}
   %vmovddup\t{%1, %0|%0, %1}
   movlhps\t%0, %0"
  [(set_attr "isa" "sse2_noavx,avx,sse3,noavx")
   (set_attr "type" "sselog1,sselog1,sselog1,ssemov")
   (set_attr "prefix" "orig,vex,maybe_vex,orig")
   (set_attr "mode" "TI,TI,DF,V4SF")])

(define_insn "*vec_concatv2si_sse4_1"
  [(set (match_operand:V2SI 0 "register_operand"     "=x, x,x,x, x, *y,*y")
	(vec_concat:V2SI
	  (match_operand:SI 1 "nonimmediate_operand" " 0, x,0,x,rm,  0,rm")
	  (match_operand:SI 2 "vector_move_operand"  "rm,rm,x,x, C,*ym, C")))]
  "TARGET_SSE4_1"
  "@
   pinsrd\t{$1, %2, %0|%0, %2, 1}
   vpinsrd\t{$1, %2, %1, %0|%0, %1, %2, 1}
   punpckldq\t{%2, %0|%0, %2}
   vpunpckldq\t{%2, %1, %0|%0, %1, %2}
   %vmovd\t{%1, %0|%0, %1}
   punpckldq\t{%2, %0|%0, %2}
   movd\t{%1, %0|%0, %1}"
  [(set_attr "isa" "noavx,avx,noavx,avx,*,*,*")
   (set_attr "type" "sselog,sselog,sselog,sselog,ssemov,mmxcvt,mmxmov")
   (set_attr "prefix_extra" "1,1,*,*,*,*,*")
   (set_attr "length_immediate" "1,1,*,*,*,*,*")
   (set_attr "prefix" "orig,vex,orig,vex,maybe_vex,orig,orig")
   (set_attr "mode" "TI,TI,TI,TI,TI,DI,DI")])

;; ??? In theory we can match memory for the MMX alternative, but allowing
;; nonimmediate_operand for operand 2 and *not* allowing memory for the SSE
;; alternatives pretty much forces the MMX alternative to be chosen.
(define_insn "*vec_concatv2si_sse2"
  [(set (match_operand:V2SI 0 "register_operand"     "=x,x ,*y,*y")
	(vec_concat:V2SI
	  (match_operand:SI 1 "nonimmediate_operand" " 0,rm, 0,rm")
	  (match_operand:SI 2 "reg_or_0_operand"     " x,C ,*y, C")))]
  "TARGET_SSE2"
  "@
   punpckldq\t{%2, %0|%0, %2}
   movd\t{%1, %0|%0, %1}
   punpckldq\t{%2, %0|%0, %2}
   movd\t{%1, %0|%0, %1}"
  [(set_attr "type" "sselog,ssemov,mmxcvt,mmxmov")
   (set_attr "mode" "TI,TI,DI,DI")])

(define_insn "*vec_concatv2si_sse"
  [(set (match_operand:V2SI 0 "register_operand"     "=x,x,*y,*y")
	(vec_concat:V2SI
	  (match_operand:SI 1 "nonimmediate_operand" " 0,m, 0,*rm")
	  (match_operand:SI 2 "reg_or_0_operand"     " x,C,*y,C")))]
  "TARGET_SSE"
  "@
   unpcklps\t{%2, %0|%0, %2}
   movss\t{%1, %0|%0, %1}
   punpckldq\t{%2, %0|%0, %2}
   movd\t{%1, %0|%0, %1}"
  [(set_attr "type" "sselog,ssemov,mmxcvt,mmxmov")
   (set_attr "mode" "V4SF,V4SF,DI,DI")])

(define_insn "*vec_concatv4si"
  [(set (match_operand:V4SI 0 "register_operand"       "=x,x,x,x,x")
	(vec_concat:V4SI
	  (match_operand:V2SI 1 "register_operand"     " 0,x,0,0,x")
	  (match_operand:V2SI 2 "nonimmediate_operand" " x,x,x,m,m")))]
  "TARGET_SSE"
  "@
   punpcklqdq\t{%2, %0|%0, %2}
   vpunpcklqdq\t{%2, %1, %0|%0, %1, %2}
   movlhps\t{%2, %0|%0, %2}
   movhps\t{%2, %0|%0, %2}
   vmovhps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "sse2_noavx,avx,noavx,noavx,avx")
   (set_attr "type" "sselog,sselog,ssemov,ssemov,ssemov")
   (set_attr "prefix" "orig,vex,orig,orig,vex")
   (set_attr "mode" "TI,TI,V4SF,V2SF,V2SF")])

;; movd instead of movq is required to handle broken assemblers.
(define_insn "*vec_concatv2di_rex64"
  [(set (match_operand:V2DI 0 "register_operand"
	  "=x,x ,x ,Yi,!x,x,x,x,x")
	(vec_concat:V2DI
	  (match_operand:DI 1 "nonimmediate_operand"
	  " 0,x ,xm,r ,*y,0,x,0,x")
	  (match_operand:DI 2 "vector_move_operand"
	  "rm,rm,C ,C ,C ,x,x,m,m")))]
  "TARGET_64BIT"
  "@
   pinsrq\t{$1, %2, %0|%0, %2, 1}
   vpinsrq\t{$1, %2, %1, %0|%0, %1, %2, 1}
   %vmovq\t{%1, %0|%0, %1}
   %vmovd\t{%1, %0|%0, %1}
   movq2dq\t{%1, %0|%0, %1}
   punpcklqdq\t{%2, %0|%0, %2}
   vpunpcklqdq\t{%2, %1, %0|%0, %1, %2}
   movhps\t{%2, %0|%0, %2}
   vmovhps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "sse4_noavx,avx,*,*,*,noavx,avx,noavx,avx")
   (set (attr "type")
     (if_then_else
       (eq_attr "alternative" "0,1,5,6")
       (const_string "sselog")
       (const_string "ssemov")))
   (set (attr "prefix_rex")
     (if_then_else
       (and (eq_attr "alternative" "0,3")
	    (not (match_test "TARGET_AVX")))
       (const_string "1")
       (const_string "*")))
   (set_attr "prefix_extra" "1,1,*,*,*,*,*,*,*")
   (set_attr "length_immediate" "1,1,*,*,*,*,*,*,*")
   (set_attr "prefix" "orig,vex,maybe_vex,maybe_vex,orig,orig,vex,orig,vex")
   (set_attr "mode" "TI,TI,TI,TI,TI,TI,TI,V2SF,V2SF")])

(define_insn "vec_concatv2di"
  [(set (match_operand:V2DI 0 "register_operand"     "=x,?x,x,x,x,x,x")
	(vec_concat:V2DI
	  (match_operand:DI 1 "nonimmediate_operand" "xm,*y,0,x,0,0,x")
	  (match_operand:DI 2 "vector_move_operand"  " C, C,x,x,x,m,m")))]
  "!TARGET_64BIT && TARGET_SSE"
  "@
   %vmovq\t{%1, %0|%0, %1}
   movq2dq\t{%1, %0|%0, %1}
   punpcklqdq\t{%2, %0|%0, %2}
   vpunpcklqdq\t{%2, %1, %0|%0, %1, %2}
   movlhps\t{%2, %0|%0, %2}
   movhps\t{%2, %0|%0, %2}
   vmovhps\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "sse2,sse2,sse2_noavx,avx,noavx,noavx,avx")
   (set_attr "type" "ssemov,ssemov,sselog,sselog,ssemov,ssemov,ssemov")
   (set_attr "prefix" "maybe_vex,orig,orig,vex,orig,orig,vex")
   (set_attr "mode" "TI,TI,TI,TI,V4SF,V2SF,V2SF")])

(define_expand "vec_unpacks_lo_<mode>"
  [(match_operand:<sseunpackmode> 0 "register_operand")
   (match_operand:VI124_AVX2 1 "register_operand")]
  "TARGET_SSE2"
  "ix86_expand_sse_unpack (operands[0], operands[1], false, false); DONE;")

(define_expand "vec_unpacks_hi_<mode>"
  [(match_operand:<sseunpackmode> 0 "register_operand")
   (match_operand:VI124_AVX2 1 "register_operand")]
  "TARGET_SSE2"
  "ix86_expand_sse_unpack (operands[0], operands[1], false, true); DONE;")

(define_expand "vec_unpacku_lo_<mode>"
  [(match_operand:<sseunpackmode> 0 "register_operand")
   (match_operand:VI124_AVX2 1 "register_operand")]
  "TARGET_SSE2"
  "ix86_expand_sse_unpack (operands[0], operands[1], true, false); DONE;")

(define_expand "vec_unpacku_hi_<mode>"
  [(match_operand:<sseunpackmode> 0 "register_operand")
   (match_operand:VI124_AVX2 1 "register_operand")]
  "TARGET_SSE2"
  "ix86_expand_sse_unpack (operands[0], operands[1], true, true); DONE;")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Miscellaneous
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_expand "<sse2_avx2>_uavg<mode>3"
  [(set (match_operand:VI12_AVX2 0 "register_operand")
	(truncate:VI12_AVX2
	  (lshiftrt:<ssedoublemode>
	    (plus:<ssedoublemode>
	      (plus:<ssedoublemode>
		(zero_extend:<ssedoublemode>
		  (match_operand:VI12_AVX2 1 "nonimmediate_operand"))
		(zero_extend:<ssedoublemode>
		  (match_operand:VI12_AVX2 2 "nonimmediate_operand")))
	      (match_dup 3))
	    (const_int 1))))]
  "TARGET_SSE2"
{
  operands[3] = CONST1_RTX(<MODE>mode);
  ix86_fixup_binary_operands_no_copy (PLUS, <MODE>mode, operands);
})

(define_insn "*<sse2_avx2>_uavg<mode>3"
  [(set (match_operand:VI12_AVX2 0 "register_operand" "=x,x")
	(truncate:VI12_AVX2
	  (lshiftrt:<ssedoublemode>
	    (plus:<ssedoublemode>
	      (plus:<ssedoublemode>
		(zero_extend:<ssedoublemode>
		  (match_operand:VI12_AVX2 1 "nonimmediate_operand" "%0,x"))
		(zero_extend:<ssedoublemode>
		  (match_operand:VI12_AVX2 2 "nonimmediate_operand" "xm,xm")))
	      (match_operand:VI12_AVX2 3 "const1_operand"))
	    (const_int 1))))]
  "TARGET_SSE2 && ix86_binary_operator_ok (PLUS, <MODE>mode, operands)"
  "@
   pavg<ssemodesuffix>\t{%2, %0|%0, %2}
   vpavg<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

;; The correct representation for this is absolutely enormous, and
;; surely not generally useful.
(define_insn "<sse2_avx2>_psadbw"
  [(set (match_operand:VI8_AVX2 0 "register_operand" "=x,x")
	(unspec:VI8_AVX2
	  [(match_operand:<ssebytemode> 1 "register_operand" "0,x")
	   (match_operand:<ssebytemode> 2 "nonimmediate_operand" "xm,xm")]
	  UNSPEC_PSADBW))]
  "TARGET_SSE2"
  "@
   psadbw\t{%2, %0|%0, %2}
   vpsadbw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "atom_unit" "simul")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "<sse>_movmsk<ssemodesuffix><avxsizesuffix>"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(unspec:SI
	  [(match_operand:VF 1 "register_operand" "x")]
	  UNSPEC_MOVMSK))]
  "TARGET_SSE"
  "%vmovmsk<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<MODE>")])

(define_insn "avx2_pmovmskb"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(unspec:SI [(match_operand:V32QI 1 "register_operand" "x")]
		   UNSPEC_MOVMSK))]
  "TARGET_AVX2"
  "vpmovmskb\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "vex")
   (set_attr "mode" "DI")])

(define_insn "sse2_pmovmskb"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(unspec:SI [(match_operand:V16QI 1 "register_operand" "x")]
		   UNSPEC_MOVMSK))]
  "TARGET_SSE2"
  "%vpmovmskb\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "SI")])

(define_expand "sse2_maskmovdqu"
  [(set (match_operand:V16QI 0 "memory_operand")
	(unspec:V16QI [(match_operand:V16QI 1 "register_operand")
		       (match_operand:V16QI 2 "register_operand")
		       (match_dup 0)]
		      UNSPEC_MASKMOV))]
  "TARGET_SSE2")

(define_insn "*sse2_maskmovdqu"
  [(set (mem:V16QI (match_operand:P 0 "register_operand" "D"))
	(unspec:V16QI [(match_operand:V16QI 1 "register_operand" "x")
		       (match_operand:V16QI 2 "register_operand" "x")
		       (mem:V16QI (match_dup 0))]
		      UNSPEC_MASKMOV))]
  "TARGET_SSE2"
{
  /* We can't use %^ here due to ASM_OUTPUT_OPCODE processing
     that requires %v to be at the beginning of the opcode name.  */
  if (Pmode != word_mode)
    fputs ("\taddr32", asm_out_file);
  return "%vmaskmovdqu\t{%2, %1|%1, %2}";
}
  [(set_attr "type" "ssemov")
   (set_attr "prefix_data16" "1")
   (set (attr "length_address")
     (symbol_ref ("Pmode != word_mode")))
   ;; The implicit %rdi operand confuses default length_vex computation.
   (set (attr "length_vex")
     (symbol_ref ("3 + REX_SSE_REGNO_P (REGNO (operands[2]))")))
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "sse_ldmxcsr"
  [(unspec_volatile [(match_operand:SI 0 "memory_operand" "m")]
		    UNSPECV_LDMXCSR)]
  "TARGET_SSE"
  "%vldmxcsr\t%0"
  [(set_attr "type" "sse")
   (set_attr "atom_sse_attr" "mxcsr")
   (set_attr "prefix" "maybe_vex")
   (set_attr "memory" "load")])

(define_insn "sse_stmxcsr"
  [(set (match_operand:SI 0 "memory_operand" "=m")
	(unspec_volatile:SI [(const_int 0)] UNSPECV_STMXCSR))]
  "TARGET_SSE"
  "%vstmxcsr\t%0"
  [(set_attr "type" "sse")
   (set_attr "atom_sse_attr" "mxcsr")
   (set_attr "prefix" "maybe_vex")
   (set_attr "memory" "store")])

(define_insn "sse2_clflush"
  [(unspec_volatile [(match_operand 0 "address_operand" "p")]
		    UNSPECV_CLFLUSH)]
  "TARGET_SSE2"
  "clflush\t%a0"
  [(set_attr "type" "sse")
   (set_attr "atom_sse_attr" "fence")
   (set_attr "memory" "unknown")])


(define_insn "sse3_mwait"
  [(unspec_volatile [(match_operand:SI 0 "register_operand" "a")
		     (match_operand:SI 1 "register_operand" "c")]
		    UNSPECV_MWAIT)]
  "TARGET_SSE3"
;; 64bit version is "mwait %rax,%rcx". But only lower 32bits are used.
;; Since 32bit register operands are implicitly zero extended to 64bit,
;; we only need to set up 32bit registers.
  "mwait"
  [(set_attr "length" "3")])

(define_insn "sse3_monitor_<mode>"
  [(unspec_volatile [(match_operand:P 0 "register_operand" "a")
		     (match_operand:SI 1 "register_operand" "c")
		     (match_operand:SI 2 "register_operand" "d")]
		    UNSPECV_MONITOR)]
  "TARGET_SSE3"
;; 64bit version is "monitor %rax,%rcx,%rdx". But only lower 32bits in
;; RCX and RDX are used.  Since 32bit register operands are implicitly
;; zero extended to 64bit, we only need to set up 32bit registers.
  "%^monitor"
  [(set (attr "length")
     (symbol_ref ("(Pmode != word_mode) + 3")))])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; SSSE3 instructions
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_code_iterator ssse3_plusminus [plus ss_plus minus ss_minus])

(define_insn "avx2_ph<plusminus_mnemonic>wv16hi3"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(vec_concat:V16HI
	  (vec_concat:V8HI
	    (vec_concat:V4HI
	      (vec_concat:V2HI
		(ssse3_plusminus:HI
		  (vec_select:HI
		    (match_operand:V16HI 1 "register_operand" "x")
		    (parallel [(const_int 0)]))
		  (vec_select:HI (match_dup 1) (parallel [(const_int 1)])))
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 1) (parallel [(const_int 2)]))
		  (vec_select:HI (match_dup 1) (parallel [(const_int 3)]))))
	      (vec_concat:V2HI
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 1) (parallel [(const_int 4)]))
		  (vec_select:HI (match_dup 1) (parallel [(const_int 5)])))
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 1) (parallel [(const_int 6)]))
		  (vec_select:HI (match_dup 1) (parallel [(const_int 7)])))))
	    (vec_concat:V4HI
	      (vec_concat:V2HI
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 1) (parallel [(const_int 8)]))
		  (vec_select:HI (match_dup 1) (parallel [(const_int 9)])))
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 1) (parallel [(const_int 10)]))
		  (vec_select:HI (match_dup 1) (parallel [(const_int 11)]))))
	      (vec_concat:V2HI
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 1) (parallel [(const_int 12)]))
		  (vec_select:HI (match_dup 1) (parallel [(const_int 13)])))
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 1) (parallel [(const_int 14)]))
		  (vec_select:HI (match_dup 1) (parallel [(const_int 15)]))))))
	  (vec_concat:V8HI
	    (vec_concat:V4HI
	      (vec_concat:V2HI
		(ssse3_plusminus:HI
		  (vec_select:HI
		    (match_operand:V16HI 2 "nonimmediate_operand" "xm")
		    (parallel [(const_int 0)]))
		  (vec_select:HI (match_dup 2) (parallel [(const_int 1)])))
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 2) (parallel [(const_int 2)]))
		  (vec_select:HI (match_dup 2) (parallel [(const_int 3)]))))
	      (vec_concat:V2HI
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 2) (parallel [(const_int 4)]))
		  (vec_select:HI (match_dup 2) (parallel [(const_int 5)])))
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 2) (parallel [(const_int 6)]))
		  (vec_select:HI (match_dup 2) (parallel [(const_int 7)])))))
	    (vec_concat:V4HI
	      (vec_concat:V2HI
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 2) (parallel [(const_int 8)]))
		  (vec_select:HI (match_dup 2) (parallel [(const_int 9)])))
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 2) (parallel [(const_int 10)]))
		  (vec_select:HI (match_dup 2) (parallel [(const_int 11)]))))
	      (vec_concat:V2HI
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 2) (parallel [(const_int 12)]))
		  (vec_select:HI (match_dup 2) (parallel [(const_int 13)])))
		(ssse3_plusminus:HI
		  (vec_select:HI (match_dup 2) (parallel [(const_int 14)]))
		  (vec_select:HI (match_dup 2) (parallel [(const_int 15)]))))))))]
  "TARGET_AVX2"
  "vph<plusminus_mnemonic>w\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseiadd")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "ssse3_ph<plusminus_mnemonic>wv8hi3"
  [(set (match_operand:V8HI 0 "register_operand" "=x,x")
	(vec_concat:V8HI
	  (vec_concat:V4HI
	    (vec_concat:V2HI
	      (ssse3_plusminus:HI
		(vec_select:HI
		  (match_operand:V8HI 1 "register_operand" "0,x")
		  (parallel [(const_int 0)]))
		(vec_select:HI (match_dup 1) (parallel [(const_int 1)])))
	      (ssse3_plusminus:HI
		(vec_select:HI (match_dup 1) (parallel [(const_int 2)]))
		(vec_select:HI (match_dup 1) (parallel [(const_int 3)]))))
	    (vec_concat:V2HI
	      (ssse3_plusminus:HI
		(vec_select:HI (match_dup 1) (parallel [(const_int 4)]))
		(vec_select:HI (match_dup 1) (parallel [(const_int 5)])))
	      (ssse3_plusminus:HI
		(vec_select:HI (match_dup 1) (parallel [(const_int 6)]))
		(vec_select:HI (match_dup 1) (parallel [(const_int 7)])))))
	  (vec_concat:V4HI
	    (vec_concat:V2HI
	      (ssse3_plusminus:HI
		(vec_select:HI
		  (match_operand:V8HI 2 "nonimmediate_operand" "xm,xm")
		  (parallel [(const_int 0)]))
		(vec_select:HI (match_dup 2) (parallel [(const_int 1)])))
	      (ssse3_plusminus:HI
		(vec_select:HI (match_dup 2) (parallel [(const_int 2)]))
		(vec_select:HI (match_dup 2) (parallel [(const_int 3)]))))
	    (vec_concat:V2HI
	      (ssse3_plusminus:HI
		(vec_select:HI (match_dup 2) (parallel [(const_int 4)]))
		(vec_select:HI (match_dup 2) (parallel [(const_int 5)])))
	      (ssse3_plusminus:HI
		(vec_select:HI (match_dup 2) (parallel [(const_int 6)]))
		(vec_select:HI (match_dup 2) (parallel [(const_int 7)])))))))]
  "TARGET_SSSE3"
  "@
   ph<plusminus_mnemonic>w\t{%2, %0|%0, %2}
   vph<plusminus_mnemonic>w\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "atom_unit" "complex")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "ssse3_ph<plusminus_mnemonic>wv4hi3"
  [(set (match_operand:V4HI 0 "register_operand" "=y")
	(vec_concat:V4HI
	  (vec_concat:V2HI
	    (ssse3_plusminus:HI
	      (vec_select:HI
		(match_operand:V4HI 1 "register_operand" "0")
		(parallel [(const_int 0)]))
	      (vec_select:HI (match_dup 1) (parallel [(const_int 1)])))
	    (ssse3_plusminus:HI
	      (vec_select:HI (match_dup 1) (parallel [(const_int 2)]))
	      (vec_select:HI (match_dup 1) (parallel [(const_int 3)]))))
	  (vec_concat:V2HI
	    (ssse3_plusminus:HI
	      (vec_select:HI
		(match_operand:V4HI 2 "nonimmediate_operand" "ym")
		(parallel [(const_int 0)]))
	      (vec_select:HI (match_dup 2) (parallel [(const_int 1)])))
	    (ssse3_plusminus:HI
	      (vec_select:HI (match_dup 2) (parallel [(const_int 2)]))
	      (vec_select:HI (match_dup 2) (parallel [(const_int 3)]))))))]
  "TARGET_SSSE3"
  "ph<plusminus_mnemonic>w\t{%2, %0|%0, %2}"
  [(set_attr "type" "sseiadd")
   (set_attr "atom_unit" "complex")
   (set_attr "prefix_extra" "1")
   (set (attr "prefix_rex") (symbol_ref "x86_extended_reg_mentioned_p (insn)"))
   (set_attr "mode" "DI")])

(define_insn "avx2_ph<plusminus_mnemonic>dv8si3"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(vec_concat:V8SI
	  (vec_concat:V4SI
	    (vec_concat:V2SI
	      (plusminus:SI
		(vec_select:SI
		  (match_operand:V8SI 1 "register_operand" "x")
		  (parallel [(const_int 0)]))
		(vec_select:SI (match_dup 1) (parallel [(const_int 1)])))
	      (plusminus:SI
		(vec_select:SI (match_dup 1) (parallel [(const_int 2)]))
		(vec_select:SI (match_dup 1) (parallel [(const_int 3)]))))
	    (vec_concat:V2SI
	      (plusminus:SI
		(vec_select:SI (match_dup 1) (parallel [(const_int 4)]))
		(vec_select:SI (match_dup 1) (parallel [(const_int 5)])))
	      (plusminus:SI
		(vec_select:SI (match_dup 1) (parallel [(const_int 6)]))
		(vec_select:SI (match_dup 1) (parallel [(const_int 7)])))))
	  (vec_concat:V4SI
	    (vec_concat:V2SI
	      (plusminus:SI
		(vec_select:SI
		  (match_operand:V8SI 2 "nonimmediate_operand" "xm")
		  (parallel [(const_int 0)]))
		(vec_select:SI (match_dup 2) (parallel [(const_int 1)])))
	      (plusminus:SI
		(vec_select:SI (match_dup 2) (parallel [(const_int 2)]))
		(vec_select:SI (match_dup 2) (parallel [(const_int 3)]))))
	    (vec_concat:V2SI
	      (plusminus:SI
		(vec_select:SI (match_dup 2) (parallel [(const_int 4)]))
		(vec_select:SI (match_dup 2) (parallel [(const_int 5)])))
	      (plusminus:SI
		(vec_select:SI (match_dup 2) (parallel [(const_int 6)]))
		(vec_select:SI (match_dup 2) (parallel [(const_int 7)])))))))]
  "TARGET_AVX2"
  "vph<plusminus_mnemonic>d\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseiadd")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "ssse3_ph<plusminus_mnemonic>dv4si3"
  [(set (match_operand:V4SI 0 "register_operand" "=x,x")
	(vec_concat:V4SI
	  (vec_concat:V2SI
	    (plusminus:SI
	      (vec_select:SI
		(match_operand:V4SI 1 "register_operand" "0,x")
		(parallel [(const_int 0)]))
	      (vec_select:SI (match_dup 1) (parallel [(const_int 1)])))
	    (plusminus:SI
	      (vec_select:SI (match_dup 1) (parallel [(const_int 2)]))
	      (vec_select:SI (match_dup 1) (parallel [(const_int 3)]))))
	  (vec_concat:V2SI
	    (plusminus:SI
	      (vec_select:SI
		(match_operand:V4SI 2 "nonimmediate_operand" "xm,xm")
		(parallel [(const_int 0)]))
	      (vec_select:SI (match_dup 2) (parallel [(const_int 1)])))
	    (plusminus:SI
	      (vec_select:SI (match_dup 2) (parallel [(const_int 2)]))
	      (vec_select:SI (match_dup 2) (parallel [(const_int 3)]))))))]
  "TARGET_SSSE3"
  "@
   ph<plusminus_mnemonic>d\t{%2, %0|%0, %2}
   vph<plusminus_mnemonic>d\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "atom_unit" "complex")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "ssse3_ph<plusminus_mnemonic>dv2si3"
  [(set (match_operand:V2SI 0 "register_operand" "=y")
	(vec_concat:V2SI
	  (plusminus:SI
	    (vec_select:SI
	      (match_operand:V2SI 1 "register_operand" "0")
	      (parallel [(const_int 0)]))
	    (vec_select:SI (match_dup 1) (parallel [(const_int 1)])))
	  (plusminus:SI
	    (vec_select:SI
	      (match_operand:V2SI 2 "nonimmediate_operand" "ym")
	      (parallel [(const_int 0)]))
	    (vec_select:SI (match_dup 2) (parallel [(const_int 1)])))))]
  "TARGET_SSSE3"
  "ph<plusminus_mnemonic>d\t{%2, %0|%0, %2}"
  [(set_attr "type" "sseiadd")
   (set_attr "atom_unit" "complex")
   (set_attr "prefix_extra" "1")
   (set (attr "prefix_rex") (symbol_ref "x86_extended_reg_mentioned_p (insn)"))
   (set_attr "mode" "DI")])

(define_insn "avx2_pmaddubsw256"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(ss_plus:V16HI
	  (mult:V16HI
	    (zero_extend:V16HI
	      (vec_select:V16QI
		(match_operand:V32QI 1 "register_operand" "x")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)
			   (const_int 8) (const_int 10)
			   (const_int 12) (const_int 14)
			   (const_int 16) (const_int 18)
			   (const_int 20) (const_int 22)
			   (const_int 24) (const_int 26)
			   (const_int 28) (const_int 30)])))
	    (sign_extend:V16HI
	      (vec_select:V16QI
		(match_operand:V32QI 2 "nonimmediate_operand" "xm")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)
			   (const_int 8) (const_int 10)
			   (const_int 12) (const_int 14)
			   (const_int 16) (const_int 18)
			   (const_int 20) (const_int 22)
			   (const_int 24) (const_int 26)
			   (const_int 28) (const_int 30)]))))
	  (mult:V16HI
	    (zero_extend:V16HI
	      (vec_select:V16QI (match_dup 1)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)
			   (const_int 9) (const_int 11)
			   (const_int 13) (const_int 15)
			   (const_int 17) (const_int 19)
			   (const_int 21) (const_int 23)
			   (const_int 25) (const_int 27)
			   (const_int 29) (const_int 31)])))
	    (sign_extend:V16HI
	      (vec_select:V16QI (match_dup 2)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)
			   (const_int 9) (const_int 11)
			   (const_int 13) (const_int 15)
			   (const_int 17) (const_int 19)
			   (const_int 21) (const_int 23)
			   (const_int 25) (const_int 27)
			   (const_int 29) (const_int 31)]))))))]
  "TARGET_AVX2"
  "vpmaddubsw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseiadd")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "ssse3_pmaddubsw128"
  [(set (match_operand:V8HI 0 "register_operand" "=x,x")
	(ss_plus:V8HI
	  (mult:V8HI
	    (zero_extend:V8HI
	      (vec_select:V8QI
		(match_operand:V16QI 1 "register_operand" "0,x")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)
			   (const_int 8) (const_int 10)
			   (const_int 12) (const_int 14)])))
	    (sign_extend:V8HI
	      (vec_select:V8QI
		(match_operand:V16QI 2 "nonimmediate_operand" "xm,xm")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)
			   (const_int 8) (const_int 10)
			   (const_int 12) (const_int 14)]))))
	  (mult:V8HI
	    (zero_extend:V8HI
	      (vec_select:V8QI (match_dup 1)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)
			   (const_int 9) (const_int 11)
			   (const_int 13) (const_int 15)])))
	    (sign_extend:V8HI
	      (vec_select:V8QI (match_dup 2)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)
			   (const_int 9) (const_int 11)
			   (const_int 13) (const_int 15)]))))))]
  "TARGET_SSSE3"
  "@
   pmaddubsw\t{%2, %0|%0, %2}
   vpmaddubsw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseiadd")
   (set_attr "atom_unit" "simul")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "ssse3_pmaddubsw"
  [(set (match_operand:V4HI 0 "register_operand" "=y")
	(ss_plus:V4HI
	  (mult:V4HI
	    (zero_extend:V4HI
	      (vec_select:V4QI
		(match_operand:V8QI 1 "register_operand" "0")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)])))
	    (sign_extend:V4HI
	      (vec_select:V4QI
		(match_operand:V8QI 2 "nonimmediate_operand" "ym")
		(parallel [(const_int 0) (const_int 2)
			   (const_int 4) (const_int 6)]))))
	  (mult:V4HI
	    (zero_extend:V4HI
	      (vec_select:V4QI (match_dup 1)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)])))
	    (sign_extend:V4HI
	      (vec_select:V4QI (match_dup 2)
		(parallel [(const_int 1) (const_int 3)
			   (const_int 5) (const_int 7)]))))))]
  "TARGET_SSSE3"
  "pmaddubsw\t{%2, %0|%0, %2}"
  [(set_attr "type" "sseiadd")
   (set_attr "atom_unit" "simul")
   (set_attr "prefix_extra" "1")
   (set (attr "prefix_rex") (symbol_ref "x86_extended_reg_mentioned_p (insn)"))
   (set_attr "mode" "DI")])

(define_mode_iterator PMULHRSW
  [V4HI V8HI (V16HI "TARGET_AVX2")])

(define_expand "<ssse3_avx2>_pmulhrsw<mode>3"
  [(set (match_operand:PMULHRSW 0 "register_operand")
	(truncate:PMULHRSW
	  (lshiftrt:<ssedoublemode>
	    (plus:<ssedoublemode>
	      (lshiftrt:<ssedoublemode>
		(mult:<ssedoublemode>
		  (sign_extend:<ssedoublemode>
		    (match_operand:PMULHRSW 1 "nonimmediate_operand"))
		  (sign_extend:<ssedoublemode>
		    (match_operand:PMULHRSW 2 "nonimmediate_operand")))
		(const_int 14))
	      (match_dup 3))
	    (const_int 1))))]
  "TARGET_AVX2"
{
  operands[3] = CONST1_RTX(<MODE>mode);
  ix86_fixup_binary_operands_no_copy (MULT, <MODE>mode, operands);
})

(define_insn "*<ssse3_avx2>_pmulhrsw<mode>3"
  [(set (match_operand:VI2_AVX2 0 "register_operand" "=x,x")
	(truncate:VI2_AVX2
	  (lshiftrt:<ssedoublemode>
	    (plus:<ssedoublemode>
	      (lshiftrt:<ssedoublemode>
		(mult:<ssedoublemode>
		  (sign_extend:<ssedoublemode>
		    (match_operand:VI2_AVX2 1 "nonimmediate_operand" "%0,x"))
		  (sign_extend:<ssedoublemode>
		    (match_operand:VI2_AVX2 2 "nonimmediate_operand" "xm,xm")))
		(const_int 14))
	      (match_operand:VI2_AVX2 3 "const1_operand"))
	    (const_int 1))))]
  "TARGET_SSSE3 && ix86_binary_operator_ok (MULT, <MODE>mode, operands)"
  "@
   pmulhrsw\t{%2, %0|%0, %2}
   vpmulhrsw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseimul")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "*ssse3_pmulhrswv4hi3"
  [(set (match_operand:V4HI 0 "register_operand" "=y")
	(truncate:V4HI
	  (lshiftrt:V4SI
	    (plus:V4SI
	      (lshiftrt:V4SI
		(mult:V4SI
		  (sign_extend:V4SI
		    (match_operand:V4HI 1 "nonimmediate_operand" "%0"))
		  (sign_extend:V4SI
		    (match_operand:V4HI 2 "nonimmediate_operand" "ym")))
		(const_int 14))
	      (match_operand:V4HI 3 "const1_operand"))
	    (const_int 1))))]
  "TARGET_SSSE3 && ix86_binary_operator_ok (MULT, V4HImode, operands)"
  "pmulhrsw\t{%2, %0|%0, %2}"
  [(set_attr "type" "sseimul")
   (set_attr "prefix_extra" "1")
   (set (attr "prefix_rex") (symbol_ref "x86_extended_reg_mentioned_p (insn)"))
   (set_attr "mode" "DI")])

(define_insn "<ssse3_avx2>_pshufb<mode>3"
  [(set (match_operand:VI1_AVX2 0 "register_operand" "=x,x")
	(unspec:VI1_AVX2
	  [(match_operand:VI1_AVX2 1 "register_operand" "0,x")
	   (match_operand:VI1_AVX2 2 "nonimmediate_operand" "xm,xm")]
	  UNSPEC_PSHUFB))]
  "TARGET_SSSE3"
  "@
   pshufb\t{%2, %0|%0, %2}
   vpshufb\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog1")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "vector,vector")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "ssse3_pshufbv8qi3"
  [(set (match_operand:V8QI 0 "register_operand" "=y")
	(unspec:V8QI [(match_operand:V8QI 1 "register_operand" "0")
		      (match_operand:V8QI 2 "nonimmediate_operand" "ym")]
		     UNSPEC_PSHUFB))]
  "TARGET_SSSE3"
  "pshufb\t{%2, %0|%0, %2}";
  [(set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set (attr "prefix_rex") (symbol_ref "x86_extended_reg_mentioned_p (insn)"))
   (set_attr "mode" "DI")])

(define_insn "<ssse3_avx2>_psign<mode>3"
  [(set (match_operand:VI124_AVX2 0 "register_operand" "=x,x")
	(unspec:VI124_AVX2
	  [(match_operand:VI124_AVX2 1 "register_operand" "0,x")
	   (match_operand:VI124_AVX2 2 "nonimmediate_operand" "xm,xm")]
	  UNSPEC_PSIGN))]
  "TARGET_SSSE3"
  "@
   psign<ssemodesuffix>\t{%2, %0|%0, %2}
   vpsign<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog1")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "ssse3_psign<mode>3"
  [(set (match_operand:MMXMODEI 0 "register_operand" "=y")
	(unspec:MMXMODEI
	  [(match_operand:MMXMODEI 1 "register_operand" "0")
	   (match_operand:MMXMODEI 2 "nonimmediate_operand" "ym")]
	  UNSPEC_PSIGN))]
  "TARGET_SSSE3"
  "psign<mmxvecsize>\t{%2, %0|%0, %2}";
  [(set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set (attr "prefix_rex") (symbol_ref "x86_extended_reg_mentioned_p (insn)"))
   (set_attr "mode" "DI")])

(define_insn "<ssse3_avx2>_palignr<mode>"
  [(set (match_operand:SSESCALARMODE 0 "register_operand" "=x,x")
	(unspec:SSESCALARMODE
	  [(match_operand:SSESCALARMODE 1 "register_operand" "0,x")
	   (match_operand:SSESCALARMODE 2 "nonimmediate_operand" "xm,xm")
	   (match_operand:SI 3 "const_0_to_255_mul_8_operand" "n,n")]
	  UNSPEC_PALIGNR))]
  "TARGET_SSSE3"
{
  operands[3] = GEN_INT (INTVAL (operands[3]) / 8);

  switch (which_alternative)
    {
    case 0:
      return "palignr\t{%3, %2, %0|%0, %2, %3}";
    case 1:
      return "vpalignr\t{%3, %2, %1, %0|%0, %1, %2, %3}";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sseishft")
   (set_attr "atom_unit" "sishuf")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "ssse3_palignrdi"
  [(set (match_operand:DI 0 "register_operand" "=y")
	(unspec:DI [(match_operand:DI 1 "register_operand" "0")
		    (match_operand:DI 2 "nonimmediate_operand" "ym")
		    (match_operand:SI 3 "const_0_to_255_mul_8_operand" "n")]
		   UNSPEC_PALIGNR))]
  "TARGET_SSSE3"
{
  operands[3] = GEN_INT (INTVAL (operands[3]) / 8);
  return "palignr\t{%3, %2, %0|%0, %2, %3}";
}
  [(set_attr "type" "sseishft")
   (set_attr "atom_unit" "sishuf")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set (attr "prefix_rex") (symbol_ref "x86_extended_reg_mentioned_p (insn)"))
   (set_attr "mode" "DI")])

(define_insn "abs<mode>2"
  [(set (match_operand:VI124_AVX2 0 "register_operand" "=x")
	(abs:VI124_AVX2
	  (match_operand:VI124_AVX2 1 "nonimmediate_operand" "xm")))]
  "TARGET_SSSE3"
  "%vpabs<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "sselog1")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "abs<mode>2"
  [(set (match_operand:MMXMODEI 0 "register_operand" "=y")
	(abs:MMXMODEI
	  (match_operand:MMXMODEI 1 "nonimmediate_operand" "ym")))]
  "TARGET_SSSE3"
  "pabs<mmxvecsize>\t{%1, %0|%0, %1}";
  [(set_attr "type" "sselog1")
   (set_attr "prefix_rep" "0")
   (set_attr "prefix_extra" "1")
   (set (attr "prefix_rex") (symbol_ref "x86_extended_reg_mentioned_p (insn)"))
   (set_attr "mode" "DI")])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; AMD SSE4A instructions
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_insn "sse4a_movnt<mode>"
  [(set (match_operand:MODEF 0 "memory_operand" "=m")
	(unspec:MODEF
	  [(match_operand:MODEF 1 "register_operand" "x")]
	  UNSPEC_MOVNT))]
  "TARGET_SSE4A"
  "movnt<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "mode" "<MODE>")])

(define_insn "sse4a_vmmovnt<mode>"
  [(set (match_operand:<ssescalarmode> 0 "memory_operand" "=m")
	(unspec:<ssescalarmode>
	  [(vec_select:<ssescalarmode>
	     (match_operand:VF_128 1 "register_operand" "x")
	     (parallel [(const_int 0)]))]
	  UNSPEC_MOVNT))]
  "TARGET_SSE4A"
  "movnt<ssescalarmodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "mode" "<ssescalarmode>")])

(define_insn "sse4a_extrqi"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(unspec:V2DI [(match_operand:V2DI 1 "register_operand" "0")
		      (match_operand 2 "const_0_to_255_operand")
		      (match_operand 3 "const_0_to_255_operand")]
		     UNSPEC_EXTRQI))]
  "TARGET_SSE4A"
  "extrq\t{%3, %2, %0|%0, %2, %3}"
  [(set_attr "type" "sse")
   (set_attr "prefix_data16" "1")
   (set_attr "length_immediate" "2")
   (set_attr "mode" "TI")])

(define_insn "sse4a_extrq"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(unspec:V2DI [(match_operand:V2DI 1 "register_operand" "0")
		      (match_operand:V16QI 2 "register_operand" "x")]
		     UNSPEC_EXTRQ))]
  "TARGET_SSE4A"
  "extrq\t{%2, %0|%0, %2}"
  [(set_attr "type" "sse")
   (set_attr "prefix_data16" "1")
   (set_attr "mode" "TI")])

(define_insn "sse4a_insertqi"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(unspec:V2DI [(match_operand:V2DI 1 "register_operand" "0")
		      (match_operand:V2DI 2 "register_operand" "x")
		      (match_operand 3 "const_0_to_255_operand")
		      (match_operand 4 "const_0_to_255_operand")]
		     UNSPEC_INSERTQI))]
  "TARGET_SSE4A"
  "insertq\t{%4, %3, %2, %0|%0, %2, %3, %4}"
  [(set_attr "type" "sseins")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_rep" "1")
   (set_attr "length_immediate" "2")
   (set_attr "mode" "TI")])

(define_insn "sse4a_insertq"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(unspec:V2DI [(match_operand:V2DI 1 "register_operand" "0")
		      (match_operand:V2DI 2 "register_operand" "x")]
		     UNSPEC_INSERTQ))]
  "TARGET_SSE4A"
  "insertq\t{%2, %0|%0, %2}"
  [(set_attr "type" "sseins")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_rep" "1")
   (set_attr "mode" "TI")])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Intel SSE4.1 instructions
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_insn "<sse4_1>_blend<ssemodesuffix><avxsizesuffix>"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(vec_merge:VF
	  (match_operand:VF 2 "nonimmediate_operand" "xm,xm")
	  (match_operand:VF 1 "register_operand" "0,x")
	  (match_operand:SI 3 "const_0_to_<blendbits>_operand")))]
  "TARGET_SSE4_1"
  "@
   blend<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vblend<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssemov")
   (set_attr "length_immediate" "1")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse4_1>_blendv<ssemodesuffix><avxsizesuffix>"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(unspec:VF
	  [(match_operand:VF 1 "register_operand" "0,x")
	   (match_operand:VF 2 "nonimmediate_operand" "xm,xm")
	   (match_operand:VF 3 "register_operand" "Yz,x")]
	  UNSPEC_BLENDV))]
  "TARGET_SSE4_1"
  "@
   blendv<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vblendv<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssemov")
   (set_attr "length_immediate" "1")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "vector,vector") 
   (set_attr "mode" "<MODE>")])

(define_insn "<sse4_1>_dp<ssemodesuffix><avxsizesuffix>"
  [(set (match_operand:VF 0 "register_operand" "=x,x")
	(unspec:VF
	  [(match_operand:VF 1 "nonimmediate_operand" "%0,x")
	   (match_operand:VF 2 "nonimmediate_operand" "xm,xm")
	   (match_operand:SI 3 "const_0_to_255_operand" "n,n")]
	  UNSPEC_DP))]
  "TARGET_SSE4_1"
  "@
   dp<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vdp<ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssemul")
   (set_attr "length_immediate" "1")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "vector,vector")
   (set_attr "mode" "<MODE>")])

(define_insn "<sse4_1_avx2>_movntdqa"
  [(set (match_operand:VI8_AVX2 0 "register_operand" "=x")
	(unspec:VI8_AVX2 [(match_operand:VI8_AVX2 1 "memory_operand" "m")]
		     UNSPEC_MOVNTDQA))]
  "TARGET_SSE4_1"
  "%vmovntdqa\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "<sse4_1_avx2>_mpsadbw"
  [(set (match_operand:VI1_AVX2 0 "register_operand" "=x,x")
	(unspec:VI1_AVX2
	  [(match_operand:VI1_AVX2 1 "register_operand" "0,x")
	   (match_operand:VI1_AVX2 2 "nonimmediate_operand" "xm,xm")
	   (match_operand:SI 3 "const_0_to_255_operand" "n,n")]
	  UNSPEC_MPSADBW))]
  "TARGET_SSE4_1"
  "@
   mpsadbw\t{%3, %2, %0|%0, %2, %3}
   vmpsadbw\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "vector,vector")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "avx2_packusdw"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(vec_concat:V16HI
	  (us_truncate:V8HI
	    (match_operand:V8SI 1 "register_operand" "x"))
	  (us_truncate:V8HI
	    (match_operand:V8SI 2 "nonimmediate_operand" "xm"))))]
  "TARGET_AVX2"
  "vpackusdw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "sse4_1_packusdw"
  [(set (match_operand:V8HI 0 "register_operand" "=x,x")
	(vec_concat:V8HI
	  (us_truncate:V4HI
	    (match_operand:V4SI 1 "register_operand" "0,x"))
	  (us_truncate:V4HI
	    (match_operand:V4SI 2 "nonimmediate_operand" "xm,xm"))))]
  "TARGET_SSE4_1"
  "@
   packusdw\t{%2, %0|%0, %2}
   vpackusdw\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_insn "<sse4_1_avx2>_pblendvb"
  [(set (match_operand:VI1_AVX2 0 "register_operand" "=x,x")
	(unspec:VI1_AVX2
	  [(match_operand:VI1_AVX2 1 "register_operand"  "0,x")
	   (match_operand:VI1_AVX2 2 "nonimmediate_operand" "xm,xm")
	   (match_operand:VI1_AVX2 3 "register_operand" "Yz,x")]
	  UNSPEC_BLENDV))]
  "TARGET_SSE4_1"
  "@
   pblendvb\t{%3, %2, %0|%0, %2, %3}
   vpblendvb\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "*,1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "vector,vector")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "sse4_1_pblendw"
  [(set (match_operand:V8HI 0 "register_operand" "=x,x")
	(vec_merge:V8HI
	  (match_operand:V8HI 2 "nonimmediate_operand" "xm,xm")
	  (match_operand:V8HI 1 "register_operand" "0,x")
	  (match_operand:SI 3 "const_0_to_255_operand" "n,n")))]
  "TARGET_SSE4_1"
  "@
   pblendw\t{%3, %2, %0|%0, %2, %3}
   vpblendw\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

;; The builtin uses an 8-bit immediate.  Expand that.
(define_expand "avx2_pblendw"
  [(set (match_operand:V16HI 0 "register_operand")
	(vec_merge:V16HI
	  (match_operand:V16HI 2 "nonimmediate_operand")
	  (match_operand:V16HI 1 "register_operand")
	  (match_operand:SI 3 "const_0_to_255_operand")))]
  "TARGET_AVX2"
{
  HOST_WIDE_INT val = INTVAL (operands[3]) & 0xff;
  operands[3] = GEN_INT (val << 8 | val);
})

(define_insn "*avx2_pblendw"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(vec_merge:V16HI
	  (match_operand:V16HI 2 "nonimmediate_operand" "xm")
	  (match_operand:V16HI 1 "register_operand" "x")
	  (match_operand:SI 3 "avx2_pblendw_operand" "n")))]
  "TARGET_AVX2"
{
  operands[3] = GEN_INT (INTVAL (operands[3]) & 0xff);
  return "vpblendw\t{%3, %2, %1, %0|%0, %1, %2, %3}";
}
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "avx2_pblendd<mode>"
  [(set (match_operand:VI4_AVX2 0 "register_operand" "=x")
	(vec_merge:VI4_AVX2
	  (match_operand:VI4_AVX2 2 "nonimmediate_operand" "xm")
	  (match_operand:VI4_AVX2 1 "register_operand" "x")
	  (match_operand:SI 3 "const_0_to_255_operand" "n")))]
  "TARGET_AVX2"
  "vpblendd\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "sse4_1_phminposuw"
  [(set (match_operand:V8HI 0 "register_operand" "=x")
	(unspec:V8HI [(match_operand:V8HI 1 "nonimmediate_operand" "xm")]
		     UNSPEC_PHMINPOSUW))]
  "TARGET_SSE4_1"
  "%vphminposuw\t{%1, %0|%0, %1}"
  [(set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_<code>v16qiv16hi2"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(any_extend:V16HI
	  (match_operand:V16QI 1 "nonimmediate_operand" "xm")))]
  "TARGET_AVX2"
  "vpmov<extsuffix>bw\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "sse4_1_<code>v8qiv8hi2"
  [(set (match_operand:V8HI 0 "register_operand" "=x")
	(any_extend:V8HI
	  (vec_select:V8QI
	    (match_operand:V16QI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)
		       (const_int 4) (const_int 5)
		       (const_int 6) (const_int 7)]))))]
  "TARGET_SSE4_1"
  "%vpmov<extsuffix>bw\t{%1, %0|%0, %q1}"
  [(set_attr "type" "ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_<code>v8qiv8si2"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(any_extend:V8SI
	  (vec_select:V8QI
	    (match_operand:V16QI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)
		       (const_int 4) (const_int 5)
		       (const_int 6) (const_int 7)]))))]
  "TARGET_AVX2"
  "vpmov<extsuffix>bd\t{%1, %0|%0, %q1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "sse4_1_<code>v4qiv4si2"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(any_extend:V4SI
	  (vec_select:V4QI
	    (match_operand:V16QI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)]))))]
  "TARGET_SSE4_1"
  "%vpmov<extsuffix>bd\t{%1, %0|%0, %k1}"
  [(set_attr "type" "ssemov")
   (set_attr "ssememalign" "32")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_<code>v8hiv8si2"
  [(set (match_operand:V8SI 0 "register_operand" "=x")
	(any_extend:V8SI
	    (match_operand:V8HI 1 "nonimmediate_operand" "xm")))]
  "TARGET_AVX2"
  "vpmov<extsuffix>wd\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "sse4_1_<code>v4hiv4si2"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(any_extend:V4SI
	  (vec_select:V4HI
	    (match_operand:V8HI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)]))))]
  "TARGET_SSE4_1"
  "%vpmov<extsuffix>wd\t{%1, %0|%0, %q1}"
  [(set_attr "type" "ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_<code>v4qiv4di2"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(any_extend:V4DI
	  (vec_select:V4QI
	    (match_operand:V16QI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)]))))]
  "TARGET_AVX2"
  "vpmov<extsuffix>bq\t{%1, %0|%0, %k1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "sse4_1_<code>v2qiv2di2"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(any_extend:V2DI
	  (vec_select:V2QI
	    (match_operand:V16QI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)]))))]
  "TARGET_SSE4_1"
  "%vpmov<extsuffix>bq\t{%1, %0|%0, %w1}"
  [(set_attr "type" "ssemov")
   (set_attr "ssememalign" "16")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_<code>v4hiv4di2"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(any_extend:V4DI
	  (vec_select:V4HI
	    (match_operand:V8HI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)]))))]
  "TARGET_AVX2"
  "vpmov<extsuffix>wq\t{%1, %0|%0, %q1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "sse4_1_<code>v2hiv2di2"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(any_extend:V2DI
	  (vec_select:V2HI
	    (match_operand:V8HI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)]))))]
  "TARGET_SSE4_1"
  "%vpmov<extsuffix>wq\t{%1, %0|%0, %k1}"
  [(set_attr "type" "ssemov")
   (set_attr "ssememalign" "32")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "avx2_<code>v4siv4di2"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(any_extend:V4DI
	    (match_operand:V4SI 1 "nonimmediate_operand" "xm")))]
  "TARGET_AVX2"
  "vpmov<extsuffix>dq\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "mode" "OI")])

(define_insn "sse4_1_<code>v2siv2di2"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(any_extend:V2DI
	  (vec_select:V2SI
	    (match_operand:V4SI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 1)]))))]
  "TARGET_SSE4_1"
  "%vpmov<extsuffix>dq\t{%1, %0|%0, %q1}"
  [(set_attr "type" "ssemov")
   (set_attr "ssememalign" "64")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

;; ptestps/ptestpd are very similar to comiss and ucomiss when
;; setting FLAGS_REG. But it is not a really compare instruction.
(define_insn "avx_vtest<ssemodesuffix><avxsizesuffix>"
  [(set (reg:CC FLAGS_REG)
	(unspec:CC [(match_operand:VF 0 "register_operand" "x")
		    (match_operand:VF 1 "nonimmediate_operand" "xm")]
		   UNSPEC_VTESTP))]
  "TARGET_AVX"
  "vtest<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecomi")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<MODE>")])

;; ptest is very similar to comiss and ucomiss when setting FLAGS_REG.
;; But it is not a really compare instruction.
(define_insn "avx_ptest256"
  [(set (reg:CC FLAGS_REG)
	(unspec:CC [(match_operand:V4DI 0 "register_operand" "x")
		    (match_operand:V4DI 1 "nonimmediate_operand" "xm")]
		   UNSPEC_PTEST))]
  "TARGET_AVX"
  "vptest\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecomi")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "OI")])

(define_insn "sse4_1_ptest"
  [(set (reg:CC FLAGS_REG)
	(unspec:CC [(match_operand:V2DI 0 "register_operand" "x")
		    (match_operand:V2DI 1 "nonimmediate_operand" "xm")]
		   UNSPEC_PTEST))]
  "TARGET_SSE4_1"
  "%vptest\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecomi")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "<sse4_1>_round<ssemodesuffix><avxsizesuffix>"
  [(set (match_operand:VF 0 "register_operand" "=x")
	(unspec:VF
	  [(match_operand:VF 1 "nonimmediate_operand" "xm")
	   (match_operand:SI 2 "const_0_to_15_operand" "n")]
	  UNSPEC_ROUND))]
  "TARGET_ROUND"
  "%vround<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "ssecvt")
   (set (attr "prefix_data16")
     (if_then_else
       (match_test "TARGET_AVX")
     (const_string "*")
     (const_string "1")))
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "<MODE>")])

(define_expand "<sse4_1>_round<ssemodesuffix>_sfix<avxsizesuffix>"
  [(match_operand:<sseintvecmode> 0 "register_operand")
   (match_operand:VF1 1 "nonimmediate_operand")
   (match_operand:SI 2 "const_0_to_15_operand")]
  "TARGET_ROUND"
{
  rtx tmp = gen_reg_rtx (<MODE>mode);

  emit_insn
    (gen_<sse4_1>_round<ssemodesuffix><avxsizesuffix> (tmp, operands[1],
						       operands[2]));
  emit_insn
    (gen_fix_trunc<mode><sseintvecmodelower>2 (operands[0], tmp));
  DONE;
})

(define_expand "<sse4_1>_round<ssemodesuffix>_vec_pack_sfix<avxsizesuffix>"
  [(match_operand:<ssepackfltmode> 0 "register_operand")
   (match_operand:VF2 1 "nonimmediate_operand")
   (match_operand:VF2 2 "nonimmediate_operand")
   (match_operand:SI 3 "const_0_to_15_operand")]
  "TARGET_ROUND"
{
  rtx tmp0, tmp1;

  if (<MODE>mode == V2DFmode
      && TARGET_AVX && !TARGET_PREFER_AVX128)
    {
      rtx tmp2 = gen_reg_rtx (V4DFmode);

      tmp0 = gen_reg_rtx (V4DFmode);
      tmp1 = force_reg (V2DFmode, operands[1]);

      emit_insn (gen_avx_vec_concatv4df (tmp0, tmp1, operands[2]));
      emit_insn (gen_avx_roundpd256 (tmp2, tmp0, operands[3]));
      emit_insn (gen_fix_truncv4dfv4si2 (operands[0], tmp2));
    }
  else
    {
      tmp0 = gen_reg_rtx (<MODE>mode);
      tmp1 = gen_reg_rtx (<MODE>mode);

      emit_insn
       (gen_<sse4_1>_round<ssemodesuffix><avxsizesuffix> (tmp0, operands[1],
							  operands[3]));
      emit_insn
       (gen_<sse4_1>_round<ssemodesuffix><avxsizesuffix> (tmp1, operands[2],
							  operands[3]));
      emit_insn
       (gen_vec_pack_sfix_trunc_<mode> (operands[0], tmp0, tmp1));
    }
  DONE;
})

(define_insn "sse4_1_round<ssescalarmodesuffix>"
  [(set (match_operand:VF_128 0 "register_operand" "=x,x")
	(vec_merge:VF_128
	  (unspec:VF_128
	    [(match_operand:VF_128 2 "register_operand" "x,x")
	     (match_operand:SI 3 "const_0_to_15_operand" "n,n")]
	    UNSPEC_ROUND)
	  (match_operand:VF_128 1 "register_operand" "0,x")
	  (const_int 1)))]
  "TARGET_ROUND"
  "@
   round<ssescalarmodesuffix>\t{%3, %2, %0|%0, %2, %3}
   vround<ssescalarmodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "ssecvt")
   (set_attr "length_immediate" "1")
   (set_attr "prefix_data16" "1,*")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "<MODE>")])

(define_expand "round<mode>2"
  [(set (match_dup 4)
	(plus:VF
	  (match_operand:VF 1 "register_operand")
	  (match_dup 3)))
   (set (match_operand:VF 0 "register_operand")
	(unspec:VF
	  [(match_dup 4) (match_dup 5)]
	  UNSPEC_ROUND))]
  "TARGET_ROUND && !flag_trapping_math"
{
  enum machine_mode scalar_mode;
  const struct real_format *fmt;
  REAL_VALUE_TYPE pred_half, half_minus_pred_half;
  rtx half, vec_half;

  scalar_mode = GET_MODE_INNER (<MODE>mode);

  /* load nextafter (0.5, 0.0) */
  fmt = REAL_MODE_FORMAT (scalar_mode);
  real_2expN (&half_minus_pred_half, -(fmt->p) - 1, scalar_mode);
  REAL_ARITHMETIC (pred_half, MINUS_EXPR, dconsthalf, half_minus_pred_half);
  half = const_double_from_real_value (pred_half, scalar_mode);

  vec_half = ix86_build_const_vector (<MODE>mode, true, half);
  vec_half = force_reg (<MODE>mode, vec_half);

  operands[3] = gen_reg_rtx (<MODE>mode);
  emit_insn (gen_copysign<mode>3 (operands[3], vec_half, operands[1]));

  operands[4] = gen_reg_rtx (<MODE>mode);
  operands[5] = GEN_INT (ROUND_TRUNC);
})

(define_expand "round<mode>2_sfix"
  [(match_operand:<sseintvecmode> 0 "register_operand")
   (match_operand:VF1 1 "register_operand")]
  "TARGET_ROUND && !flag_trapping_math"
{
  rtx tmp = gen_reg_rtx (<MODE>mode);

  emit_insn (gen_round<mode>2 (tmp, operands[1]));

  emit_insn
    (gen_fix_trunc<mode><sseintvecmodelower>2 (operands[0], tmp));
  DONE;
})

(define_expand "round<mode>2_vec_pack_sfix"
  [(match_operand:<ssepackfltmode> 0 "register_operand")
   (match_operand:VF2 1 "register_operand")
   (match_operand:VF2 2 "register_operand")]
  "TARGET_ROUND && !flag_trapping_math"
{
  rtx tmp0, tmp1;

  if (<MODE>mode == V2DFmode
      && TARGET_AVX && !TARGET_PREFER_AVX128)
    {
      rtx tmp2 = gen_reg_rtx (V4DFmode);

      tmp0 = gen_reg_rtx (V4DFmode);
      tmp1 = force_reg (V2DFmode, operands[1]);

      emit_insn (gen_avx_vec_concatv4df (tmp0, tmp1, operands[2]));
      emit_insn (gen_roundv4df2 (tmp2, tmp0));
      emit_insn (gen_fix_truncv4dfv4si2 (operands[0], tmp2));
    }
  else
    {
      tmp0 = gen_reg_rtx (<MODE>mode);
      tmp1 = gen_reg_rtx (<MODE>mode);

      emit_insn (gen_round<mode>2 (tmp0, operands[1]));
      emit_insn (gen_round<mode>2 (tmp1, operands[2]));

      emit_insn
       (gen_vec_pack_sfix_trunc_<mode> (operands[0], tmp0, tmp1));
    }
  DONE;
})

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Intel SSE4.2 string/text processing instructions
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_insn_and_split "sse4_2_pcmpestr"
  [(set (match_operand:SI 0 "register_operand" "=c,c")
	(unspec:SI
	  [(match_operand:V16QI 2 "register_operand" "x,x")
	   (match_operand:SI 3 "register_operand" "a,a")
	   (match_operand:V16QI 4 "nonimmediate_operand" "x,m")
	   (match_operand:SI 5 "register_operand" "d,d")
	   (match_operand:SI 6 "const_0_to_255_operand" "n,n")]
	  UNSPEC_PCMPESTR))
   (set (match_operand:V16QI 1 "register_operand" "=Yz,Yz")
	(unspec:V16QI
	  [(match_dup 2)
	   (match_dup 3)
	   (match_dup 4)
	   (match_dup 5)
	   (match_dup 6)]
	  UNSPEC_PCMPESTR))
   (set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_dup 2)
	   (match_dup 3)
	   (match_dup 4)
	   (match_dup 5)
	   (match_dup 6)]
	  UNSPEC_PCMPESTR))]
  "TARGET_SSE4_2
   && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(const_int 0)]
{
  int ecx = !find_regno_note (curr_insn, REG_UNUSED, REGNO (operands[0]));
  int xmm0 = !find_regno_note (curr_insn, REG_UNUSED, REGNO (operands[1]));
  int flags = !find_regno_note (curr_insn, REG_UNUSED, FLAGS_REG);

  if (ecx)
    emit_insn (gen_sse4_2_pcmpestri (operands[0], operands[2],
				     operands[3], operands[4],
				     operands[5], operands[6]));
  if (xmm0)
    emit_insn (gen_sse4_2_pcmpestrm (operands[1], operands[2],
				     operands[3], operands[4],
				     operands[5], operands[6]));
  if (flags && !(ecx || xmm0))
    emit_insn (gen_sse4_2_pcmpestr_cconly (NULL, NULL,
					   operands[2], operands[3],
					   operands[4], operands[5],
					   operands[6]));
  if (!(flags || ecx || xmm0))
    emit_note (NOTE_INSN_DELETED);

  DONE;
}
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "none,load")
   (set_attr "mode" "TI")])

(define_insn_and_split "*sse4_2_pcmpestr_unaligned"
  [(set (match_operand:SI 0 "register_operand" "=c")
	(unspec:SI
	  [(match_operand:V16QI 2 "register_operand" "x")
	   (match_operand:SI 3 "register_operand" "a")
	   (unspec:V16QI
	     [(match_operand:V16QI 4 "memory_operand" "m")]
	     UNSPEC_LOADU)
	   (match_operand:SI 5 "register_operand" "d")
	   (match_operand:SI 6 "const_0_to_255_operand" "n")]
	  UNSPEC_PCMPESTR))
   (set (match_operand:V16QI 1 "register_operand" "=Yz")
	(unspec:V16QI
	  [(match_dup 2)
	   (match_dup 3)
	   (unspec:V16QI [(match_dup 4)] UNSPEC_LOADU)
	   (match_dup 5)
	   (match_dup 6)]
	  UNSPEC_PCMPESTR))
   (set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_dup 2)
	   (match_dup 3)
	   (unspec:V16QI [(match_dup 4)] UNSPEC_LOADU)
	   (match_dup 5)
	   (match_dup 6)]
	  UNSPEC_PCMPESTR))]
  "TARGET_SSE4_2
   && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(const_int 0)]
{
  int ecx = !find_regno_note (curr_insn, REG_UNUSED, REGNO (operands[0]));
  int xmm0 = !find_regno_note (curr_insn, REG_UNUSED, REGNO (operands[1]));
  int flags = !find_regno_note (curr_insn, REG_UNUSED, FLAGS_REG);

  if (ecx)
    emit_insn (gen_sse4_2_pcmpestri (operands[0], operands[2],
				     operands[3], operands[4],
				     operands[5], operands[6]));
  if (xmm0)
    emit_insn (gen_sse4_2_pcmpestrm (operands[1], operands[2],
				     operands[3], operands[4],
				     operands[5], operands[6]));
  if (flags && !(ecx || xmm0))
    emit_insn (gen_sse4_2_pcmpestr_cconly (NULL, NULL,
					   operands[2], operands[3],
					   operands[4], operands[5],
					   operands[6]));
  if (!(flags || ecx || xmm0))
    emit_note (NOTE_INSN_DELETED);

  DONE;
}
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "load")
   (set_attr "mode" "TI")])

(define_insn "sse4_2_pcmpestri"
  [(set (match_operand:SI 0 "register_operand" "=c,c")
	(unspec:SI
	  [(match_operand:V16QI 1 "register_operand" "x,x")
	   (match_operand:SI 2 "register_operand" "a,a")
	   (match_operand:V16QI 3 "nonimmediate_operand" "x,m")
	   (match_operand:SI 4 "register_operand" "d,d")
	   (match_operand:SI 5 "const_0_to_255_operand" "n,n")]
	  UNSPEC_PCMPESTR))
   (set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_dup 1)
	   (match_dup 2)
	   (match_dup 3)
	   (match_dup 4)
	   (match_dup 5)]
	  UNSPEC_PCMPESTR))]
  "TARGET_SSE4_2"
  "%vpcmpestri\t{%5, %3, %1|%1, %3, %5}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "btver2_decode" "vector")
   (set_attr "memory" "none,load")
   (set_attr "mode" "TI")])

(define_insn "sse4_2_pcmpestrm"
  [(set (match_operand:V16QI 0 "register_operand" "=Yz,Yz")
	(unspec:V16QI
	  [(match_operand:V16QI 1 "register_operand" "x,x")
	   (match_operand:SI 2 "register_operand" "a,a")
	   (match_operand:V16QI 3 "nonimmediate_operand" "x,m")
	   (match_operand:SI 4 "register_operand" "d,d")
	   (match_operand:SI 5 "const_0_to_255_operand" "n,n")]
	  UNSPEC_PCMPESTR))
   (set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_dup 1)
	   (match_dup 2)
	   (match_dup 3)
	   (match_dup 4)
	   (match_dup 5)]
	  UNSPEC_PCMPESTR))]
  "TARGET_SSE4_2"
  "%vpcmpestrm\t{%5, %3, %1|%1, %3, %5}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "memory" "none,load")
   (set_attr "mode" "TI")])

(define_insn "sse4_2_pcmpestr_cconly"
  [(set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_operand:V16QI 2 "register_operand" "x,x,x,x")
	   (match_operand:SI 3 "register_operand" "a,a,a,a")
	   (match_operand:V16QI 4 "nonimmediate_operand" "x,m,x,m")
	   (match_operand:SI 5 "register_operand" "d,d,d,d")
	   (match_operand:SI 6 "const_0_to_255_operand" "n,n,n,n")]
	  UNSPEC_PCMPESTR))
   (clobber (match_scratch:V16QI 0 "=Yz,Yz,X,X"))
   (clobber (match_scratch:SI    1 "= X, X,c,c"))]
  "TARGET_SSE4_2"
  "@
   %vpcmpestrm\t{%6, %4, %2|%2, %4, %6}
   %vpcmpestrm\t{%6, %4, %2|%2, %4, %6}
   %vpcmpestri\t{%6, %4, %2|%2, %4, %6}
   %vpcmpestri\t{%6, %4, %2|%2, %4, %6}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "none,load,none,load")
   (set_attr "btver2_decode" "vector,vector,vector,vector") 
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn_and_split "sse4_2_pcmpistr"
  [(set (match_operand:SI 0 "register_operand" "=c,c")
	(unspec:SI
	  [(match_operand:V16QI 2 "register_operand" "x,x")
	   (match_operand:V16QI 3 "nonimmediate_operand" "x,m")
	   (match_operand:SI 4 "const_0_to_255_operand" "n,n")]
	  UNSPEC_PCMPISTR))
   (set (match_operand:V16QI 1 "register_operand" "=Yz,Yz")
	(unspec:V16QI
	  [(match_dup 2)
	   (match_dup 3)
	   (match_dup 4)]
	  UNSPEC_PCMPISTR))
   (set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_dup 2)
	   (match_dup 3)
	   (match_dup 4)]
	  UNSPEC_PCMPISTR))]
  "TARGET_SSE4_2
   && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(const_int 0)]
{
  int ecx = !find_regno_note (curr_insn, REG_UNUSED, REGNO (operands[0]));
  int xmm0 = !find_regno_note (curr_insn, REG_UNUSED, REGNO (operands[1]));
  int flags = !find_regno_note (curr_insn, REG_UNUSED, FLAGS_REG);

  if (ecx)
    emit_insn (gen_sse4_2_pcmpistri (operands[0], operands[2],
				     operands[3], operands[4]));
  if (xmm0)
    emit_insn (gen_sse4_2_pcmpistrm (operands[1], operands[2],
				     operands[3], operands[4]));
  if (flags && !(ecx || xmm0))
    emit_insn (gen_sse4_2_pcmpistr_cconly (NULL, NULL,
					   operands[2], operands[3],
					   operands[4]));
  if (!(flags || ecx || xmm0))
    emit_note (NOTE_INSN_DELETED);

  DONE;
}
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "none,load")
   (set_attr "mode" "TI")])

(define_insn_and_split "*sse4_2_pcmpistr_unaligned"
  [(set (match_operand:SI 0 "register_operand" "=c")
	(unspec:SI
	  [(match_operand:V16QI 2 "register_operand" "x")
	   (unspec:V16QI
	     [(match_operand:V16QI 3 "memory_operand" "m")]
	     UNSPEC_LOADU)
	   (match_operand:SI 4 "const_0_to_255_operand" "n")]
	  UNSPEC_PCMPISTR))
   (set (match_operand:V16QI 1 "register_operand" "=Yz")
	(unspec:V16QI
	  [(match_dup 2)
	   (unspec:V16QI [(match_dup 3)] UNSPEC_LOADU)
	   (match_dup 4)]
	  UNSPEC_PCMPISTR))
   (set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_dup 2)
	   (unspec:V16QI [(match_dup 3)] UNSPEC_LOADU)
	   (match_dup 4)]
	  UNSPEC_PCMPISTR))]
  "TARGET_SSE4_2
   && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(const_int 0)]
{
  int ecx = !find_regno_note (curr_insn, REG_UNUSED, REGNO (operands[0]));
  int xmm0 = !find_regno_note (curr_insn, REG_UNUSED, REGNO (operands[1]));
  int flags = !find_regno_note (curr_insn, REG_UNUSED, FLAGS_REG);

  if (ecx)
    emit_insn (gen_sse4_2_pcmpistri (operands[0], operands[2],
				     operands[3], operands[4]));
  if (xmm0)
    emit_insn (gen_sse4_2_pcmpistrm (operands[1], operands[2],
				     operands[3], operands[4]));
  if (flags && !(ecx || xmm0))
    emit_insn (gen_sse4_2_pcmpistr_cconly (NULL, NULL,
					   operands[2], operands[3],
					   operands[4]));
  if (!(flags || ecx || xmm0))
    emit_note (NOTE_INSN_DELETED);

  DONE;
}
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "load")
   (set_attr "mode" "TI")])

(define_insn "sse4_2_pcmpistri"
  [(set (match_operand:SI 0 "register_operand" "=c,c")
	(unspec:SI
	  [(match_operand:V16QI 1 "register_operand" "x,x")
	   (match_operand:V16QI 2 "nonimmediate_operand" "x,m")
	   (match_operand:SI 3 "const_0_to_255_operand" "n,n")]
	  UNSPEC_PCMPISTR))
   (set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_dup 1)
	   (match_dup 2)
	   (match_dup 3)]
	  UNSPEC_PCMPISTR))]
  "TARGET_SSE4_2"
  "%vpcmpistri\t{%3, %2, %1|%1, %2, %3}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "memory" "none,load")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "TI")])

(define_insn "sse4_2_pcmpistrm"
  [(set (match_operand:V16QI 0 "register_operand" "=Yz,Yz")
	(unspec:V16QI
	  [(match_operand:V16QI 1 "register_operand" "x,x")
	   (match_operand:V16QI 2 "nonimmediate_operand" "x,m")
	   (match_operand:SI 3 "const_0_to_255_operand" "n,n")]
	  UNSPEC_PCMPISTR))
   (set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_dup 1)
	   (match_dup 2)
	   (match_dup 3)]
	  UNSPEC_PCMPISTR))]
  "TARGET_SSE4_2"
  "%vpcmpistrm\t{%3, %2, %1|%1, %2, %3}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "memory" "none,load")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "TI")])

(define_insn "sse4_2_pcmpistr_cconly"
  [(set (reg:CC FLAGS_REG)
	(unspec:CC
	  [(match_operand:V16QI 2 "register_operand" "x,x,x,x")
	   (match_operand:V16QI 3 "nonimmediate_operand" "x,m,x,m")
	   (match_operand:SI 4 "const_0_to_255_operand" "n,n,n,n")]
	  UNSPEC_PCMPISTR))
   (clobber (match_scratch:V16QI 0 "=Yz,Yz,X,X"))
   (clobber (match_scratch:SI    1 "= X, X,c,c"))]
  "TARGET_SSE4_2"
  "@
   %vpcmpistrm\t{%4, %3, %2|%2, %3, %4}
   %vpcmpistrm\t{%4, %3, %2|%2, %3, %4}
   %vpcmpistri\t{%4, %3, %2|%2, %3, %4}
   %vpcmpistri\t{%4, %3, %2|%2, %3, %4}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_data16" "1")
   (set_attr "prefix_extra" "1")
   (set_attr "ssememalign" "8")
   (set_attr "length_immediate" "1")
   (set_attr "memory" "none,load,none,load")
   (set_attr "prefix" "maybe_vex")
   (set_attr "btver2_decode" "vector,vector,vector,vector")
   (set_attr "mode" "TI")])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; XOP instructions
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_code_iterator xop_plus [plus ss_plus])

(define_code_attr macs [(plus "macs") (ss_plus "macss")])
(define_code_attr madcs [(plus "madcs") (ss_plus "madcss")])

;; XOP parallel integer multiply/add instructions.

(define_insn "xop_p<macs><ssemodesuffix><ssemodesuffix>"
  [(set (match_operand:VI24_128 0 "register_operand" "=x")
	(xop_plus:VI24_128
	 (mult:VI24_128
	  (match_operand:VI24_128 1 "nonimmediate_operand" "%x")
	  (match_operand:VI24_128 2 "nonimmediate_operand" "xm"))
	 (match_operand:VI24_128 3 "register_operand" "x")))]
  "TARGET_XOP"
  "vp<macs><ssemodesuffix><ssemodesuffix>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "TI")])

(define_insn "xop_p<macs>dql"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(xop_plus:V2DI
	 (mult:V2DI
	  (sign_extend:V2DI
	   (vec_select:V2SI
	    (match_operand:V4SI 1 "nonimmediate_operand" "%x")
	    (parallel [(const_int 0) (const_int 2)])))
	  (sign_extend:V2DI
	   (vec_select:V2SI
	    (match_operand:V4SI 2 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 2)]))))
	 (match_operand:V2DI 3 "register_operand" "x")))]
  "TARGET_XOP"
  "vp<macs>dql\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "TI")])

(define_insn "xop_p<macs>dqh"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(xop_plus:V2DI
	 (mult:V2DI
	  (sign_extend:V2DI
	   (vec_select:V2SI
	    (match_operand:V4SI 1 "nonimmediate_operand" "%x")
	    (parallel [(const_int 1) (const_int 3)])))
	  (sign_extend:V2DI
	   (vec_select:V2SI
	    (match_operand:V4SI 2 "nonimmediate_operand" "xm")
	    (parallel [(const_int 1) (const_int 3)]))))
	 (match_operand:V2DI 3 "register_operand" "x")))]
  "TARGET_XOP"
  "vp<macs>dqh\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "TI")])

;; XOP parallel integer multiply/add instructions for the intrinisics
(define_insn "xop_p<macs>wd"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(xop_plus:V4SI
	 (mult:V4SI
	  (sign_extend:V4SI
	   (vec_select:V4HI
	    (match_operand:V8HI 1 "nonimmediate_operand" "%x")
	    (parallel [(const_int 1) (const_int 3)
		       (const_int 5) (const_int 7)])))
	  (sign_extend:V4SI
	   (vec_select:V4HI
	    (match_operand:V8HI 2 "nonimmediate_operand" "xm")
	    (parallel [(const_int 1) (const_int 3)
		       (const_int 5) (const_int 7)]))))
	 (match_operand:V4SI 3 "register_operand" "x")))]
  "TARGET_XOP"
  "vp<macs>wd\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "TI")])

(define_insn "xop_p<madcs>wd"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(xop_plus:V4SI
	 (plus:V4SI
	  (mult:V4SI
	   (sign_extend:V4SI
	    (vec_select:V4HI
	     (match_operand:V8HI 1 "nonimmediate_operand" "%x")
	     (parallel [(const_int 0) (const_int 2)
			(const_int 4) (const_int 6)])))
	   (sign_extend:V4SI
	    (vec_select:V4HI
	     (match_operand:V8HI 2 "nonimmediate_operand" "xm")
	     (parallel [(const_int 0) (const_int 2)
			(const_int 4) (const_int 6)]))))
	  (mult:V4SI
	   (sign_extend:V4SI
	    (vec_select:V4HI
	     (match_dup 1)
	     (parallel [(const_int 1) (const_int 3)
			(const_int 5) (const_int 7)])))
	   (sign_extend:V4SI
	    (vec_select:V4HI
	     (match_dup 2)
	     (parallel [(const_int 1) (const_int 3)
			(const_int 5) (const_int 7)])))))
	 (match_operand:V4SI 3 "register_operand" "x")))]
  "TARGET_XOP"
  "vp<madcs>wd\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "ssemuladd")
   (set_attr "mode" "TI")])

;; XOP parallel XMM conditional moves
(define_insn "xop_pcmov_<mode><avxsizesuffix>"
  [(set (match_operand:V 0 "register_operand" "=x,x")
	(if_then_else:V
	  (match_operand:V 3 "nonimmediate_operand" "x,m")
	  (match_operand:V 1 "register_operand" "x,x")
	  (match_operand:V 2 "nonimmediate_operand" "xm,x")))]
  "TARGET_XOP"
  "vpcmov\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "sse4arg")])

;; XOP horizontal add/subtract instructions
(define_insn "xop_phadd<u>bw"
  [(set (match_operand:V8HI 0 "register_operand" "=x")
	(plus:V8HI
	 (any_extend:V8HI
	  (vec_select:V8QI
	   (match_operand:V16QI 1 "nonimmediate_operand" "xm")
	   (parallel [(const_int 0) (const_int 2)
		      (const_int 4) (const_int 6)
		      (const_int 8) (const_int 10)
		      (const_int 12) (const_int 14)])))
	 (any_extend:V8HI
	  (vec_select:V8QI
	   (match_dup 1)
	   (parallel [(const_int 1) (const_int 3)
		      (const_int 5) (const_int 7)
		      (const_int 9) (const_int 11)
		      (const_int 13) (const_int 15)])))))]
  "TARGET_XOP"
  "vphadd<u>bw\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseiadd1")])

(define_insn "xop_phadd<u>bd"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(plus:V4SI
	 (plus:V4SI
	  (any_extend:V4SI
	   (vec_select:V4QI
	    (match_operand:V16QI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 4)
		       (const_int 8) (const_int 12)])))
	  (any_extend:V4SI
	   (vec_select:V4QI
	    (match_dup 1)
	    (parallel [(const_int 1) (const_int 5)
		       (const_int 9) (const_int 13)]))))
	 (plus:V4SI
	  (any_extend:V4SI
	   (vec_select:V4QI
	    (match_dup 1)
	    (parallel [(const_int 2) (const_int 6)
		       (const_int 10) (const_int 14)])))
	  (any_extend:V4SI
	   (vec_select:V4QI
	    (match_dup 1)
	    (parallel [(const_int 3) (const_int 7)
		       (const_int 11) (const_int 15)]))))))]
  "TARGET_XOP"
  "vphadd<u>bd\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseiadd1")])

(define_insn "xop_phadd<u>bq"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(plus:V2DI
	 (plus:V2DI
	  (plus:V2DI
	   (any_extend:V2DI
	    (vec_select:V2QI
	     (match_operand:V16QI 1 "nonimmediate_operand" "xm")
	     (parallel [(const_int 0) (const_int 8)])))
	   (any_extend:V2DI
	    (vec_select:V2QI
	     (match_dup 1)
	     (parallel [(const_int 1) (const_int 9)]))))
	  (plus:V2DI
	   (any_extend:V2DI
	    (vec_select:V2QI
	     (match_dup 1)
	     (parallel [(const_int 2) (const_int 10)])))
	   (any_extend:V2DI
	    (vec_select:V2QI
	     (match_dup 1)
	     (parallel [(const_int 3) (const_int 11)])))))
	 (plus:V2DI
	  (plus:V2DI
	   (any_extend:V2DI
	    (vec_select:V2QI
	     (match_dup 1)
	     (parallel [(const_int 4) (const_int 12)])))
	   (any_extend:V2DI
	    (vec_select:V2QI
	     (match_dup 1)
	     (parallel [(const_int 5) (const_int 13)]))))
	  (plus:V2DI
	   (any_extend:V2DI
	    (vec_select:V2QI
	     (match_dup 1)
	     (parallel [(const_int 6) (const_int 14)])))
	   (any_extend:V2DI
	    (vec_select:V2QI
	     (match_dup 1)
	     (parallel [(const_int 7) (const_int 15)])))))))]
  "TARGET_XOP"
  "vphadd<u>bq\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseiadd1")])

(define_insn "xop_phadd<u>wd"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(plus:V4SI
	 (any_extend:V4SI
	  (vec_select:V4HI
	   (match_operand:V8HI 1 "nonimmediate_operand" "xm")
	   (parallel [(const_int 0) (const_int 2)
		      (const_int 4) (const_int 6)])))
	 (any_extend:V4SI
	  (vec_select:V4HI
	   (match_dup 1)
	   (parallel [(const_int 1) (const_int 3)
		      (const_int 5) (const_int 7)])))))]
  "TARGET_XOP"
  "vphadd<u>wd\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseiadd1")])

(define_insn "xop_phadd<u>wq"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(plus:V2DI
	 (plus:V2DI
	  (any_extend:V2DI
	   (vec_select:V2HI
	    (match_operand:V8HI 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0) (const_int 4)])))
	  (any_extend:V2DI
	   (vec_select:V2HI
	    (match_dup 1)
	    (parallel [(const_int 1) (const_int 5)]))))
	 (plus:V2DI
	  (any_extend:V2DI
	   (vec_select:V2HI
	    (match_dup 1)
	    (parallel [(const_int 2) (const_int 6)])))
	  (any_extend:V2DI
	   (vec_select:V2HI
	    (match_dup 1)
	    (parallel [(const_int 3) (const_int 7)]))))))]
  "TARGET_XOP"
  "vphadd<u>wq\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseiadd1")])

(define_insn "xop_phadd<u>dq"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(plus:V2DI
	 (any_extend:V2DI
	  (vec_select:V2SI
	   (match_operand:V4SI 1 "nonimmediate_operand" "xm")
	   (parallel [(const_int 0) (const_int 2)])))
	 (any_extend:V2DI
	  (vec_select:V2SI
	   (match_dup 1)
	   (parallel [(const_int 1) (const_int 3)])))))]
  "TARGET_XOP"
  "vphadd<u>dq\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseiadd1")])

(define_insn "xop_phsubbw"
  [(set (match_operand:V8HI 0 "register_operand" "=x")
	(minus:V8HI
	 (sign_extend:V8HI
	  (vec_select:V8QI
	   (match_operand:V16QI 1 "nonimmediate_operand" "xm")
	   (parallel [(const_int 0) (const_int 2)
		      (const_int 4) (const_int 6)
		      (const_int 8) (const_int 10)
		      (const_int 12) (const_int 14)])))
	 (sign_extend:V8HI
	  (vec_select:V8QI
	   (match_dup 1)
	   (parallel [(const_int 1) (const_int 3)
		      (const_int 5) (const_int 7)
		      (const_int 9) (const_int 11)
		      (const_int 13) (const_int 15)])))))]
  "TARGET_XOP"
  "vphsubbw\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseiadd1")])

(define_insn "xop_phsubwd"
  [(set (match_operand:V4SI 0 "register_operand" "=x")
	(minus:V4SI
	 (sign_extend:V4SI
	  (vec_select:V4HI
	   (match_operand:V8HI 1 "nonimmediate_operand" "xm")
	   (parallel [(const_int 0) (const_int 2)
		      (const_int 4) (const_int 6)])))
	 (sign_extend:V4SI
	  (vec_select:V4HI
	   (match_dup 1)
	   (parallel [(const_int 1) (const_int 3)
		      (const_int 5) (const_int 7)])))))]
  "TARGET_XOP"
  "vphsubwd\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseiadd1")])

(define_insn "xop_phsubdq"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(minus:V2DI
	 (sign_extend:V2DI
	  (vec_select:V2SI
	   (match_operand:V4SI 1 "nonimmediate_operand" "xm")
	   (parallel [(const_int 0) (const_int 2)])))
	 (sign_extend:V2DI
	  (vec_select:V2SI
	   (match_dup 1)
	   (parallel [(const_int 1) (const_int 3)])))))]
  "TARGET_XOP"
  "vphsubdq\t{%1, %0|%0, %1}"
  [(set_attr "type" "sseiadd1")])

;; XOP permute instructions
(define_insn "xop_pperm"
  [(set (match_operand:V16QI 0 "register_operand" "=x,x")
	(unspec:V16QI
	  [(match_operand:V16QI 1 "register_operand" "x,x")
	   (match_operand:V16QI 2 "nonimmediate_operand" "x,m")
	   (match_operand:V16QI 3 "nonimmediate_operand" "xm,x")]
	  UNSPEC_XOP_PERMUTE))]
  "TARGET_XOP && !(MEM_P (operands[2]) && MEM_P (operands[3]))"
  "vpperm\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "sse4arg")
   (set_attr "mode" "TI")])

;; XOP pack instructions that combine two vectors into a smaller vector
(define_insn "xop_pperm_pack_v2di_v4si"
  [(set (match_operand:V4SI 0 "register_operand" "=x,x")
	(vec_concat:V4SI
	 (truncate:V2SI
	  (match_operand:V2DI 1 "register_operand" "x,x"))
	 (truncate:V2SI
	  (match_operand:V2DI 2 "nonimmediate_operand" "x,m"))))
   (use (match_operand:V16QI 3 "nonimmediate_operand" "xm,x"))]
  "TARGET_XOP && !(MEM_P (operands[2]) && MEM_P (operands[3]))"
  "vpperm\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "sse4arg")
   (set_attr "mode" "TI")])

(define_insn "xop_pperm_pack_v4si_v8hi"
  [(set (match_operand:V8HI 0 "register_operand" "=x,x")
	(vec_concat:V8HI
	 (truncate:V4HI
	  (match_operand:V4SI 1 "register_operand" "x,x"))
	 (truncate:V4HI
	  (match_operand:V4SI 2 "nonimmediate_operand" "x,m"))))
   (use (match_operand:V16QI 3 "nonimmediate_operand" "xm,x"))]
  "TARGET_XOP && !(MEM_P (operands[2]) && MEM_P (operands[3]))"
  "vpperm\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "sse4arg")
   (set_attr "mode" "TI")])

(define_insn "xop_pperm_pack_v8hi_v16qi"
  [(set (match_operand:V16QI 0 "register_operand" "=x,x")
	(vec_concat:V16QI
	 (truncate:V8QI
	  (match_operand:V8HI 1 "register_operand" "x,x"))
	 (truncate:V8QI
	  (match_operand:V8HI 2 "nonimmediate_operand" "x,m"))))
   (use (match_operand:V16QI 3 "nonimmediate_operand" "xm,x"))]
  "TARGET_XOP && !(MEM_P (operands[2]) && MEM_P (operands[3]))"
  "vpperm\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "sse4arg")
   (set_attr "mode" "TI")])

;; XOP packed rotate instructions
(define_expand "rotl<mode>3"
  [(set (match_operand:VI_128 0 "register_operand")
	(rotate:VI_128
	 (match_operand:VI_128 1 "nonimmediate_operand")
	 (match_operand:SI 2 "general_operand")))]
  "TARGET_XOP"
{
  /* If we were given a scalar, convert it to parallel */
  if (! const_0_to_<sserotatemax>_operand (operands[2], SImode))
    {
      rtvec vs = rtvec_alloc (<ssescalarnum>);
      rtx par = gen_rtx_PARALLEL (<MODE>mode, vs);
      rtx reg = gen_reg_rtx (<MODE>mode);
      rtx op2 = operands[2];
      int i;

      if (GET_MODE (op2) != <ssescalarmode>mode)
	{
	  op2 = gen_reg_rtx (<ssescalarmode>mode);
	  convert_move (op2, operands[2], false);
	}

      for (i = 0; i < <ssescalarnum>; i++)
	RTVEC_ELT (vs, i) = op2;

      emit_insn (gen_vec_init<mode> (reg, par));
      emit_insn (gen_xop_vrotl<mode>3 (operands[0], operands[1], reg));
      DONE;
    }
})

(define_expand "rotr<mode>3"
  [(set (match_operand:VI_128 0 "register_operand")
	(rotatert:VI_128
	 (match_operand:VI_128 1 "nonimmediate_operand")
	 (match_operand:SI 2 "general_operand")))]
  "TARGET_XOP"
{
  /* If we were given a scalar, convert it to parallel */
  if (! const_0_to_<sserotatemax>_operand (operands[2], SImode))
    {
      rtvec vs = rtvec_alloc (<ssescalarnum>);
      rtx par = gen_rtx_PARALLEL (<MODE>mode, vs);
      rtx neg = gen_reg_rtx (<MODE>mode);
      rtx reg = gen_reg_rtx (<MODE>mode);
      rtx op2 = operands[2];
      int i;

      if (GET_MODE (op2) != <ssescalarmode>mode)
	{
	  op2 = gen_reg_rtx (<ssescalarmode>mode);
	  convert_move (op2, operands[2], false);
	}

      for (i = 0; i < <ssescalarnum>; i++)
	RTVEC_ELT (vs, i) = op2;

      emit_insn (gen_vec_init<mode> (reg, par));
      emit_insn (gen_neg<mode>2 (neg, reg));
      emit_insn (gen_xop_vrotl<mode>3 (operands[0], operands[1], neg));
      DONE;
    }
})

(define_insn "xop_rotl<mode>3"
  [(set (match_operand:VI_128 0 "register_operand" "=x")
	(rotate:VI_128
	 (match_operand:VI_128 1 "nonimmediate_operand" "xm")
	 (match_operand:SI 2 "const_0_to_<sserotatemax>_operand" "n")))]
  "TARGET_XOP"
  "vprot<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseishft")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "TI")])

(define_insn "xop_rotr<mode>3"
  [(set (match_operand:VI_128 0 "register_operand" "=x")
	(rotatert:VI_128
	 (match_operand:VI_128 1 "nonimmediate_operand" "xm")
	 (match_operand:SI 2 "const_0_to_<sserotatemax>_operand" "n")))]
  "TARGET_XOP"
{
  operands[3]
    = GEN_INT (GET_MODE_BITSIZE (<ssescalarmode>mode) - INTVAL (operands[2]));
  return \"vprot<ssemodesuffix>\t{%3, %1, %0|%0, %1, %3}\";
}
  [(set_attr "type" "sseishft")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "TI")])

(define_expand "vrotr<mode>3"
  [(match_operand:VI_128 0 "register_operand")
   (match_operand:VI_128 1 "register_operand")
   (match_operand:VI_128 2 "register_operand")]
  "TARGET_XOP"
{
  rtx reg = gen_reg_rtx (<MODE>mode);
  emit_insn (gen_neg<mode>2 (reg, operands[2]));
  emit_insn (gen_xop_vrotl<mode>3 (operands[0], operands[1], reg));
  DONE;
})

(define_expand "vrotl<mode>3"
  [(match_operand:VI_128 0 "register_operand")
   (match_operand:VI_128 1 "register_operand")
   (match_operand:VI_128 2 "register_operand")]
  "TARGET_XOP"
{
  emit_insn (gen_xop_vrotl<mode>3 (operands[0], operands[1], operands[2]));
  DONE;
})

(define_insn "xop_vrotl<mode>3"
  [(set (match_operand:VI_128 0 "register_operand" "=x,x")
	(if_then_else:VI_128
	 (ge:VI_128
	  (match_operand:VI_128 2 "nonimmediate_operand" "x,m")
	  (const_int 0))
	 (rotate:VI_128
	  (match_operand:VI_128 1 "nonimmediate_operand" "xm,x")
	  (match_dup 2))
	 (rotatert:VI_128
	  (match_dup 1)
	  (neg:VI_128 (match_dup 2)))))]
  "TARGET_XOP && !(MEM_P (operands[1]) && MEM_P (operands[2]))"
  "vprot<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseishft")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_extra" "2")
   (set_attr "mode" "TI")])

;; XOP packed shift instructions.
(define_expand "vlshr<mode>3"
  [(set (match_operand:VI12_128 0 "register_operand")
	(lshiftrt:VI12_128
	  (match_operand:VI12_128 1 "register_operand")
	  (match_operand:VI12_128 2 "nonimmediate_operand")))]
  "TARGET_XOP"
{
  rtx neg = gen_reg_rtx (<MODE>mode);
  emit_insn (gen_neg<mode>2 (neg, operands[2]));
  emit_insn (gen_xop_shl<mode>3 (operands[0], operands[1], neg));
  DONE;
})

(define_expand "vlshr<mode>3"
  [(set (match_operand:VI48_128 0 "register_operand")
	(lshiftrt:VI48_128
	  (match_operand:VI48_128 1 "register_operand")
	  (match_operand:VI48_128 2 "nonimmediate_operand")))]
  "TARGET_AVX2 || TARGET_XOP"
{
  if (!TARGET_AVX2)
    {
      rtx neg = gen_reg_rtx (<MODE>mode);
      emit_insn (gen_neg<mode>2 (neg, operands[2]));
      emit_insn (gen_xop_shl<mode>3 (operands[0], operands[1], neg));
      DONE;
    }
})

(define_expand "vlshr<mode>3"
  [(set (match_operand:VI48_256 0 "register_operand")
	(lshiftrt:VI48_256
	  (match_operand:VI48_256 1 "register_operand")
	  (match_operand:VI48_256 2 "nonimmediate_operand")))]
  "TARGET_AVX2")

(define_expand "vashr<mode>3"
  [(set (match_operand:VI128_128 0 "register_operand")
	(ashiftrt:VI128_128
	  (match_operand:VI128_128 1 "register_operand")
	  (match_operand:VI128_128 2 "nonimmediate_operand")))]
  "TARGET_XOP"
{
  rtx neg = gen_reg_rtx (<MODE>mode);
  emit_insn (gen_neg<mode>2 (neg, operands[2]));
  emit_insn (gen_xop_sha<mode>3 (operands[0], operands[1], neg));
  DONE;
})

(define_expand "vashrv4si3"
  [(set (match_operand:V4SI 0 "register_operand")
	(ashiftrt:V4SI (match_operand:V4SI 1 "register_operand")
		       (match_operand:V4SI 2 "nonimmediate_operand")))]
  "TARGET_AVX2 || TARGET_XOP"
{
  if (!TARGET_AVX2)
    {
      rtx neg = gen_reg_rtx (V4SImode);
      emit_insn (gen_negv4si2 (neg, operands[2]));
      emit_insn (gen_xop_shav4si3 (operands[0], operands[1], neg));
      DONE;
    }
})

(define_expand "vashrv8si3"
  [(set (match_operand:V8SI 0 "register_operand")
	(ashiftrt:V8SI (match_operand:V8SI 1 "register_operand")
		       (match_operand:V8SI 2 "nonimmediate_operand")))]
  "TARGET_AVX2")

(define_expand "vashl<mode>3"
  [(set (match_operand:VI12_128 0 "register_operand")
	(ashift:VI12_128
	  (match_operand:VI12_128 1 "register_operand")
	  (match_operand:VI12_128 2 "nonimmediate_operand")))]
  "TARGET_XOP"
{
  emit_insn (gen_xop_sha<mode>3 (operands[0], operands[1], operands[2]));
  DONE;
})

(define_expand "vashl<mode>3"
  [(set (match_operand:VI48_128 0 "register_operand")
	(ashift:VI48_128
	  (match_operand:VI48_128 1 "register_operand")
	  (match_operand:VI48_128 2 "nonimmediate_operand")))]
  "TARGET_AVX2 || TARGET_XOP"
{
  if (!TARGET_AVX2)
    {
      operands[2] = force_reg (<MODE>mode, operands[2]);
      emit_insn (gen_xop_sha<mode>3 (operands[0], operands[1], operands[2]));
      DONE;
    }
})

(define_expand "vashl<mode>3"
  [(set (match_operand:VI48_256 0 "register_operand")
	(ashift:VI48_256
	  (match_operand:VI48_256 1 "register_operand")
	  (match_operand:VI48_256 2 "nonimmediate_operand")))]
  "TARGET_AVX2")

(define_insn "xop_sha<mode>3"
  [(set (match_operand:VI_128 0 "register_operand" "=x,x")
	(if_then_else:VI_128
	 (ge:VI_128
	  (match_operand:VI_128 2 "nonimmediate_operand" "x,m")
	  (const_int 0))
	 (ashift:VI_128
	  (match_operand:VI_128 1 "nonimmediate_operand" "xm,x")
	  (match_dup 2))
	 (ashiftrt:VI_128
	  (match_dup 1)
	  (neg:VI_128 (match_dup 2)))))]
  "TARGET_XOP && !(MEM_P (operands[1]) && MEM_P (operands[2]))"
  "vpsha<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseishft")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_extra" "2")
   (set_attr "mode" "TI")])

(define_insn "xop_shl<mode>3"
  [(set (match_operand:VI_128 0 "register_operand" "=x,x")
	(if_then_else:VI_128
	 (ge:VI_128
	  (match_operand:VI_128 2 "nonimmediate_operand" "x,m")
	  (const_int 0))
	 (ashift:VI_128
	  (match_operand:VI_128 1 "nonimmediate_operand" "xm,x")
	  (match_dup 2))
	 (lshiftrt:VI_128
	  (match_dup 1)
	  (neg:VI_128 (match_dup 2)))))]
  "TARGET_XOP && !(MEM_P (operands[1]) && MEM_P (operands[2]))"
  "vpshl<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseishft")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_extra" "2")
   (set_attr "mode" "TI")])

(define_expand "<shift_insn><mode>3"
  [(set (match_operand:VI1_AVX2 0 "register_operand")
	(any_shift:VI1_AVX2
	  (match_operand:VI1_AVX2 1 "register_operand")
	  (match_operand:SI 2 "nonmemory_operand")))]
  "TARGET_SSE2"
{
  if (TARGET_XOP && <MODE>mode == V16QImode)
    {
      bool negate = false;
      rtx (*gen) (rtx, rtx, rtx);
      rtx tmp, par;
      int i;

      if (<CODE> != ASHIFT)
	{
	  if (CONST_INT_P (operands[2]))
	    operands[2] = GEN_INT (-INTVAL (operands[2]));
	  else
	    negate = true;
	}
      par = gen_rtx_PARALLEL (V16QImode, rtvec_alloc (16));
      for (i = 0; i < 16; i++)
        XVECEXP (par, 0, i) = operands[2];

      tmp = gen_reg_rtx (V16QImode);
      emit_insn (gen_vec_initv16qi (tmp, par));

      if (negate)
	emit_insn (gen_negv16qi2 (tmp, tmp));

      gen = (<CODE> == LSHIFTRT ? gen_xop_shlv16qi3 : gen_xop_shav16qi3);
      emit_insn (gen (operands[0], operands[1], tmp));
    }
  else
    ix86_expand_vecop_qihi (<CODE>, operands[0], operands[1], operands[2]);
  DONE;
})

(define_expand "ashrv2di3"
  [(set (match_operand:V2DI 0 "register_operand")
	(ashiftrt:V2DI
	  (match_operand:V2DI 1 "register_operand")
	  (match_operand:DI 2 "nonmemory_operand")))]
  "TARGET_XOP"
{
  rtx reg = gen_reg_rtx (V2DImode);
  rtx par;
  bool negate = false;
  int i;

  if (CONST_INT_P (operands[2]))
    operands[2] = GEN_INT (-INTVAL (operands[2]));
  else
    negate = true;

  par = gen_rtx_PARALLEL (V2DImode, rtvec_alloc (2));
  for (i = 0; i < 2; i++)
    XVECEXP (par, 0, i) = operands[2];

  emit_insn (gen_vec_initv2di (reg, par));

  if (negate)
    emit_insn (gen_negv2di2 (reg, reg));

  emit_insn (gen_xop_shav2di3 (operands[0], operands[1], reg));
  DONE;
})

;; XOP FRCZ support
(define_insn "xop_frcz<mode>2"
  [(set (match_operand:FMAMODE 0 "register_operand" "=x")
	(unspec:FMAMODE
	 [(match_operand:FMAMODE 1 "nonimmediate_operand" "xm")]
	 UNSPEC_FRCZ))]
  "TARGET_XOP"
  "vfrcz<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt1")
   (set_attr "mode" "<MODE>")])

(define_expand "xop_vmfrcz<mode>2"
  [(set (match_operand:VF_128 0 "register_operand")
	(vec_merge:VF_128
	  (unspec:VF_128
	   [(match_operand:VF_128 1 "nonimmediate_operand")]
	   UNSPEC_FRCZ)
	  (match_dup 3)
	  (const_int 1)))]
  "TARGET_XOP"
  "operands[3] = CONST0_RTX (<MODE>mode);")

(define_insn "*xop_vmfrcz<mode>2"
  [(set (match_operand:VF_128 0 "register_operand" "=x")
	(vec_merge:VF_128
	  (unspec:VF_128
	   [(match_operand:VF_128 1 "nonimmediate_operand" "xm")]
	   UNSPEC_FRCZ)
	  (match_operand:VF_128 2 "const0_operand")
	  (const_int 1)))]
  "TARGET_XOP"
  "vfrcz<ssescalarmodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt1")
   (set_attr "mode" "<MODE>")])

(define_insn "xop_maskcmp<mode>3"
  [(set (match_operand:VI_128 0 "register_operand" "=x")
	(match_operator:VI_128 1 "ix86_comparison_int_operator"
	 [(match_operand:VI_128 2 "register_operand" "x")
	  (match_operand:VI_128 3 "nonimmediate_operand" "xm")]))]
  "TARGET_XOP"
  "vpcom%Y1<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}"
  [(set_attr "type" "sse4arg")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_rep" "0")
   (set_attr "prefix_extra" "2")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "TI")])

(define_insn "xop_maskcmp_uns<mode>3"
  [(set (match_operand:VI_128 0 "register_operand" "=x")
	(match_operator:VI_128 1 "ix86_comparison_uns_operator"
	 [(match_operand:VI_128 2 "register_operand" "x")
	  (match_operand:VI_128 3 "nonimmediate_operand" "xm")]))]
  "TARGET_XOP"
  "vpcom%Y1u<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}"
  [(set_attr "type" "ssecmp")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_rep" "0")
   (set_attr "prefix_extra" "2")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "TI")])

;; Version of pcom*u* that is called from the intrinsics that allows pcomequ*
;; and pcomneu* not to be converted to the signed ones in case somebody needs
;; the exact instruction generated for the intrinsic.
(define_insn "xop_maskcmp_uns2<mode>3"
  [(set (match_operand:VI_128 0 "register_operand" "=x")
	(unspec:VI_128
	 [(match_operator:VI_128 1 "ix86_comparison_uns_operator"
	  [(match_operand:VI_128 2 "register_operand" "x")
	   (match_operand:VI_128 3 "nonimmediate_operand" "xm")])]
	 UNSPEC_XOP_UNSIGNED_CMP))]
  "TARGET_XOP"
  "vpcom%Y1u<ssemodesuffix>\t{%3, %2, %0|%0, %2, %3}"
  [(set_attr "type" "ssecmp")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_extra" "2")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "TI")])

;; Pcomtrue and pcomfalse support.  These are useless instructions, but are
;; being added here to be complete.
(define_insn "xop_pcom_tf<mode>3"
  [(set (match_operand:VI_128 0 "register_operand" "=x")
	(unspec:VI_128
	  [(match_operand:VI_128 1 "register_operand" "x")
	   (match_operand:VI_128 2 "nonimmediate_operand" "xm")
	   (match_operand:SI 3 "const_int_operand" "n")]
	  UNSPEC_XOP_TRUEFALSE))]
  "TARGET_XOP"
{
  return ((INTVAL (operands[3]) != 0)
	  ? "vpcomtrue<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
	  : "vpcomfalse<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}");
}
  [(set_attr "type" "ssecmp")
   (set_attr "prefix_data16" "0")
   (set_attr "prefix_extra" "2")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "TI")])

(define_insn "xop_vpermil2<mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x")
	(unspec:VF
	  [(match_operand:VF 1 "register_operand" "x")
	   (match_operand:VF 2 "nonimmediate_operand" "%x")
	   (match_operand:<sseintvecmode> 3 "nonimmediate_operand" "xm")
	   (match_operand:SI 4 "const_0_to_3_operand" "n")]
	  UNSPEC_VPERMIL2))]
  "TARGET_XOP"
  "vpermil2<ssemodesuffix>\t{%4, %3, %2, %1, %0|%0, %1, %2, %3, %4}"
  [(set_attr "type" "sse4arg")
   (set_attr "length_immediate" "1")
   (set_attr "mode" "<MODE>")])

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define_insn "aesenc"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(unspec:V2DI [(match_operand:V2DI 1 "register_operand" "0,x")
		       (match_operand:V2DI 2 "nonimmediate_operand" "xm,xm")]
		      UNSPEC_AESENC))]
  "TARGET_AES"
  "@
   aesenc\t{%2, %0|%0, %2}
   vaesenc\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "double,double")
   (set_attr "mode" "TI")])

(define_insn "aesenclast"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(unspec:V2DI [(match_operand:V2DI 1 "register_operand" "0,x")
		       (match_operand:V2DI 2 "nonimmediate_operand" "xm,xm")]
		      UNSPEC_AESENCLAST))]
  "TARGET_AES"
  "@
   aesenclast\t{%2, %0|%0, %2}
   vaesenclast\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "double,double") 
   (set_attr "mode" "TI")])

(define_insn "aesdec"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(unspec:V2DI [(match_operand:V2DI 1 "register_operand" "0,x")
		       (match_operand:V2DI 2 "nonimmediate_operand" "xm,xm")]
		      UNSPEC_AESDEC))]
  "TARGET_AES"
  "@
   aesdec\t{%2, %0|%0, %2}
   vaesdec\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "double,double") 
   (set_attr "mode" "TI")])

(define_insn "aesdeclast"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(unspec:V2DI [(match_operand:V2DI 1 "register_operand" "0,x")
		       (match_operand:V2DI 2 "nonimmediate_operand" "xm,xm")]
		      UNSPEC_AESDECLAST))]
  "TARGET_AES"
  "@
   aesdeclast\t{%2, %0|%0, %2}
   vaesdeclast\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "btver2_decode" "double,double")
   (set_attr "mode" "TI")])

(define_insn "aesimc"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(unspec:V2DI [(match_operand:V2DI 1 "nonimmediate_operand" "xm")]
		      UNSPEC_AESIMC))]
  "TARGET_AES"
  "%vaesimc\t{%1, %0|%0, %1}"
  [(set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "aeskeygenassist"
  [(set (match_operand:V2DI 0 "register_operand" "=x")
	(unspec:V2DI [(match_operand:V2DI 1 "nonimmediate_operand" "xm")
		      (match_operand:SI 2 "const_0_to_255_operand" "n")]
		     UNSPEC_AESKEYGENASSIST))]
  "TARGET_AES"
  "%vaeskeygenassist\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "maybe_vex")
   (set_attr "mode" "TI")])

(define_insn "pclmulqdq"
  [(set (match_operand:V2DI 0 "register_operand" "=x,x")
	(unspec:V2DI [(match_operand:V2DI 1 "register_operand" "0,x")
		      (match_operand:V2DI 2 "nonimmediate_operand" "xm,xm")
		      (match_operand:SI 3 "const_0_to_255_operand" "n,n")]
		     UNSPEC_PCLMUL))]
  "TARGET_PCLMUL"
  "@
   pclmulqdq\t{%3, %2, %0|%0, %2, %3}
   vpclmulqdq\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "isa" "noavx,avx")
   (set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "orig,vex")
   (set_attr "mode" "TI")])

(define_expand "avx_vzeroall"
  [(match_par_dup 0 [(const_int 0)])]
  "TARGET_AVX"
{
  int nregs = TARGET_64BIT ? 16 : 8;
  int regno;

  operands[0] = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc (nregs + 1));

  XVECEXP (operands[0], 0, 0)
    = gen_rtx_UNSPEC_VOLATILE (VOIDmode, gen_rtvec (1, const0_rtx),
			       UNSPECV_VZEROALL);

  for (regno = 0; regno < nregs; regno++)
    XVECEXP (operands[0], 0, regno + 1)
      = gen_rtx_SET (VOIDmode,
		     gen_rtx_REG (V8SImode, SSE_REGNO (regno)),
		     CONST0_RTX (V8SImode));
})

(define_insn "*avx_vzeroall"
  [(match_parallel 0 "vzeroall_operation"
    [(unspec_volatile [(const_int 0)] UNSPECV_VZEROALL)])]
  "TARGET_AVX"
  "vzeroall"
  [(set_attr "type" "sse")
   (set_attr "modrm" "0")
   (set_attr "memory" "none")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "OI")])

;; Clear the upper 128bits of AVX registers, equivalent to a NOP
;; if the upper 128bits are unused.
(define_insn "avx_vzeroupper"
  [(unspec_volatile [(const_int 0)] UNSPECV_VZEROUPPER)]
  "TARGET_AVX"
  "vzeroupper"
  [(set_attr "type" "sse")
   (set_attr "modrm" "0")
   (set_attr "memory" "none")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "OI")])

(define_mode_attr AVXTOSSEMODE
  [(V4DI "V2DI") (V2DI "V2DI")
   (V8SI "V4SI") (V4SI "V4SI")
   (V16HI "V8HI") (V8HI "V8HI")
   (V32QI "V16QI") (V16QI "V16QI")])

(define_insn "avx2_pbroadcast<mode>"
  [(set (match_operand:VI 0 "register_operand" "=x")
	(vec_duplicate:VI
	  (vec_select:<ssescalarmode>
	    (match_operand:<AVXTOSSEMODE> 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0)]))))]
  "TARGET_AVX2"
  "vpbroadcast<ssemodesuffix>\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "avx2_pbroadcast<mode>_1"
  [(set (match_operand:VI_256 0 "register_operand" "=x")
	(vec_duplicate:VI_256
	  (vec_select:<ssescalarmode>
	    (match_operand:VI_256 1 "nonimmediate_operand" "xm")
	    (parallel [(const_int 0)]))))]
  "TARGET_AVX2"
  "vpbroadcast<ssemodesuffix>\t{%x1, %0|%0, %x1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "avx2_permvar<mode>"
  [(set (match_operand:VI4F_256 0 "register_operand" "=x")
	(unspec:VI4F_256
	  [(match_operand:VI4F_256 1 "nonimmediate_operand" "xm")
	   (match_operand:V8SI 2 "register_operand" "x")]
	  UNSPEC_VPERMVAR))]
  "TARGET_AVX2"
  "vperm<ssemodesuffix>\t{%1, %2, %0|%0, %2, %1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_expand "avx2_perm<mode>"
  [(match_operand:VI8F_256 0 "register_operand")
   (match_operand:VI8F_256 1 "nonimmediate_operand")
   (match_operand:SI 2 "const_0_to_255_operand")]
  "TARGET_AVX2"
{
  int mask = INTVAL (operands[2]);
  emit_insn (gen_avx2_perm<mode>_1 (operands[0], operands[1],
				    GEN_INT ((mask >> 0) & 3),
				    GEN_INT ((mask >> 2) & 3),
				    GEN_INT ((mask >> 4) & 3),
				    GEN_INT ((mask >> 6) & 3)));
  DONE;
})

(define_insn "avx2_perm<mode>_1"
  [(set (match_operand:VI8F_256 0 "register_operand" "=x")
	(vec_select:VI8F_256
	  (match_operand:VI8F_256 1 "nonimmediate_operand" "xm")
	  (parallel [(match_operand 2 "const_0_to_3_operand")
		     (match_operand 3 "const_0_to_3_operand")
		     (match_operand 4 "const_0_to_3_operand")
		     (match_operand 5 "const_0_to_3_operand")])))]
  "TARGET_AVX2"
{
  int mask = 0;
  mask |= INTVAL (operands[2]) << 0;
  mask |= INTVAL (operands[3]) << 2;
  mask |= INTVAL (operands[4]) << 4;
  mask |= INTVAL (operands[5]) << 6;
  operands[2] = GEN_INT (mask);
  return "vperm<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}";
}
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "avx2_permv2ti"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(unspec:V4DI
	  [(match_operand:V4DI 1 "register_operand" "x")
	   (match_operand:V4DI 2 "nonimmediate_operand" "xm")
	   (match_operand:SI 3 "const_0_to_255_operand" "n")]
	  UNSPEC_VPERMTI))]
  "TARGET_AVX2"
  "vperm2i128\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "sselog")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "avx2_vec_dupv4df"
  [(set (match_operand:V4DF 0 "register_operand" "=x")
	(vec_duplicate:V4DF
	  (vec_select:DF
	    (match_operand:V2DF 1 "register_operand" "x")
	    (parallel [(const_int 0)]))))]
  "TARGET_AVX2"
  "vbroadcastsd\t{%1, %0|%0, %1}"
  [(set_attr "type" "sselog1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4DF")])

;; Modes handled by AVX vec_dup patterns.
(define_mode_iterator AVX_VEC_DUP_MODE
  [V8SI V8SF V4DI V4DF])

(define_insn "vec_dup<mode>"
  [(set (match_operand:AVX_VEC_DUP_MODE 0 "register_operand" "=x,x,x")
	(vec_duplicate:AVX_VEC_DUP_MODE
	  (match_operand:<ssescalarmode> 1 "nonimmediate_operand" "m,x,?x")))]
  "TARGET_AVX"
  "@
   vbroadcast<ssescalarmodesuffix>\t{%1, %0|%0, %1}
   vbroadcast<ssescalarmodesuffix>\t{%x1, %0|%0, %x1}
   #"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "isa" "*,avx2,noavx2")
   (set_attr "mode" "V8SF")])

(define_insn "avx2_vbroadcasti128_<mode>"
  [(set (match_operand:VI_256 0 "register_operand" "=x")
	(vec_concat:VI_256
	  (match_operand:<ssehalfvecmode> 1 "memory_operand" "m")
	  (match_dup 1)))]
  "TARGET_AVX2"
  "vbroadcasti128\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_split
  [(set (match_operand:AVX_VEC_DUP_MODE 0 "register_operand")
	(vec_duplicate:AVX_VEC_DUP_MODE
	  (match_operand:<ssescalarmode> 1 "register_operand")))]
  "TARGET_AVX && !TARGET_AVX2 && reload_completed"
  [(set (match_dup 2)
	(vec_duplicate:<ssehalfvecmode> (match_dup 1)))
   (set (match_dup 0)
	(vec_concat:AVX_VEC_DUP_MODE (match_dup 2) (match_dup 2)))]
  "operands[2] = gen_rtx_REG (<ssehalfvecmode>mode, REGNO (operands[0]));")

(define_insn "avx_vbroadcastf128_<mode>"
  [(set (match_operand:V_256 0 "register_operand" "=x,x,x")
	(vec_concat:V_256
	  (match_operand:<ssehalfvecmode> 1 "nonimmediate_operand" "m,0,?x")
	  (match_dup 1)))]
  "TARGET_AVX"
  "@
   vbroadcast<i128>\t{%1, %0|%0, %1}
   vinsert<i128>\t{$1, %1, %0, %0|%0, %0, %1, 1}
   vperm2<i128>\t{$0, %t1, %t1, %0|%0, %t1, %t1, 0}"
  [(set_attr "type" "ssemov,sselog1,sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "0,1,1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

;; Recognize broadcast as a vec_select as produced by builtin_vec_perm.
;; If it so happens that the input is in memory, use vbroadcast.
;; Otherwise use vpermilp (and in the case of 256-bit modes, vperm2f128).
(define_insn "*avx_vperm_broadcast_v4sf"
  [(set (match_operand:V4SF 0 "register_operand" "=x,x,x")
	(vec_select:V4SF
	  (match_operand:V4SF 1 "nonimmediate_operand" "m,o,x")
	  (match_parallel 2 "avx_vbroadcast_operand"
	    [(match_operand 3 "const_int_operand" "C,n,n")])))]
  "TARGET_AVX"
{
  int elt = INTVAL (operands[3]);
  switch (which_alternative)
    {
    case 0:
    case 1:
      operands[1] = adjust_address_nv (operands[1], SFmode, elt * 4);
      return "vbroadcastss\t{%1, %0|%0, %1}";
    case 2:
      operands[2] = GEN_INT (elt * 0x55);
      return "vpermilps\t{%2, %1, %0|%0, %1, %2}";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "type" "ssemov,ssemov,sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "0,0,1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "SF,SF,V4SF")])

(define_insn_and_split "*avx_vperm_broadcast_<mode>"
  [(set (match_operand:VF_256 0 "register_operand" "=x,x,x")
	(vec_select:VF_256
	  (match_operand:VF_256 1 "nonimmediate_operand" "m,o,?x")
	  (match_parallel 2 "avx_vbroadcast_operand"
	    [(match_operand 3 "const_int_operand" "C,n,n")])))]
  "TARGET_AVX"
  "#"
  "&& reload_completed && (<MODE>mode != V4DFmode || !TARGET_AVX2)"
  [(set (match_dup 0) (vec_duplicate:VF_256 (match_dup 1)))]
{
  rtx op0 = operands[0], op1 = operands[1];
  int elt = INTVAL (operands[3]);

  if (REG_P (op1))
    {
      int mask;

      if (TARGET_AVX2 && elt == 0)
	{
	  emit_insn (gen_vec_dup<mode> (op0, gen_lowpart (<ssescalarmode>mode,
							  op1)));
	  DONE;
	}

      /* Shuffle element we care about into all elements of the 128-bit lane.
	 The other lane gets shuffled too, but we don't care.  */
      if (<MODE>mode == V4DFmode)
	mask = (elt & 1 ? 15 : 0);
      else
	mask = (elt & 3) * 0x55;
      emit_insn (gen_avx_vpermil<mode> (op0, op1, GEN_INT (mask)));

      /* Shuffle the lane we care about into both lanes of the dest.  */
      mask = (elt / (<ssescalarnum> / 2)) * 0x11;
      emit_insn (gen_avx_vperm2f128<mode>3 (op0, op0, op0, GEN_INT (mask)));
      DONE;
    }

  operands[1] = adjust_address_nv (op1, <ssescalarmode>mode,
				   elt * GET_MODE_SIZE (<ssescalarmode>mode));
})

(define_expand "avx_vpermil<mode>"
  [(set (match_operand:VF2 0 "register_operand")
	(vec_select:VF2
	  (match_operand:VF2 1 "nonimmediate_operand")
	  (match_operand:SI 2 "const_0_to_255_operand")))]
  "TARGET_AVX"
{
  int mask = INTVAL (operands[2]);
  rtx perm[<ssescalarnum>];

  perm[0] = GEN_INT (mask & 1);
  perm[1] = GEN_INT ((mask >> 1) & 1);
  if (<MODE>mode == V4DFmode)
    {
      perm[2] = GEN_INT (((mask >> 2) & 1) + 2);
      perm[3] = GEN_INT (((mask >> 3) & 1) + 2);
    }

  operands[2]
    = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (<ssescalarnum>, perm));
})

(define_expand "avx_vpermil<mode>"
  [(set (match_operand:VF1 0 "register_operand")
	(vec_select:VF1
	  (match_operand:VF1 1 "nonimmediate_operand")
	  (match_operand:SI 2 "const_0_to_255_operand")))]
  "TARGET_AVX"
{
  int mask = INTVAL (operands[2]);
  rtx perm[<ssescalarnum>];

  perm[0] = GEN_INT (mask & 3);
  perm[1] = GEN_INT ((mask >> 2) & 3);
  perm[2] = GEN_INT ((mask >> 4) & 3);
  perm[3] = GEN_INT ((mask >> 6) & 3);
  if (<MODE>mode == V8SFmode)
    {
      perm[4] = GEN_INT ((mask & 3) + 4);
      perm[5] = GEN_INT (((mask >> 2) & 3) + 4);
      perm[6] = GEN_INT (((mask >> 4) & 3) + 4);
      perm[7] = GEN_INT (((mask >> 6) & 3) + 4);
    }

  operands[2]
    = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (<ssescalarnum>, perm));
})

(define_insn "*avx_vpermilp<mode>"
  [(set (match_operand:VF 0 "register_operand" "=x")
	(vec_select:VF
	  (match_operand:VF 1 "nonimmediate_operand" "xm")
	  (match_parallel 2 ""
	    [(match_operand 3 "const_int_operand")])))]
  "TARGET_AVX
   && avx_vpermilp_parallel (operands[2], <MODE>mode)"
{
  int mask = avx_vpermilp_parallel (operands[2], <MODE>mode) - 1;
  operands[2] = GEN_INT (mask);
  return "vpermil<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}";
}
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<MODE>")])

(define_insn "avx_vpermilvar<mode>3"
  [(set (match_operand:VF 0 "register_operand" "=x")
	(unspec:VF
	  [(match_operand:VF 1 "register_operand" "x")
	   (match_operand:<sseintvecmode> 2 "nonimmediate_operand" "xm")]
	  UNSPEC_VPERMIL))]
  "TARGET_AVX"
  "vpermil<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "<MODE>")])

(define_expand "avx_vperm2f128<mode>3"
  [(set (match_operand:AVX256MODE2P 0 "register_operand")
	(unspec:AVX256MODE2P
	  [(match_operand:AVX256MODE2P 1 "register_operand")
	   (match_operand:AVX256MODE2P 2 "nonimmediate_operand")
	   (match_operand:SI 3 "const_0_to_255_operand")]
	  UNSPEC_VPERMIL2F128))]
  "TARGET_AVX"
{
  int mask = INTVAL (operands[3]);
  if ((mask & 0x88) == 0)
    {
      rtx perm[<ssescalarnum>], t1, t2;
      int i, base, nelt = <ssescalarnum>, nelt2 = nelt / 2;

      base = (mask & 3) * nelt2;
      for (i = 0; i < nelt2; ++i)
	perm[i] = GEN_INT (base + i);

      base = ((mask >> 4) & 3) * nelt2;
      for (i = 0; i < nelt2; ++i)
	perm[i + nelt2] = GEN_INT (base + i);

      t2 = gen_rtx_VEC_CONCAT (<ssedoublevecmode>mode,
			       operands[1], operands[2]);
      t1 = gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (nelt, perm));
      t2 = gen_rtx_VEC_SELECT (<MODE>mode, t2, t1);
      t2 = gen_rtx_SET (VOIDmode, operands[0], t2);
      emit_insn (t2);
      DONE;
    }
})

;; Note that bits 7 and 3 of the imm8 allow lanes to be zeroed, which
;; means that in order to represent this properly in rtl we'd have to
;; nest *another* vec_concat with a zero operand and do the select from
;; a 4x wide vector.  That doesn't seem very nice.
(define_insn "*avx_vperm2f128<mode>_full"
  [(set (match_operand:AVX256MODE2P 0 "register_operand" "=x")
	(unspec:AVX256MODE2P
	  [(match_operand:AVX256MODE2P 1 "register_operand" "x")
	   (match_operand:AVX256MODE2P 2 "nonimmediate_operand" "xm")
	   (match_operand:SI 3 "const_0_to_255_operand" "n")]
	  UNSPEC_VPERMIL2F128))]
  "TARGET_AVX"
  "vperm2<i128>\t{%3, %2, %1, %0|%0, %1, %2, %3}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "*avx_vperm2f128<mode>_nozero"
  [(set (match_operand:AVX256MODE2P 0 "register_operand" "=x")
	(vec_select:AVX256MODE2P
	  (vec_concat:<ssedoublevecmode>
	    (match_operand:AVX256MODE2P 1 "register_operand" "x")
	    (match_operand:AVX256MODE2P 2 "nonimmediate_operand" "xm"))
	  (match_parallel 3 ""
	    [(match_operand 4 "const_int_operand")])))]
  "TARGET_AVX
   && avx_vperm2f128_parallel (operands[3], <MODE>mode)"
{
  int mask = avx_vperm2f128_parallel (operands[3], <MODE>mode) - 1;
  if (mask == 0x12)
    return "vinsert<i128>\t{$0, %x2, %1, %0|%0, %1, %x2, 0}";
  if (mask == 0x20)
    return "vinsert<i128>\t{$1, %x2, %1, %0|%0, %1, %x2, 1}";
  operands[3] = GEN_INT (mask);
  return "vperm2<i128>\t{%3, %2, %1, %0|%0, %1, %2, %3}";
}
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_expand "avx_vinsertf128<mode>"
  [(match_operand:V_256 0 "register_operand")
   (match_operand:V_256 1 "register_operand")
   (match_operand:<ssehalfvecmode> 2 "nonimmediate_operand")
   (match_operand:SI 3 "const_0_to_1_operand")]
  "TARGET_AVX"
{
  rtx (*insn)(rtx, rtx, rtx);

  switch (INTVAL (operands[3]))
    {
    case 0:
      insn = gen_vec_set_lo_<mode>;
      break;
    case 1:
      insn = gen_vec_set_hi_<mode>;
      break;
    default:
      gcc_unreachable ();
    }

  emit_insn (insn (operands[0], operands[1], operands[2]));
  DONE;
})

(define_insn "avx2_vec_set_lo_v4di"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(vec_concat:V4DI
	  (match_operand:V2DI 2 "nonimmediate_operand" "xm")
	  (vec_select:V2DI
	    (match_operand:V4DI 1 "register_operand" "x")
	    (parallel [(const_int 2) (const_int 3)]))))]
  "TARGET_AVX2"
  "vinserti128\t{$0x0, %2, %1, %0|%0, %1, %2, 0x0}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "avx2_vec_set_hi_v4di"
  [(set (match_operand:V4DI 0 "register_operand" "=x")
	(vec_concat:V4DI
	  (vec_select:V2DI
	    (match_operand:V4DI 1 "register_operand" "x")
	    (parallel [(const_int 0) (const_int 1)]))
	  (match_operand:V2DI 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX2"
  "vinserti128\t{$0x1, %2, %1, %0|%0, %1, %2, 0x1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_set_lo_<mode>"
  [(set (match_operand:VI8F_256 0 "register_operand" "=x")
	(vec_concat:VI8F_256
	  (match_operand:<ssehalfvecmode> 2 "nonimmediate_operand" "xm")
	  (vec_select:<ssehalfvecmode>
	    (match_operand:VI8F_256 1 "register_operand" "x")
	    (parallel [(const_int 2) (const_int 3)]))))]
  "TARGET_AVX"
  "vinsert<i128>\t{$0x0, %2, %1, %0|%0, %1, %2, 0x0}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "vec_set_hi_<mode>"
  [(set (match_operand:VI8F_256 0 "register_operand" "=x")
	(vec_concat:VI8F_256
	  (vec_select:<ssehalfvecmode>
	    (match_operand:VI8F_256 1 "register_operand" "x")
	    (parallel [(const_int 0) (const_int 1)]))
	  (match_operand:<ssehalfvecmode> 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX"
  "vinsert<i128>\t{$0x1, %2, %1, %0|%0, %1, %2, 0x1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "vec_set_lo_<mode>"
  [(set (match_operand:VI4F_256 0 "register_operand" "=x")
	(vec_concat:VI4F_256
	  (match_operand:<ssehalfvecmode> 2 "nonimmediate_operand" "xm")
	  (vec_select:<ssehalfvecmode>
	    (match_operand:VI4F_256 1 "register_operand" "x")
	    (parallel [(const_int 4) (const_int 5)
		       (const_int 6) (const_int 7)]))))]
  "TARGET_AVX"
  "vinsert<i128>\t{$0x0, %2, %1, %0|%0, %1, %2, 0x0}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "vec_set_hi_<mode>"
  [(set (match_operand:VI4F_256 0 "register_operand" "=x")
	(vec_concat:VI4F_256
	  (vec_select:<ssehalfvecmode>
	    (match_operand:VI4F_256 1 "register_operand" "x")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)]))
	  (match_operand:<ssehalfvecmode> 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX"
  "vinsert<i128>\t{$0x1, %2, %1, %0|%0, %1, %2, 0x1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "vec_set_lo_v16hi"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(vec_concat:V16HI
	  (match_operand:V8HI 2 "nonimmediate_operand" "xm")
	  (vec_select:V8HI
	    (match_operand:V16HI 1 "register_operand" "x")
	    (parallel [(const_int 8) (const_int 9)
		       (const_int 10) (const_int 11)
		       (const_int 12) (const_int 13)
		       (const_int 14) (const_int 15)]))))]
  "TARGET_AVX"
  "vinsert%~128\t{$0x0, %2, %1, %0|%0, %1, %2, 0x0}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_set_hi_v16hi"
  [(set (match_operand:V16HI 0 "register_operand" "=x")
	(vec_concat:V16HI
	  (vec_select:V8HI
	    (match_operand:V16HI 1 "register_operand" "x")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)
		       (const_int 4) (const_int 5)
		       (const_int 6) (const_int 7)]))
	  (match_operand:V8HI 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX"
  "vinsert%~128\t{$0x1, %2, %1, %0|%0, %1, %2, 0x1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_set_lo_v32qi"
  [(set (match_operand:V32QI 0 "register_operand" "=x")
	(vec_concat:V32QI
	  (match_operand:V16QI 2 "nonimmediate_operand" "xm")
	  (vec_select:V16QI
	    (match_operand:V32QI 1 "register_operand" "x")
	    (parallel [(const_int 16) (const_int 17)
		       (const_int 18) (const_int 19)
		       (const_int 20) (const_int 21)
		       (const_int 22) (const_int 23)
		       (const_int 24) (const_int 25)
		       (const_int 26) (const_int 27)
		       (const_int 28) (const_int 29)
		       (const_int 30) (const_int 31)]))))]
  "TARGET_AVX"
  "vinsert%~128\t{$0x0, %2, %1, %0|%0, %1, %2, 0x0}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "vec_set_hi_v32qi"
  [(set (match_operand:V32QI 0 "register_operand" "=x")
	(vec_concat:V32QI
	  (vec_select:V16QI
	    (match_operand:V32QI 1 "register_operand" "x")
	    (parallel [(const_int 0) (const_int 1)
		       (const_int 2) (const_int 3)
		       (const_int 4) (const_int 5)
		       (const_int 6) (const_int 7)
		       (const_int 8) (const_int 9)
		       (const_int 10) (const_int 11)
		       (const_int 12) (const_int 13)
		       (const_int 14) (const_int 15)]))
	  (match_operand:V16QI 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX"
  "vinsert%~128\t{$0x1, %2, %1, %0|%0, %1, %2, 0x1}"
  [(set_attr "type" "sselog")
   (set_attr "prefix_extra" "1")
   (set_attr "length_immediate" "1")
   (set_attr "prefix" "vex")
   (set_attr "mode" "OI")])

(define_insn "<avx_avx2>_maskload<ssemodesuffix><avxsizesuffix>"
  [(set (match_operand:V48_AVX2 0 "register_operand" "=x")
	(unspec:V48_AVX2
	  [(match_operand:<sseintvecmode> 2 "register_operand" "x")
	   (match_operand:V48_AVX2 1 "memory_operand" "m")]
	  UNSPEC_MASKMOV))]
  "TARGET_AVX"
  "v<sseintprefix>maskmov<ssemodesuffix>\t{%1, %2, %0|%0, %2, %1}"
  [(set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "<avx_avx2>_maskstore<ssemodesuffix><avxsizesuffix>"
  [(set (match_operand:V48_AVX2 0 "memory_operand" "+m")
	(unspec:V48_AVX2
	  [(match_operand:<sseintvecmode> 1 "register_operand" "x")
	   (match_operand:V48_AVX2 2 "register_operand" "x")
	   (match_dup 0)]
	  UNSPEC_MASKMOV))]
  "TARGET_AVX"
  "v<sseintprefix>maskmov<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sselog1")
   (set_attr "prefix_extra" "1")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector") 
   (set_attr "mode" "<sseinsnmode>")])

(define_insn_and_split "avx_<castmode><avxsizesuffix>_<castmode>"
  [(set (match_operand:AVX256MODE2P 0 "nonimmediate_operand" "=x,m")
	(unspec:AVX256MODE2P
	  [(match_operand:<ssehalfvecmode> 1 "nonimmediate_operand" "xm,x")]
	  UNSPEC_CAST))]
  "TARGET_AVX"
  "#"
  "&& reload_completed"
  [(const_int 0)]
{
  rtx op0 = operands[0];
  rtx op1 = operands[1];
  if (REG_P (op0))
    op0 = gen_rtx_REG (<ssehalfvecmode>mode, REGNO (op0));
  else
    op1 = gen_rtx_REG (<MODE>mode, REGNO (op1));
  emit_move_insn (op0, op1);
  DONE;
})

(define_expand "vec_init<mode>"
  [(match_operand:V_256 0 "register_operand")
   (match_operand 1)]
  "TARGET_AVX"
{
  ix86_expand_vector_init (false, operands[0], operands[1]);
  DONE;
})

(define_expand "avx2_extracti128"
  [(match_operand:V2DI 0 "nonimmediate_operand")
   (match_operand:V4DI 1 "register_operand")
   (match_operand:SI 2 "const_0_to_1_operand")]
  "TARGET_AVX2"
{
  rtx (*insn)(rtx, rtx);

  switch (INTVAL (operands[2]))
    {
    case 0:
      insn = gen_vec_extract_lo_v4di;
      break;
    case 1:
      insn = gen_vec_extract_hi_v4di;
      break;
    default:
      gcc_unreachable ();
    }

  emit_insn (insn (operands[0], operands[1]));
  DONE;
})

(define_expand "avx2_inserti128"
  [(match_operand:V4DI 0 "register_operand")
   (match_operand:V4DI 1 "register_operand")
   (match_operand:V2DI 2 "nonimmediate_operand")
   (match_operand:SI 3 "const_0_to_1_operand")]
  "TARGET_AVX2"
{
  rtx (*insn)(rtx, rtx, rtx);

  switch (INTVAL (operands[3]))
    {
    case 0:
      insn = gen_avx2_vec_set_lo_v4di;
      break;
    case 1:
      insn = gen_avx2_vec_set_hi_v4di;
      break;
    default:
      gcc_unreachable ();
    }

  emit_insn (insn (operands[0], operands[1], operands[2]));
  DONE;
})

(define_insn "avx2_ashrv<mode>"
  [(set (match_operand:VI4_AVX2 0 "register_operand" "=x")
	(ashiftrt:VI4_AVX2
	  (match_operand:VI4_AVX2 1 "register_operand" "x")
	  (match_operand:VI4_AVX2 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX2"
  "vpsravd\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseishft")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "avx2_<shift_insn>v<mode>"
  [(set (match_operand:VI48_AVX2 0 "register_operand" "=x")
	(any_lshift:VI48_AVX2
	  (match_operand:VI48_AVX2 1 "register_operand" "x")
	  (match_operand:VI48_AVX2 2 "nonimmediate_operand" "xm")))]
  "TARGET_AVX2"
  "vp<vshift>v<ssemodesuffix>\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "sseishft")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "avx_vec_concat<mode>"
  [(set (match_operand:V_256 0 "register_operand" "=x,x")
	(vec_concat:V_256
	  (match_operand:<ssehalfvecmode> 1 "register_operand" "x,x")
	  (match_operand:<ssehalfvecmode> 2 "vector_move_operand" "xm,C")))]
  "TARGET_AVX"
{
  switch (which_alternative)
    {
    case 0:
      return "vinsert<i128>\t{$0x1, %2, %t1, %0|%0, %t1, %2, 0x1}";
    case 1:
      switch (get_attr_mode (insn))
	{
	case MODE_V8SF:
	  return "vmovaps\t{%1, %x0|%x0, %1}";
	case MODE_V4DF:
	  return "vmovapd\t{%1, %x0|%x0, %1}";
	default:
	  return "vmovdqa\t{%1, %x0|%x0, %1}";
	}
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "type" "sselog,ssemov")
   (set_attr "prefix_extra" "1,*")
   (set_attr "length_immediate" "1,*")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "vcvtph2ps"
  [(set (match_operand:V4SF 0 "register_operand" "=x")
	(vec_select:V4SF
	  (unspec:V8SF [(match_operand:V8HI 1 "register_operand" "x")]
		       UNSPEC_VCVTPH2PS)
	  (parallel [(const_int 0) (const_int 1)
		     (const_int 2) (const_int 3)])))]
  "TARGET_F16C"
  "vcvtph2ps\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4SF")])

(define_insn "*vcvtph2ps_load"
  [(set (match_operand:V4SF 0 "register_operand" "=x")
	(unspec:V4SF [(match_operand:V4HI 1 "memory_operand" "m")]
		     UNSPEC_VCVTPH2PS))]
  "TARGET_F16C"
  "vcvtph2ps\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V8SF")])

(define_insn "vcvtph2ps256"
  [(set (match_operand:V8SF 0 "register_operand" "=x")
	(unspec:V8SF [(match_operand:V8HI 1 "nonimmediate_operand" "xm")]
		     UNSPEC_VCVTPH2PS))]
  "TARGET_F16C"
  "vcvtph2ps\t{%1, %0|%0, %1}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "double")
   (set_attr "mode" "V8SF")])

(define_expand "vcvtps2ph"
  [(set (match_operand:V8HI 0 "register_operand")
	(vec_concat:V8HI
	  (unspec:V4HI [(match_operand:V4SF 1 "register_operand")
			(match_operand:SI 2 "const_0_to_255_operand")]
		       UNSPEC_VCVTPS2PH)
	  (match_dup 3)))]
  "TARGET_F16C"
  "operands[3] = CONST0_RTX (V4HImode);")

(define_insn "*vcvtps2ph"
  [(set (match_operand:V8HI 0 "register_operand" "=x")
	(vec_concat:V8HI
	  (unspec:V4HI [(match_operand:V4SF 1 "register_operand" "x")
			(match_operand:SI 2 "const_0_to_255_operand" "N")]
		       UNSPEC_VCVTPS2PH)
	  (match_operand:V4HI 3 "const0_operand")))]
  "TARGET_F16C"
  "vcvtps2ph\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4SF")])

(define_insn "*vcvtps2ph_store"
  [(set (match_operand:V4HI 0 "memory_operand" "=m")
	(unspec:V4HI [(match_operand:V4SF 1 "register_operand" "x")
		      (match_operand:SI 2 "const_0_to_255_operand" "N")]
		     UNSPEC_VCVTPS2PH))]
  "TARGET_F16C"
  "vcvtps2ph\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "mode" "V4SF")])

(define_insn "vcvtps2ph256"
  [(set (match_operand:V8HI 0 "nonimmediate_operand" "=xm")
	(unspec:V8HI [(match_operand:V8SF 1 "register_operand" "x")
		      (match_operand:SI 2 "const_0_to_255_operand" "N")]
		     UNSPEC_VCVTPS2PH))]
  "TARGET_F16C"
  "vcvtps2ph\t{%2, %1, %0|%0, %1, %2}"
  [(set_attr "type" "ssecvt")
   (set_attr "prefix" "vex")
   (set_attr "btver2_decode" "vector")
   (set_attr "mode" "V8SF")])

;; For gather* insn patterns
(define_mode_iterator VEC_GATHER_MODE
		      [V2DI V2DF V4DI V4DF V4SI V4SF V8SI V8SF])
(define_mode_attr VEC_GATHER_IDXSI
		      [(V2DI "V4SI") (V2DF "V4SI")
		       (V4DI "V4SI") (V4DF "V4SI")
		       (V4SI "V4SI") (V4SF "V4SI")
		       (V8SI "V8SI") (V8SF "V8SI")])
(define_mode_attr VEC_GATHER_IDXDI
		      [(V2DI "V2DI") (V2DF "V2DI")
		       (V4DI "V4DI") (V4DF "V4DI")
		       (V4SI "V2DI") (V4SF "V2DI")
		       (V8SI "V4DI") (V8SF "V4DI")])
(define_mode_attr VEC_GATHER_SRCDI
		      [(V2DI "V2DI") (V2DF "V2DF")
		       (V4DI "V4DI") (V4DF "V4DF")
		       (V4SI "V4SI") (V4SF "V4SF")
		       (V8SI "V4SI") (V8SF "V4SF")])

(define_expand "avx2_gathersi<mode>"
  [(parallel [(set (match_operand:VEC_GATHER_MODE 0 "register_operand")
		   (unspec:VEC_GATHER_MODE
		     [(match_operand:VEC_GATHER_MODE 1 "register_operand")
		      (mem:<ssescalarmode>
			(match_par_dup 7
			  [(match_operand 2 "vsib_address_operand")
			   (match_operand:<VEC_GATHER_IDXSI>
			      3 "register_operand")
			   (match_operand:SI 5 "const1248_operand ")]))
		      (mem:BLK (scratch))
		      (match_operand:VEC_GATHER_MODE 4 "register_operand")]
		     UNSPEC_GATHER))
	      (clobber (match_scratch:VEC_GATHER_MODE 6))])]
  "TARGET_AVX2"
{
  operands[7]
    = gen_rtx_UNSPEC (Pmode, gen_rtvec (3, operands[2], operands[3],
					operands[5]), UNSPEC_VSIBADDR);
})

(define_insn "*avx2_gathersi<mode>"
  [(set (match_operand:VEC_GATHER_MODE 0 "register_operand" "=&x")
	(unspec:VEC_GATHER_MODE
	  [(match_operand:VEC_GATHER_MODE 2 "register_operand" "0")
	   (match_operator:<ssescalarmode> 7 "vsib_mem_operator"
	     [(unspec:P
		[(match_operand:P 3 "vsib_address_operand" "p")
		 (match_operand:<VEC_GATHER_IDXSI> 4 "register_operand" "x")
		 (match_operand:SI 6 "const1248_operand" "n")]
		UNSPEC_VSIBADDR)])
	   (mem:BLK (scratch))
	   (match_operand:VEC_GATHER_MODE 5 "register_operand" "1")]
	  UNSPEC_GATHER))
   (clobber (match_scratch:VEC_GATHER_MODE 1 "=&x"))]
  "TARGET_AVX2"
  "v<sseintprefix>gatherd<ssemodesuffix>\t{%1, %7, %0|%0, %7, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "*avx2_gathersi<mode>_2"
  [(set (match_operand:VEC_GATHER_MODE 0 "register_operand" "=&x")
	(unspec:VEC_GATHER_MODE
	  [(pc)
	   (match_operator:<ssescalarmode> 6 "vsib_mem_operator"
	     [(unspec:P
		[(match_operand:P 2 "vsib_address_operand" "p")
		 (match_operand:<VEC_GATHER_IDXSI> 3 "register_operand" "x")
		 (match_operand:SI 5 "const1248_operand" "n")]
		UNSPEC_VSIBADDR)])
	   (mem:BLK (scratch))
	   (match_operand:VEC_GATHER_MODE 4 "register_operand" "1")]
	  UNSPEC_GATHER))
   (clobber (match_scratch:VEC_GATHER_MODE 1 "=&x"))]
  "TARGET_AVX2"
  "v<sseintprefix>gatherd<ssemodesuffix>\t{%1, %6, %0|%0, %6, %1}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_expand "avx2_gatherdi<mode>"
  [(parallel [(set (match_operand:VEC_GATHER_MODE 0 "register_operand")
		   (unspec:VEC_GATHER_MODE
		     [(match_operand:<VEC_GATHER_SRCDI> 1 "register_operand")
		      (mem:<ssescalarmode>
			(match_par_dup 7
			  [(match_operand 2 "vsib_address_operand")
			   (match_operand:<VEC_GATHER_IDXDI>
			      3 "register_operand")
			   (match_operand:SI 5 "const1248_operand ")]))
		      (mem:BLK (scratch))
		      (match_operand:<VEC_GATHER_SRCDI>
			4 "register_operand")]
		     UNSPEC_GATHER))
	      (clobber (match_scratch:VEC_GATHER_MODE 6))])]
  "TARGET_AVX2"
{
  operands[7]
    = gen_rtx_UNSPEC (Pmode, gen_rtvec (3, operands[2], operands[3],
					operands[5]), UNSPEC_VSIBADDR);
})

(define_insn "*avx2_gatherdi<mode>"
  [(set (match_operand:VEC_GATHER_MODE 0 "register_operand" "=&x")
	(unspec:VEC_GATHER_MODE
	  [(match_operand:<VEC_GATHER_SRCDI> 2 "register_operand" "0")
	   (match_operator:<ssescalarmode> 7 "vsib_mem_operator"
	     [(unspec:P
		[(match_operand:P 3 "vsib_address_operand" "p")
		 (match_operand:<VEC_GATHER_IDXDI> 4 "register_operand" "x")
		 (match_operand:SI 6 "const1248_operand" "n")]
		UNSPEC_VSIBADDR)])
	   (mem:BLK (scratch))
	   (match_operand:<VEC_GATHER_SRCDI> 5 "register_operand" "1")]
	  UNSPEC_GATHER))
   (clobber (match_scratch:VEC_GATHER_MODE 1 "=&x"))]
  "TARGET_AVX2"
  "v<sseintprefix>gatherq<ssemodesuffix>\t{%5, %7, %2|%2, %7, %5}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "*avx2_gatherdi<mode>_2"
  [(set (match_operand:VEC_GATHER_MODE 0 "register_operand" "=&x")
	(unspec:VEC_GATHER_MODE
	  [(pc)
	   (match_operator:<ssescalarmode> 6 "vsib_mem_operator"
	     [(unspec:P
		[(match_operand:P 2 "vsib_address_operand" "p")
		 (match_operand:<VEC_GATHER_IDXDI> 3 "register_operand" "x")
		 (match_operand:SI 5 "const1248_operand" "n")]
		UNSPEC_VSIBADDR)])
	   (mem:BLK (scratch))
	   (match_operand:<VEC_GATHER_SRCDI> 4 "register_operand" "1")]
	  UNSPEC_GATHER))
   (clobber (match_scratch:VEC_GATHER_MODE 1 "=&x"))]
  "TARGET_AVX2"
{
  if (<MODE>mode != <VEC_GATHER_SRCDI>mode)
    return "v<sseintprefix>gatherq<ssemodesuffix>\t{%4, %6, %x0|%x0, %6, %4}";
  return "v<sseintprefix>gatherq<ssemodesuffix>\t{%4, %6, %0|%0, %6, %4}";
}
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "*avx2_gatherdi<mode>_3"
  [(set (match_operand:<VEC_GATHER_SRCDI> 0 "register_operand" "=&x")
	(vec_select:<VEC_GATHER_SRCDI>
	  (unspec:VI4F_256
	    [(match_operand:<VEC_GATHER_SRCDI> 2 "register_operand" "0")
	     (match_operator:<ssescalarmode> 7 "vsib_mem_operator"
	       [(unspec:P
		  [(match_operand:P 3 "vsib_address_operand" "p")
		   (match_operand:<VEC_GATHER_IDXDI> 4 "register_operand" "x")
		   (match_operand:SI 6 "const1248_operand" "n")]
		  UNSPEC_VSIBADDR)])
	     (mem:BLK (scratch))
	     (match_operand:<VEC_GATHER_SRCDI> 5 "register_operand" "1")]
	     UNSPEC_GATHER)
	  (parallel [(const_int 0) (const_int 1)
		     (const_int 2) (const_int 3)])))
   (clobber (match_scratch:VI4F_256 1 "=&x"))]
  "TARGET_AVX2"
  "v<sseintprefix>gatherq<ssemodesuffix>\t{%5, %7, %0|%0, %7, %5}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])

(define_insn "*avx2_gatherdi<mode>_4"
  [(set (match_operand:<VEC_GATHER_SRCDI> 0 "register_operand" "=&x")
	(vec_select:<VEC_GATHER_SRCDI>
	  (unspec:VI4F_256
	    [(pc)
	     (match_operator:<ssescalarmode> 6 "vsib_mem_operator"
	       [(unspec:P
		  [(match_operand:P 2 "vsib_address_operand" "p")
		   (match_operand:<VEC_GATHER_IDXDI> 3 "register_operand" "x")
		   (match_operand:SI 5 "const1248_operand" "n")]
		  UNSPEC_VSIBADDR)])
	     (mem:BLK (scratch))
	     (match_operand:<VEC_GATHER_SRCDI> 4 "register_operand" "1")]
	    UNSPEC_GATHER)
	  (parallel [(const_int 0) (const_int 1)
		     (const_int 2) (const_int 3)])))
   (clobber (match_scratch:VI4F_256 1 "=&x"))]
  "TARGET_AVX2"
  "v<sseintprefix>gatherq<ssemodesuffix>\t{%4, %6, %0|%0, %6, %4}"
  [(set_attr "type" "ssemov")
   (set_attr "prefix" "vex")
   (set_attr "mode" "<sseinsnmode>")])
