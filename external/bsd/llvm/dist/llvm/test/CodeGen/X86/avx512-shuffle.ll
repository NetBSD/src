; RUN: llc < %s -mtriple=x86_64-apple-darwin -mcpu=knl --show-mc-encoding| FileCheck %s
; CHECK: LCP
; CHECK: .long 2
; CHECK: .long 5
; CHECK: .long 0
; CHECK: .long 0
; CHECK: .long 7
; CHECK: .long 0
; CHECK: .long 10
; CHECK: .long 1
; CHECK: .long 0
; CHECK: .long 5
; CHECK: .long 0
; CHECK: .long 4
; CHECK: .long 7
; CHECK: .long 0
; CHECK: .long 10
; CHECK: .long 1
; CHECK-LABEL: test1:
; CHECK: vpermps
; CHECK: ret
define <16 x float> @test1(<16 x float> %a) nounwind {
  %c = shufflevector <16 x float> %a, <16 x float> undef, <16 x i32> <i32 2, i32 5, i32 undef, i32 undef, i32 7, i32 undef, i32 10, i32 1,  i32 0, i32 5, i32 undef, i32 4, i32 7, i32 undef, i32 10, i32 1>
  ret <16 x float> %c
}

; CHECK-LABEL: test2:
; CHECK: vpermd
; CHECK: ret
define <16 x i32> @test2(<16 x i32> %a) nounwind {
  %c = shufflevector <16 x i32> %a, <16 x i32> undef, <16 x i32> <i32 2, i32 5, i32 undef, i32 undef, i32 7, i32 undef, i32 10, i32 1,  i32 0, i32 5, i32 undef, i32 4, i32 7, i32 undef, i32 10, i32 1>
  ret <16 x i32> %c
}

; CHECK-LABEL: test3:
; CHECK: vpermq
; CHECK: ret
define <8 x i64> @test3(<8 x i64> %a) nounwind {
  %c = shufflevector <8 x i64> %a, <8 x i64> undef, <8 x i32> <i32 2, i32 5, i32 1, i32 undef, i32 7, i32 undef, i32 3, i32 1>
  ret <8 x i64> %c
}

; CHECK-LABEL: test4:
; CHECK: vpermpd
; CHECK: ret
define <8 x double> @test4(<8 x double> %a) nounwind {
  %c = shufflevector <8 x double> %a, <8 x double> undef, <8 x i32> <i32 1, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  ret <8 x double> %c
}

; CHECK-LABEL: test5:
; CHECK: vpermt2pd
; CHECK: ret
define <8 x double> @test5(<8 x double> %a, <8 x double> %b) nounwind {
  %c = shufflevector <8 x double> %a, <8 x double> %b, <8 x i32> <i32 2, i32 8, i32 0, i32 1, i32 6, i32 10, i32 4, i32 5>
  ret <8 x double> %c
}

; The reg variant of vpermt2 with a writemask
; CHECK-LABEL: test5m:
; CHECK: vpermt2pd {{.* {%k[1-7]} {z}}}
define <8 x double> @test5m(<8 x double> %a, <8 x double> %b, i8 %mask) nounwind {
  %c = shufflevector <8 x double> %a, <8 x double> %b, <8 x i32> <i32 2, i32 8, i32 0, i32 1, i32 6, i32 10, i32 4, i32 5>
  %m = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %m, <8 x double> %c, <8 x double> zeroinitializer
  ret <8 x double> %res
}

; CHECK-LABEL: test6:
; CHECK: vpermq $30
; CHECK: ret
define <8 x i64> @test6(<8 x i64> %a) nounwind {
  %c = shufflevector <8 x i64> %a, <8 x i64> undef, <8 x i32> <i32 2, i32 3, i32 1, i32 0, i32 6, i32 7, i32 5, i32 4>
  ret <8 x i64> %c
}

; CHECK-LABEL: test7:
; CHECK: vpermt2q
; CHECK: ret
define <8 x i64> @test7(<8 x i64> %a, <8 x i64> %b) nounwind {
  %c = shufflevector <8 x i64> %a, <8 x i64> %b, <8 x i32> <i32 2, i32 8, i32 0, i32 1, i32 6, i32 10, i32 4, i32 5>
  ret <8 x i64> %c
}

; The reg variant of vpermt2 with a writemask
; CHECK-LABEL: test7m:
; CHECK: vpermt2q {{.* {%k[1-7]} {z}}}
define <8 x i64> @test7m(<8 x i64> %a, <8 x i64> %b, i8 %mask) nounwind {
  %c = shufflevector <8 x i64> %a, <8 x i64> %b, <8 x i32> <i32 2, i32 8, i32 0, i32 1, i32 6, i32 10, i32 4, i32 5>
  %m = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %m, <8 x i64> %c, <8 x i64> zeroinitializer
  ret <8 x i64> %res
}

; The mem variant of vpermt2 with a writemask
; CHECK-LABEL: test7mm:
; CHECK: vpermt2q {{\(.*\).* {%k[1-7]} {z}}}
define <8 x i64> @test7mm(<8 x i64> %a, <8 x i64> *%pb, i8 %mask) nounwind {
  %b = load <8 x i64>* %pb
  %c = shufflevector <8 x i64> %a, <8 x i64> %b, <8 x i32> <i32 2, i32 8, i32 0, i32 1, i32 6, i32 10, i32 4, i32 5>
  %m = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %m, <8 x i64> %c, <8 x i64> zeroinitializer
  ret <8 x i64> %res
}

; CHECK-LABEL: test8:
; CHECK: vpermt2d
; CHECK: ret
define <16 x i32> @test8(<16 x i32> %a, <16 x i32> %b) nounwind {
  %c = shufflevector <16 x i32> %a, <16 x i32> %b, <16 x i32> <i32 15, i32 31, i32 14, i32 22, i32 13, i32 29, i32 4, i32 28, i32 11, i32 27, i32 10, i32 26, i32 9, i32 25, i32 8, i32 24>
  ret <16 x i32> %c
}

; The reg variant of vpermt2 with a writemask
; CHECK-LABEL: test8m:
; CHECK: vpermt2d {{.* {%k[1-7]} {z}}}
define <16 x i32> @test8m(<16 x i32> %a, <16 x i32> %b, i16 %mask) nounwind {
  %c = shufflevector <16 x i32> %a, <16 x i32> %b, <16 x i32> <i32 15, i32 31, i32 14, i32 22, i32 13, i32 29, i32 4, i32 28, i32 11, i32 27, i32 10, i32 26, i32 9, i32 25, i32 8, i32 24>
  %m = bitcast i16 %mask to <16 x i1>
  %res = select <16 x i1> %m, <16 x i32> %c, <16 x i32> zeroinitializer
  ret <16 x i32> %res
}

; The mem variant of vpermt2 with a writemask
; CHECK-LABEL: test8mm:
; CHECK: vpermt2d {{\(.*\).* {%k[1-7]} {z}}}
define <16 x i32> @test8mm(<16 x i32> %a, <16 x i32> *%pb, i16 %mask) nounwind {
  %b = load <16 x i32> * %pb
  %c = shufflevector <16 x i32> %a, <16 x i32> %b, <16 x i32> <i32 15, i32 31, i32 14, i32 22, i32 13, i32 29, i32 4, i32 28, i32 11, i32 27, i32 10, i32 26, i32 9, i32 25, i32 8, i32 24>
  %m = bitcast i16 %mask to <16 x i1>
  %res = select <16 x i1> %m, <16 x i32> %c, <16 x i32> zeroinitializer
  ret <16 x i32> %res
}

; CHECK-LABEL: test9:
; CHECK: vpermt2ps
; CHECK: ret
define <16 x float> @test9(<16 x float> %a, <16 x float> %b) nounwind {
  %c = shufflevector <16 x float> %a, <16 x float> %b, <16 x i32> <i32 15, i32 31, i32 14, i32 22, i32 13, i32 29, i32 4, i32 28, i32 11, i32 27, i32 10, i32 26, i32 9, i32 25, i32 8, i32 24>
  ret <16 x float> %c
}

; The reg variant of vpermt2 with a writemask
; CHECK-LABEL: test9m:
; CHECK: vpermt2ps {{.*}} {%k{{.}}} {z}
define <16 x float> @test9m(<16 x float> %a, <16 x float> %b, i16 %mask) nounwind {
  %c = shufflevector <16 x float> %a, <16 x float> %b, <16 x i32> <i32 15, i32 31, i32 14, i32 22, i32 13, i32 29, i32 4, i32 28, i32 11, i32 27, i32 10, i32 26, i32 9, i32 25, i32 8, i32 24>
  %m = bitcast i16 %mask to <16 x i1>
  %res = select <16 x i1> %m, <16 x float> %c, <16 x float> zeroinitializer
  ret <16 x float> %res
}

; CHECK-LABEL: test10:
; CHECK: vpermt2ps (
; CHECK: ret
define <16 x float> @test10(<16 x float> %a, <16 x float>* %b) nounwind {
  %c = load <16 x float>* %b
  %d = shufflevector <16 x float> %a, <16 x float> %c, <16 x i32> <i32 15, i32 31, i32 14, i32 22, i32 13, i32 29, i32 4, i32 28, i32 11, i32 27, i32 10, i32 26, i32 9, i32 25, i32 8, i32 24>
  ret <16 x float> %d
}

; CHECK-LABEL: test11:
; CHECK: vpermt2d 
; CHECK: ret
define <16 x i32> @test11(<16 x i32> %a, <16 x i32>* %b) nounwind {
  %c = load <16 x i32>* %b
  %d = shufflevector <16 x i32> %a, <16 x i32> %c, <16 x i32> <i32 15, i32 31, i32 14, i32 22, i32 13, i32 29, i32 4, i32 28, i32 11, i32 27, i32 10, i32 26, i32 9, i32 25, i32 8, i32 24>
  ret <16 x i32> %d
}

; CHECK-LABEL: test12
; CHECK: vmovlhps {{.*}}## encoding: [0x62
; CHECK: ret
define <4 x i32> @test12(<4 x i32> %a, <4 x i32> %b) nounwind {
  %c = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 1, i32 4, i32 5>
  ret <4 x i32> %c
}

; CHECK-LABEL: test13
; CHECK: vpermilps $-79, %zmm
; CHECK: ret
define <16 x float> @test13(<16 x float> %a) {
 %b = shufflevector <16 x float> %a, <16 x float> undef, <16 x i32><i32 1, i32 0, i32 3, i32 2, i32 5, i32 4, i32 7, i32 6, i32 9, i32 8, i32 11, i32 10, i32 13, i32 12, i32 15, i32 14>
 ret <16 x float> %b
}

; CHECK-LABEL: test14
; CHECK: vpermilpd $-53, %zmm
; CHECK: ret
define <8 x double> @test14(<8 x double> %a) {
 %b = shufflevector <8 x double> %a, <8 x double> undef, <8 x i32><i32 1, i32 1, i32 2, i32 3, i32 4, i32 4, i32 7, i32 7>
 ret <8 x double> %b
}

; CHECK-LABEL: test15
; CHECK: vpshufd $-79, %zmm
; CHECK: ret
define <16 x i32> @test15(<16 x i32> %a) {
 %b = shufflevector <16 x i32> %a, <16 x i32> undef, <16 x i32><i32 1, i32 0, i32 3, i32 2, i32 5, i32 4, i32 7, i32 6, i32 9, i32 8, i32 11, i32 10, i32 13, i32 12, i32 15, i32 14>
 ret <16 x i32> %b
}
; CHECK-LABEL: test16
; CHECK: valignq $2, %zmm0, %zmm1
; CHECK: ret
define <8 x double> @test16(<8 x double> %a, <8 x double> %b) nounwind {
  %c = shufflevector <8 x double> %a, <8 x double> %b, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9>
  ret <8 x double> %c
}

; CHECK-LABEL: test16k
; CHECK: valignq $2, %zmm0, %zmm1, %zmm2 {%k1} #
define <8 x i64> @test16k(<8 x i64> %a, <8 x i64> %b, <8 x i64> %src, i8 %mask) nounwind {
  %c = shufflevector <8 x i64> %a, <8 x i64> %b, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9>
  %m = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %m, <8 x i64> %c, <8 x i64> %src
  ret <8 x i64> %res
}

; CHECK-LABEL: test16kz
; CHECK: valignq $2, %zmm0, %zmm1, %zmm0 {%k1} {z} ## encoding: [0x62,0xf3,0xf5,0xc9,0x03,0xc0,0x02]
define <8 x i64> @test16kz(<8 x i64> %a, <8 x i64> %b, i8 %mask) nounwind {
  %c = shufflevector <8 x i64> %a, <8 x i64> %b, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9>
  %m = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %m, <8 x i64> %c, <8 x i64> zeroinitializer
  ret <8 x i64> %res
}

; CHECK-LABEL: test17
; CHECK: vshufpd $19, %zmm1, %zmm0
; CHECK: ret
define <8 x double> @test17(<8 x double> %a, <8 x double> %b) nounwind {
  %c = shufflevector <8 x double> %a, <8 x double> %b, <8 x i32> <i32 1, i32 9, i32 2, i32 10, i32 5, i32 undef, i32 undef, i32 undef>
  ret <8 x double> %c
}

; CHECK-LABEL: test18
; CHECK: vpunpckhdq %zmm
; CHECK: ret
define <16 x i32> @test18(<16 x i32> %a, <16 x i32> %c) {
 %b = shufflevector <16 x i32> %a, <16 x i32> %c, <16 x i32><i32 2, i32 10, i32 3, i32 11, i32 6, i32 14, i32 7, i32 15, i32 18, i32 26, i32 19, i32 27, i32 22, i32 30, i32 23, i32 31>
 ret <16 x i32> %b
}

; CHECK-LABEL: test19
; CHECK: vpunpckldq %zmm
; CHECK: ret
define <16 x i32> @test19(<16 x i32> %a, <16 x i32> %c) {
 %b = shufflevector <16 x i32> %a, <16 x i32> %c, <16 x i32><i32 0, i32 8, i32 1, i32 9, i32 4, i32 12, i32 5, i32 13, i32 16, i32 24, i32 17, i32 25, i32 20, i32 28, i32 21, i32 29>
 ret <16 x i32> %b
}

; CHECK-LABEL: test20
; CHECK: vpunpckhqdq  %zmm
; CHECK: ret
define <8 x i64> @test20(<8 x i64> %a, <8 x i64> %c) {
 %b = shufflevector <8 x i64> %a, <8 x i64> %c, <8 x i32><i32 1, i32 5, i32 3, i32 7, i32 9, i32 13, i32 11, i32 15>
 ret <8 x i64> %b
}

; CHECK-LABEL: test21
; CHECK: vunpcklps %zmm
; CHECK: ret
define <16 x float> @test21(<16 x float> %a, <16 x float> %c) {
 %b = shufflevector <16 x float> %a, <16 x float> %c, <16 x i32><i32 0, i32 8, i32 1, i32 9, i32 4, i32 12, i32 5, i32 13, i32 16, i32 24, i32 17, i32 25, i32 20, i32 28, i32 21, i32 29>
 ret <16 x float> %b
}

; CHECK-LABEL: test22
; CHECK: vmovhlps {{.*}}## encoding: [0x62
; CHECK: ret
define <4 x i32> @test22(<4 x i32> %a, <4 x i32> %b) nounwind {
  %c = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 2, i32 3, i32 6, i32 7>
  ret <4 x i32> %c
}

; CHECK-LABEL: @test23
; CHECK: vshufps $-112, %zmm
; CHECK: ret
define <16 x float> @test23(<16 x float> %a, <16 x float> %c) {
 %b = shufflevector <16 x float> %a, <16 x float> %c, <16 x i32><i32 0, i32 0, i32 17, i32 18, i32 4, i32 4, i32 21, i32 22, i32 8, i32 8, i32 25, i32 26, i32 12, i32 12, i32 29, i32 30>
 ret <16 x float> %b
}

; CHECK-LABEL: @test24
; CHECK: vpermt2d
; CHECK: ret
define <16 x i32> @test24(<16 x i32> %a, <16 x i32> %b) nounwind {
  %c = shufflevector <16 x i32> %a, <16 x i32> %b, <16 x i32> <i32 0, i32 1, i32 2, i32 19, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  ret <16 x i32> %c
}

; CHECK-LABEL: @test25
; CHECK: vshufps  $52
; CHECK: ret
define <16 x i32> @test25(<16 x i32> %a, <16 x i32> %b) nounwind {
  %c = shufflevector <16 x i32> %a, <16 x i32> %b, <16 x i32> <i32 0, i32 1, i32 19, i32 undef, i32 4, i32 5, i32 23, i32 undef, i32 8, i32 9, i32 27, i32 undef, i32 12, i32 13, i32 undef, i32 undef>
  ret <16 x i32> %c
}

; CHECK-LABEL: @test26
; CHECK: vmovshdup
; CHECK: ret
define <16 x i32> @test26(<16 x i32> %a) nounwind {
  %c = shufflevector <16 x i32> %a, <16 x i32> undef, <16 x i32> <i32 1, i32 1, i32 3, i32 3, i32 5, i32 5, i32 7, i32 undef, i32 9, i32 9, i32 undef, i32 11, i32 13, i32 undef, i32 undef, i32 undef>
  ret <16 x i32> %c
}

; CHECK-LABEL: @test27
; CHECK: ret
define <16 x i32> @test27(<4 x i32>%a) {
 %res = shufflevector <4 x i32> %a, <4 x i32> undef, <16 x i32> <i32 0, i32 1, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
 ret <16 x i32> %res
}

; CHECK-LABEL: @test28
; CHECK: vinserti64x4 $1
; CHECK: ret
define <16 x i32> @test28(<16 x i32>%x, <16 x i32>%y) {
 %res = shufflevector <16 x i32>%x, <16 x i32>%y, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7,
                                                              i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23>
 ret <16 x i32> %res
}

; CHECK-LABEL: @test29
; CHECK: vinserti64x4 $0
; CHECK: ret
define <16 x i32> @test29(<16 x i32>%x, <16 x i32>%y) {
 %res = shufflevector <16 x i32>%x, <16 x i32>%y, <16 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                                                              i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
 ret <16 x i32> %res
}

