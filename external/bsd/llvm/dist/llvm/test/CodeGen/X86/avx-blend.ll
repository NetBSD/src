; RUN: llc < %s -mtriple=x86_64-apple-darwin -mcpu=corei7-avx  -mattr=+avx | FileCheck %s

; AVX128 tests:

;CHECK-LABEL: vsel_float:
;CHECK: vblendvps
;CHECK: ret
define <4 x float> @vsel_float(<4 x float> %v1, <4 x float> %v2) {
  %vsel = select <4 x i1> <i1 true, i1 false, i1 true, i1 false>, <4 x float> %v1, <4 x float> %v2
  ret <4 x float> %vsel
}


;CHECK-LABEL: vsel_i32:
;CHECK: vblendvps
;CHECK: ret
define <4 x i32> @vsel_i32(<4 x i32> %v1, <4 x i32> %v2) {
  %vsel = select <4 x i1> <i1 true, i1 false, i1 true, i1 false>, <4 x i32> %v1, <4 x i32> %v2
  ret <4 x i32> %vsel
}


;CHECK-LABEL: vsel_double:
;CHECK: vmovsd
;CHECK: ret
define <2 x double> @vsel_double(<2 x double> %v1, <2 x double> %v2) {
  %vsel = select <2 x i1> <i1 true, i1 false>, <2 x double> %v1, <2 x double> %v2
  ret <2 x double> %vsel
}


;CHECK-LABEL: vsel_i64:
;CHECK: vmovsd
;CHECK: ret
define <2 x i64> @vsel_i64(<2 x i64> %v1, <2 x i64> %v2) {
  %vsel = select <2 x i1> <i1 true, i1 false>, <2 x i64> %v1, <2 x i64> %v2
  ret <2 x i64> %vsel
}


;CHECK-LABEL: vsel_i8:
;CHECK: vpblendvb
;CHECK: ret
define <16 x i8> @vsel_i8(<16 x i8> %v1, <16 x i8> %v2) {
  %vsel = select <16 x i1> <i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false>, <16 x i8> %v1, <16 x i8> %v2
  ret <16 x i8> %vsel
}


; AVX256 tests:


;CHECK-LABEL: vsel_float8:
;CHECK-NOT: vinsertf128
;CHECK: vblendvps
;CHECK: ret
define <8 x float> @vsel_float8(<8 x float> %v1, <8 x float> %v2) {
  %vsel = select <8 x i1> <i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false>, <8 x float> %v1, <8 x float> %v2
  ret <8 x float> %vsel
}

;CHECK-LABEL: vsel_i328:
;CHECK-NOT: vinsertf128
;CHECK: vblendvps
;CHECK-NEXT: ret
define <8 x i32> @vsel_i328(<8 x i32> %v1, <8 x i32> %v2) {
  %vsel = select <8 x i1> <i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false>, <8 x i32> %v1, <8 x i32> %v2
  ret <8 x i32> %vsel
}

;CHECK-LABEL: vsel_double8:
;CHECK: vblendvpd
;CHECK: ret
define <8 x double> @vsel_double8(<8 x double> %v1, <8 x double> %v2) {
  %vsel = select <8 x i1> <i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false>, <8 x double> %v1, <8 x double> %v2
  ret <8 x double> %vsel
}

;CHECK-LABEL: vsel_i648:
;CHECK: vblendvpd
;CHECK: ret
define <8 x i64> @vsel_i648(<8 x i64> %v1, <8 x i64> %v2) {
  %vsel = select <8 x i1> <i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false>, <8 x i64> %v1, <8 x i64> %v2
  ret <8 x i64> %vsel
}

;CHECK-LABEL: vsel_double4:
;CHECK-NOT: vinsertf128
;CHECK: vblendvpd
;CHECK-NEXT: ret
define <4 x double> @vsel_double4(<4 x double> %v1, <4 x double> %v2) {
  %vsel = select <4 x i1> <i1 true, i1 false, i1 true, i1 false>, <4 x double> %v1, <4 x double> %v2
  ret <4 x double> %vsel
}

;; TEST blend + compares
; CHECK: testa
define <2 x double> @testa(<2 x double> %x, <2 x double> %y) {
  ; CHECK: vcmplepd
  ; CHECK: vblendvpd
  %max_is_x = fcmp oge <2 x double> %x, %y
  %max = select <2 x i1> %max_is_x, <2 x double> %x, <2 x double> %y
  ret <2 x double> %max
}

; CHECK: testb
define <2 x double> @testb(<2 x double> %x, <2 x double> %y) {
  ; CHECK: vcmpnlepd
  ; CHECK: vblendvpd
  %min_is_x = fcmp ult <2 x double> %x, %y
  %min = select <2 x i1> %min_is_x, <2 x double> %x, <2 x double> %y
  ret <2 x double> %min
}


