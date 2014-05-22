; RUN: llc < %s -mcpu=corei7 -march=x86 -mattr=+sse4.1 -o %t
; RUN: not grep extractps   %t
; RUN: not grep pextrd      %t
; RUN: not grep pshufd  %t
; RUN: not grep movss   %t

define void @t1(float* %R, <4 x float>* %P1) nounwind {
	%X = load <4 x float>* %P1
	%tmp = extractelement <4 x float> %X, i32 3
	store float %tmp, float* %R
	ret void
}

define float @t2(<4 x float>* %P1) nounwind {
	%X = load <4 x float>* %P1
	%tmp = extractelement <4 x float> %X, i32 2
	ret float %tmp
}

define void @t3(i32* %R, <4 x i32>* %P1) nounwind {
	%X = load <4 x i32>* %P1
	%tmp = extractelement <4 x i32> %X, i32 3
	store i32 %tmp, i32* %R
	ret void
}

define i32 @t4(<4 x i32>* %P1) nounwind {
	%X = load <4 x i32>* %P1
	%tmp = extractelement <4 x i32> %X, i32 3
	ret i32 %tmp
}
