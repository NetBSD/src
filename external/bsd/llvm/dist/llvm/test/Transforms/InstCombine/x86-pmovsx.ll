; RUN: opt < %s -instcombine -S | FileCheck %s

declare <4 x i32>  @llvm.x86.sse41.pmovsxbd(<16 x i8>) nounwind readnone
declare <2 x i64>  @llvm.x86.sse41.pmovsxbq(<16 x i8>) nounwind readnone
declare <8 x i16>  @llvm.x86.sse41.pmovsxbw(<16 x i8>) nounwind readnone
declare <2 x i64>  @llvm.x86.sse41.pmovsxdq(<4 x i32>) nounwind readnone
declare <4 x i32>  @llvm.x86.sse41.pmovsxwd(<8 x i16>) nounwind readnone
declare <2 x i64>  @llvm.x86.sse41.pmovsxwq(<8 x i16>) nounwind readnone

declare <8 x i32>  @llvm.x86.avx2.pmovsxbd(<16 x i8>) nounwind readnone
declare <4 x i64>  @llvm.x86.avx2.pmovsxbq(<16 x i8>) nounwind readnone
declare <16 x i16> @llvm.x86.avx2.pmovsxbw(<16 x i8>) nounwind readnone
declare <4 x i64>  @llvm.x86.avx2.pmovsxdq(<4 x i32>) nounwind readnone
declare <8 x i32>  @llvm.x86.avx2.pmovsxwd(<8 x i16>) nounwind readnone
declare <4 x i64>  @llvm.x86.avx2.pmovsxwq(<8 x i16>) nounwind readnone

;
; Basic sign extension tests
;

define <4 x i32> @sse41_pmovsxbd(<16 x i8> %v) nounwind readnone {
; CHECK-LABEL: @sse41_pmovsxbd
; CHECK-NEXT:  shufflevector <16 x i8> %v, <16 x i8> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT:  sext <4 x i8> %1 to <4 x i32>
; CHECK-NEXT:  ret <4 x i32> %2

  %res = call <4 x i32> @llvm.x86.sse41.pmovsxbd(<16 x i8> %v)
  ret <4 x i32> %res
}

define <2 x i64> @sse41_pmovsxbq(<16 x i8> %v) nounwind readnone {
; CHECK-LABEL: @sse41_pmovsxbq
; CHECK-NEXT:  shufflevector <16 x i8> %v, <16 x i8> undef, <2 x i32> <i32 0, i32 1>
; CHECK-NEXT:  sext <2 x i8> %1 to <2 x i64>
; CHECK-NEXT:  ret <2 x i64> %2

  %res = call <2 x i64> @llvm.x86.sse41.pmovsxbq(<16 x i8> %v)
  ret <2 x i64> %res
}

define <8 x i16> @sse41_pmovsxbw(<16 x i8> %v) nounwind readnone {
; CHECK-LABEL: @sse41_pmovsxbw
; CHECK-NEXT:  shufflevector <16 x i8> %v, <16 x i8> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:  sext <8 x i8> %1 to <8 x i16>
; CHECK-NEXT:  ret <8 x i16> %2

  %res = call <8 x i16> @llvm.x86.sse41.pmovsxbw(<16 x i8> %v)
  ret <8 x i16> %res
}

define <2 x i64> @sse41_pmovsxdq(<4 x i32> %v) nounwind readnone {
; CHECK-LABEL: @sse41_pmovsxdq
; CHECK-NEXT:  shufflevector <4 x i32> %v, <4 x i32> undef, <2 x i32> <i32 0, i32 1>
; CHECK-NEXT:  sext <2 x i32> %1 to <2 x i64>
; CHECK-NEXT:  ret <2 x i64> %2

  %res = call <2 x i64> @llvm.x86.sse41.pmovsxdq(<4 x i32> %v)
  ret <2 x i64> %res
}

define <4 x i32> @sse41_pmovsxwd(<8 x i16> %v) nounwind readnone {
; CHECK-LABEL: @sse41_pmovsxwd
; CHECK-NEXT:  shufflevector <8 x i16> %v, <8 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT:  sext <4 x i16> %1 to <4 x i32>
; CHECK-NEXT:  ret <4 x i32> %2

  %res = call <4 x i32> @llvm.x86.sse41.pmovsxwd(<8 x i16> %v)
  ret <4 x i32> %res
}

define <2 x i64> @sse41_pmovsxwq(<8 x i16> %v) nounwind readnone {
; CHECK-LABEL: @sse41_pmovsxwq
; CHECK-NEXT:  shufflevector <8 x i16> %v, <8 x i16> undef, <2 x i32> <i32 0, i32 1>
; CHECK-NEXT:  sext <2 x i16> %1 to <2 x i64>
; CHECK-NEXT:  ret <2 x i64> %2

  %res = call <2 x i64> @llvm.x86.sse41.pmovsxwq(<8 x i16> %v)
  ret <2 x i64> %res
}

define <8 x i32> @avx2_pmovsxbd(<16 x i8> %v) nounwind readnone {
; CHECK-LABEL: @avx2_pmovsxbd
; CHECK-NEXT:  shufflevector <16 x i8> %v, <16 x i8> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:  sext <8 x i8> %1 to <8 x i32>
; CHECK-NEXT:  ret <8 x i32> %2

  %res = call <8 x i32> @llvm.x86.avx2.pmovsxbd(<16 x i8> %v)
  ret <8 x i32> %res
}

define <4 x i64> @avx2_pmovsxbq(<16 x i8> %v) nounwind readnone {
; CHECK-LABEL: @avx2_pmovsxbq
; CHECK-NEXT:  shufflevector <16 x i8> %v, <16 x i8> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT:  sext <4 x i8> %1 to <4 x i64>
; CHECK-NEXT:  ret <4 x i64> %2

  %res = call <4 x i64> @llvm.x86.avx2.pmovsxbq(<16 x i8> %v)
  ret <4 x i64> %res
}

define <16 x i16> @avx2_pmovsxbw(<16 x i8> %v) nounwind readnone {
; CHECK-LABEL: @avx2_pmovsxbw
; CHECK-NEXT:  sext <16 x i8> %v to <16 x i16>
; CHECK-NEXT:  ret <16 x i16> %1

  %res = call <16 x i16> @llvm.x86.avx2.pmovsxbw(<16 x i8> %v)
  ret <16 x i16> %res
}

define <4 x i64> @avx2_pmovsxdq(<4 x i32> %v) nounwind readnone {
; CHECK-LABEL: @avx2_pmovsxdq
; CHECK-NEXT:  sext <4 x i32> %v to <4 x i64>
; CHECK-NEXT:  ret <4 x i64> %1

  %res = call <4 x i64> @llvm.x86.avx2.pmovsxdq(<4 x i32> %v)
  ret <4 x i64> %res
}

define <8 x i32> @avx2_pmovsxwd(<8 x i16> %v) nounwind readnone {
; CHECK-LABEL: @avx2_pmovsxwd
; CHECK-NEXT:  sext <8 x i16> %v to <8 x i32>
; CHECK-NEXT:  ret <8 x i32> %1

  %res = call <8 x i32> @llvm.x86.avx2.pmovsxwd(<8 x i16> %v)
  ret <8 x i32> %res
}

define <4 x i64> @avx2_pmovsxwq(<8 x i16> %v) nounwind readnone {
; CHECK-LABEL: @avx2_pmovsxwq
; CHECK-NEXT:  shufflevector <8 x i16> %v, <8 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT:  sext <4 x i16> %1 to <4 x i64>
; CHECK-NEXT:  ret <4 x i64> %2

  %res = call <4 x i64> @llvm.x86.avx2.pmovsxwq(<8 x i16> %v)
  ret <4 x i64> %res
}
