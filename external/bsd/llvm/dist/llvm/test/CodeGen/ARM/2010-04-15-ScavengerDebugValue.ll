; RUN: llc < %s
; PR6847
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:64:64-v128:128:128-a0:0:64-n32"
target triple = "armv4t-apple-darwin10"

define hidden i32 @__addvsi3(i32 %a, i32 %b) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %b}, i64 0, metadata !0)
  %0 = add nsw i32 %b, %a, !dbg !9                ; <i32> [#uses=1]
  ret i32 %0, !dbg !11
}

declare void @llvm.dbg.value(metadata, i64, metadata) nounwind readnone

!llvm.dbg.cu = !{!3}
!llvm.module.flags = !{!15}
!0 = metadata !{i32 524545, metadata !1, metadata !"b", metadata !2, i32 93, metadata !6, i32 0, null} ; [ DW_TAG_arg_variable ]
!1 = metadata !{i32 524334, metadata !12, null, metadata !"__addvsi3", metadata !"__addvsi3", metadata !"__addvsi3", i32 94, metadata !4, i1 false, i1 true, i32 0, i32 0, null, i32 0, i32 0, null, null, null, null, i32 0} ; [ DW_TAG_subprogram ]
!2 = metadata !{i32 524329, metadata !12} ; [ DW_TAG_file_type ]
!12 = metadata !{metadata !"libgcc2.c", metadata !"/Users/bwilson/local/nightly/test-2010-04-14/build/llvmgcc.roots/llvmgcc~obj/src/gcc"}
!3 = metadata !{i32 524305, metadata !12, i32 1, metadata !"4.2.1 (Based on Apple Inc. build 5658) (LLVM build 00)", i1 true, metadata !"", i32 0, metadata !13, metadata !13, metadata !14, null, null, metadata !""} ; [ DW_TAG_compile_unit ]
!4 = metadata !{i32 524309, metadata !12, metadata !2, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !5, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!5 = metadata !{metadata !6, metadata !6, metadata !6}
!6 = metadata !{i32 524310, metadata !12, null, metadata !"SItype", i32 152, i64 0, i64 0, i64 0, i32 0, metadata !8} ; [ DW_TAG_typedef ]
!7 = metadata !{i32 524329, metadata !"libgcc2.h", metadata !"/Users/bwilson/local/nightly/test-2010-04-14/build/llvmgcc.roots/llvmgcc~obj/src/gcc", metadata !3} ; [ DW_TAG_file_type ]
!8 = metadata !{i32 524324, metadata !12, metadata !2, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ]
!9 = metadata !{i32 95, i32 0, metadata !10, null}
!10 = metadata !{i32 524299, metadata !12, metadata !1, i32 94, i32 0, i32 0} ; [ DW_TAG_lexical_block ]
!11 = metadata !{i32 100, i32 0, metadata !10, null}
!13 = metadata !{i32 0}
!14 = metadata !{metadata !1}
!15 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
