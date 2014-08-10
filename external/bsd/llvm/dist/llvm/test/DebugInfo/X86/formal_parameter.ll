; ModuleID = 'formal_parameter.c'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.9.0"
;
; From (clang -g -c -O1):
;
; int lookup(int* map);
; int verify(int val);
; void foo(int map)
; {
;   lookup(&map);
;   if (!verify(map)) {  }
; }
;
; RUN: opt %s -O2 -S -o %t
; RUN: cat %t | FileCheck --check-prefix=LOWERING %s
; RUN: llc -filetype=obj %t -o - | llvm-dwarfdump -debug-dump=info - | FileCheck %s
; Test that we only emit only one DW_AT_formal_parameter "map" for this function.
; rdar://problem/14874886
;
; CHECK: DW_TAG_formal_parameter
; CHECK-NOT: DW_TAG
; CHECK: DW_AT_name {{.*}}map
; CHECK-NOT: DW_AT_name {{.*}}map

; Function Attrs: nounwind ssp uwtable
define void @foo(i32 %map) #0 {
entry:
  %map.addr = alloca i32, align 4
  store i32 %map, i32* %map.addr, align 4, !tbaa !15
  call void @llvm.dbg.declare(metadata !{i32* %map.addr}, metadata !10), !dbg !14
  %call = call i32 (i32*, ...)* bitcast (i32 (...)* @lookup to i32 (i32*, ...)*)(i32* %map.addr) #3, !dbg !19
  ; Ensure that all dbg intrinsics have the same scope after
  ; LowerDbgDeclare is finished with them.
  ;
  ; LOWERING: call void @llvm.dbg.value{{.*}}, !dbg ![[LOC:.*]]
  ; LOWERING: call void @llvm.dbg.value{{.*}}, !dbg ![[LOC]]
  ; LOWERING: call void @llvm.dbg.value{{.*}}, !dbg ![[LOC]]
%0 = load i32* %map.addr, align 4, !dbg !20, !tbaa !15
  %call1 = call i32 (i32, ...)* bitcast (i32 (...)* @verify to i32 (i32, ...)*)(i32 %0) #3, !dbg !20
  ret void, !dbg !22
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #1

declare i32 @lookup(...)

declare i32 @verify(...)

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata) #1

attributes #0 = { nounwind ssp uwtable }
attributes #1 = { nounwind readnone }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!11, !12}
!llvm.ident = !{!13}

!0 = metadata !{i32 786449, metadata !1, i32 12, metadata !"clang version 3.5.0 ", i1 true, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [formal_parameter.c] [DW_LANG_C99]
!1 = metadata !{metadata !"formal_parameter.c", metadata !""}
!2 = metadata !{}
!3 = metadata !{metadata !4}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"foo", metadata !"foo", metadata !"", i32 1, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32)* @foo, null, null, metadata !9, i32 2} ; [ DW_TAG_subprogram ] [line 1] [def] [scope 2] [foo]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [formal_parameter.c]
!6 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !7, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{null, metadata !8}
!8 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!9 = metadata !{metadata !10}
!10 = metadata !{i32 786689, metadata !4, metadata !"map", metadata !5, i32 16777217, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [map] [line 1]
!11 = metadata !{i32 2, metadata !"Dwarf Version", i32 2}
!12 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!13 = metadata !{metadata !"clang version 3.5.0 "}
!14 = metadata !{i32 1, i32 0, metadata !4, null}
!15 = metadata !{metadata !16, metadata !16, i64 0}
!16 = metadata !{metadata !"int", metadata !17, i64 0}
!17 = metadata !{metadata !"omnipotent char", metadata !18, i64 0}
!18 = metadata !{metadata !"Simple C/C++ TBAA"}
!19 = metadata !{i32 3, i32 0, metadata !4, null}
!20 = metadata !{i32 4, i32 0, metadata !21, null}
!21 = metadata !{i32 786443, metadata !1, metadata !4, i32 4, i32 0, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [formal_parameter.c]
!22 = metadata !{i32 5, i32 0, metadata !4, null}
