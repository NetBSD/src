; PR30693
; RUN: llc < %s | FileCheck %s

; CHECK:      .p2align	2
; CHECK-NEXT: .LCPI0_0:
; CHECK-NOT:  vmovaps	.LCPI0_0(%rip),
; CHECK:      .cfi_endproc
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@var_35 = external local_unnamed_addr global i32, align 4
@var_14 = external local_unnamed_addr global i16, align 2

; Function Attrs: uwtable
define void @_Z3foov() local_unnamed_addr #0 {
entry:
  %0 = load i32, i32* @var_35, align 4
  %1 = load i16, i16* @var_14, align 2
  %conv34 = zext i16 %1 to i64
  %conv37 = ashr exact i64 undef, 32
  %sub316 = add i16 undef, -7198
  %cmp339981 = icmp sgt i32 undef, 0
  %cmp401989 = icmp sgt i32 undef, 0
  %cmp443994 = icmp sgt i32 undef, 0
  %lcmp.mod = icmp eq i64 undef, 0
  %broadcast.splat1461 = shufflevector <32 x i16> undef, <32 x i16> undef, <32 x i32> zeroinitializer
  %broadcast.splat1357 = shufflevector <32 x i16> undef, <32 x i16> undef, <32 x i32> zeroinitializer
  %broadcast.splat1435 = shufflevector <32 x i16> undef, <32 x i16> undef, <32 x i32> zeroinitializer
  %broadcast.splat1409 = shufflevector <32 x i16> undef, <32 x i16> undef, <32 x i32> zeroinitializer
  br label %for.cond11.preheader

for.cond11.preheader:                             ; preds = %for.cond.cleanup477.loopexit, %entry
  %div = sdiv i32 0, 0
  %mul31 = mul nsw i32 %div, %0
  %conv32 = sext i32 %mul31 to i64
  %div40 = sdiv i64 0, 0
  %div41 = sdiv i32 0, 0
  %conv42 = sext i32 %div41 to i64
  %mul43 = mul nsw i64 %conv32, %conv34
  %mul44 = mul i64 %mul43, %div40
  %mul45 = mul i64 %mul44, %conv37
  %mul46 = mul i64 %mul45, %conv42
  %add48 = add nsw i64 %mul46, 36611
  %conv49 = trunc i64 %add48 to i16
  br label %vector.ph1520

vector.ph1520:                                    ; preds = %for.cond11.preheader
  %broadcast.splatinsert1531 = insertelement <32 x i16> undef, i16 %conv49, i32 0
  %broadcast.splat1532 = shufflevector <32 x i16> %broadcast.splatinsert1531, <32 x i16> undef, <32 x i32> zeroinitializer
  br i1 %lcmp.mod, label %vector.body1512.prol.loopexit, label %vector.body1512.prol.preheader

vector.body1512.prol.preheader:                   ; preds = %vector.ph1520
  store <32 x i16> %broadcast.splat1532, <32 x i16>* undef, align 8, !tbaa !1
  unreachable

vector.body1512.prol.loopexit:                    ; preds = %vector.ph1520
  %add318 = add i16 %sub316, 0
  %2 = insertelement <16 x i16> undef, i16 %add318, i32 7
  %3 = insertelement <16 x i16> %2, i16 %add318, i32 8
  %4 = insertelement <16 x i16> %3, i16 %add318, i32 9
  %5 = insertelement <16 x i16> %4, i16 %add318, i32 10
  %6 = insertelement <16 x i16> %5, i16 %add318, i32 11
  %7 = insertelement <16 x i16> %6, i16 %add318, i32 12
  %8 = insertelement <16 x i16> %7, i16 %add318, i32 13
  %9 = insertelement <16 x i16> %8, i16 %add318, i32 14
  %10 = insertelement <16 x i16> undef, i16 %add318, i32 7
  %11 = insertelement <16 x i16> %10, i16 %add318, i32 8
  %12 = insertelement <16 x i16> %11, i16 %add318, i32 9
  %13 = insertelement <16 x i16> %12, i16 %add318, i32 10
  %14 = insertelement <16 x i16> %13, i16 %add318, i32 11
  %15 = insertelement <16 x i16> %14, i16 %add318, i32 12
  %16 = insertelement <16 x i16> %15, i16 %add318, i32 13
  %17 = insertelement <16 x i16> %16, i16 %add318, i32 14
  %18 = insertelement <16 x i16> %17, i16 %add318, i32 15
  %19 = insertelement <8 x i16> undef, i16 %add318, i32 7
  br label %for.cond74.loopexit.us

for.cond337.preheader.lr.ph:                      ; preds = %for.cond130.preheader.loopexit
  br i1 %cmp339981, label %for.cond337.preheader.us.preheader, label %for.cond.cleanup335

for.cond337.preheader.us.preheader:               ; preds = %for.cond337.preheader.lr.ph
  store <32 x i16> %broadcast.splat1461, <32 x i16>* undef, align 4, !tbaa !1
  unreachable

for.cond74.loopexit.us:                           ; preds = %for.cond74.loopexit.us, %vector.body1512.prol.loopexit
  store <8 x i16> zeroinitializer, <8 x i16>* undef, align 2, !tbaa !1
  %cmp76.us = icmp slt i64 undef, undef
  br i1 %cmp76.us, label %for.cond74.loopexit.us, label %for.cond130.preheader.loopexit

for.cond130.preheader.loopexit:                   ; preds = %for.cond74.loopexit.us
  store <16 x i16> zeroinitializer, <16 x i16>* undef, align 2, !tbaa !1
  store <16 x i16> %18, <16 x i16>* undef, align 2, !tbaa !1
  store <8 x i16> %19, <8 x i16>* undef, align 2, !tbaa !1
  br label %for.cond337.preheader.lr.ph

for.cond.cleanup335:                              ; preds = %for.cond337.preheader.lr.ph
  br label %for.cond380.preheader

for.cond380.preheader:                            ; preds = %for.cond.cleanup335
  br label %for.cond385.preheader

for.cond.cleanup378.loopexit:                     ; preds = %for.cond.cleanup388
  br label %for.cond481.preheader

for.cond385.preheader:                            ; preds = %for.cond380.preheader
  br i1 %cmp443994, label %for.cond392.preheader.us.preheader, label %for.cond392.preheader.preheader

for.cond392.preheader.preheader:                  ; preds = %for.cond385.preheader
  store <32 x i16> %broadcast.splat1435, <32 x i16>* undef, align 4, !tbaa !1
  store <32 x i16> %broadcast.splat1409, <32 x i16>* undef, align 4, !tbaa !1
  unreachable

for.cond392.preheader.us.preheader:               ; preds = %for.cond385.preheader
  br label %for.cond399.preheader.lr.ph.us.1

for.cond.cleanup388:                              ; preds = %for.cond399.preheader.lr.ph.us.1
  br label %for.cond.cleanup378.loopexit

for.cond481.preheader:                            ; preds = %for.cond.cleanup486, %for.cond.cleanup378.loopexit
  br label %for.cond.cleanup486

for.cond.cleanup477.loopexit:                     ; preds = %for.cond.cleanup486
  store <8 x i32> <i32 1221902566, i32 1221902566, i32 1221902566, i32 1221902566, i32 1221902566, i32 1221902566, i32 1221902566, i32 1221902566>, <8 x i32>* undef, align 4, !tbaa !5
  br label %for.cond11.preheader

for.cond.cleanup486:                              ; preds = %for.cond481.preheader
  br i1 undef, label %for.cond481.preheader, label %for.cond.cleanup477.loopexit

for.cond399.preheader.lr.ph.us.1:                 ; preds = %for.cond392.preheader.us.preheader
  br i1 %cmp401989, label %for.cond399.preheader.us.us.1.preheader, label %for.cond.cleanup388

for.cond399.preheader.us.us.1.preheader:          ; preds = %for.cond399.preheader.lr.ph.us.1
  store <32 x i16> %broadcast.splat1357, <32 x i16>* undef, align 4, !tbaa !1
  unreachable
}

attributes #0 = { uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="knl" "target-features"="+adx,+aes,+avx,+avx2,+avx512cd,+avx512er,+avx512f,+avx512pf,+bmi,+bmi2,+cx16,+f16c,+fma,+fsgsbase,+fxsr,+lzcnt,+mmx,+movbe,+pclmul,+popcnt,+prefetchwt1,+rdrnd,+rdseed,+rtm,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave,+xsaveopt" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.0 (http://llvm.org/git/clang.git ef66d4d58b9a2c6b3d31bbaf3ed2a70a9754a137) (http://llvm.org/git/llvm.git 5e661621191d6133a12effa103bfb2cbbdbb35ad)"}
!1 = !{!2, !2, i64 0}
!2 = !{!"short", !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C++ TBAA"}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !3, i64 0}
