; RUN: llc < %s -march=x86 -mattr=sse4.1 -o %t
; RUN: grep unpcklps %t | count 3
; RUN: grep unpckhps %t | count 1
 
; Transpose example using the more generic vector shuffle.  We return
; float8 instead of float16 since x86 can return that in register.
; ModuleID = 'transpose2_opt.bc'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:32:32"
target triple = "i386-apple-cl.1.0"
@r0 = common global <4 x float> zeroinitializer, align 16		; <<4 x float>*> [#uses=1]
@r1 = common global <4 x float> zeroinitializer, align 16		; <<4 x float>*> [#uses=1]
@r2 = common global <4 x float> zeroinitializer, align 16		; <<4 x float>*> [#uses=1]
@r3 = common global <4 x float> zeroinitializer, align 16		; <<4 x float>*> [#uses=1]

define <8 x float> @__transpose2(<4 x float> %p0, <4 x float> %p1, <4 x float> %p2, <4 x float> %p3) nounwind {
entry:
	%unpcklps = shufflevector <4 x float> %p0, <4 x float> %p2, <4 x i32> < i32 0, i32 4, i32 1, i32 5 >		; <<4 x float>> [#uses=2]
	%unpckhps = shufflevector <4 x float> %p0, <4 x float> %p2, <4 x i32> < i32 2, i32 6, i32 3, i32 7 >		; <<4 x float>> [#uses=2]
	%unpcklps8 = shufflevector <4 x float> %p1, <4 x float> %p3, <4 x i32> < i32 0, i32 4, i32 1, i32 5 >		; <<4 x float>> [#uses=2]
	%unpckhps11 = shufflevector <4 x float> %p1, <4 x float> %p3, <4 x i32> < i32 2, i32 6, i32 3, i32 7 >		; <<4 x float>> [#uses=2]
	%unpcklps14 = shufflevector <4 x float> %unpcklps, <4 x float> %unpcklps8, <4 x i32> < i32 0, i32 4, i32 1, i32 5 >		; <<4 x float>> [#uses=1]
	%unpcklps14a = shufflevector <4 x float> %unpcklps14,  <4 x float> undef,  <16 x i32> < i32 0, i32 1, i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
	%unpckhps17 = shufflevector <4 x float> %unpcklps, <4 x float> %unpcklps8, <4 x i32> < i32 2, i32 6, i32 3, i32 7 >		; <<4 x float>> [#uses=1]
	%unpckhps17a = shufflevector <4 x float> %unpckhps17,  <4 x float> undef, <16 x i32> < i32 0, i32 1, i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
	%r1 = shufflevector <16 x float> %unpcklps14a,  <16 x float> %unpckhps17a, <16 x i32> < i32 0, i32 1, i32 2, i32 3, i32 16, i32 17, i32 18, i32 19, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
	%unpcklps20 = shufflevector <4 x float> %unpckhps, <4 x float> %unpckhps11, <4 x i32> < i32 0, i32 4, i32 1, i32 5 >		; <<4 x float>> [#uses=1]
	%unpcklps20a = shufflevector <4 x float> %unpcklps20,  <4 x float> undef,  <16 x i32> < i32 0, i32 1, i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
	%r2 = shufflevector <16 x float> %r1,  <16 x float> %unpcklps20a, <16 x i32> < i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 16, i32 17, i32 18, i32 19, i32 12, i32 13, i32 14, i32 15>
	%unpckhps23 = shufflevector <4 x float> %unpckhps, <4 x float> %unpckhps11, <4 x i32> < i32 2, i32 6, i32 3, i32 7 >		; <<4 x float>> [#uses=1]
	%unpckhps23a = shufflevector <4 x float> %unpckhps23,  <4 x float> undef,  <16 x i32> < i32 0, i32 1, i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
	%r3 = shufflevector <16 x float> %r2,  <16 x float> %unpckhps23a, <16 x i32> < i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 16, i32 17, i32 18, i32 19>
	%r4 = shufflevector <16 x float> %r3,  <16 x float> undef, <8 x i32> < i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
	ret <8 x float> %r4
}
