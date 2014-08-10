; REQUIRES: object-emission

; RUN: %llc_dwarf -O0 -filetype=obj %s -o - | llvm-dwarfdump -debug-dump=info - | FileCheck %s
; Use correct signedness when emitting constants of derived (sugared) types.

; Test compiled to IR from clang with -O1 and the following source:

; void func(int);
; void func(unsigned);
; void func(char16_t);
; int main() {
;   const int i = 42;
;   func(i);
;   const unsigned j = 117;
;   func(j);
;   char16_t c = 7;
;   func(c);
; }

; CHECK: DW_AT_const_value [DW_FORM_sdata] (42)
; CHECK: DW_AT_const_value [DW_FORM_udata] (117)
; CHECK: DW_AT_const_value [DW_FORM_udata] (7)

; Function Attrs: uwtable
define i32 @main() #0 {
entry:
  tail call void @llvm.dbg.value(metadata !20, i64 0, metadata !10), !dbg !21
  tail call void @_Z4funci(i32 42), !dbg !22
  tail call void @llvm.dbg.value(metadata !23, i64 0, metadata !12), !dbg !24
  tail call void @_Z4funcj(i32 117), !dbg !25
  tail call void @llvm.dbg.value(metadata !26, i64 0, metadata !15), !dbg !27
  tail call void @_Z4funcDs(i16 zeroext 7), !dbg !28
  ret i32 0, !dbg !29
}

declare void @_Z4funci(i32) #1

declare void @_Z4funcj(i32) #1

declare void @_Z4funcDs(i16 zeroext) #1

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata) #2

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!17, !18}
!llvm.ident = !{!19}

!0 = metadata !{i32 786449, metadata !1, i32 4, metadata !"clang version 3.5.0 ", i1 true, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/tmp/dbginfo/const.cpp] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !"const.cpp", metadata !"/tmp/dbginfo"}
!2 = metadata !{}
!3 = metadata !{metadata !4}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"main", metadata !"main", metadata !"", i32 4, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, i32 ()* @main, null, null, metadata !9, i32 4} ; [ DW_TAG_subprogram ] [line 4] [def] [main]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [/tmp/dbginfo/const.cpp]
!6 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !7, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{metadata !8}
!8 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!9 = metadata !{metadata !10, metadata !12, metadata !15}
!10 = metadata !{i32 786688, metadata !4, metadata !"i", metadata !5, i32 5, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 5]
!11 = metadata !{i32 786470, null, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, metadata !8} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from int]
!12 = metadata !{i32 786688, metadata !4, metadata !"j", metadata !5, i32 7, metadata !13, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 7]
!13 = metadata !{i32 786470, null, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, metadata !14} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from unsigned int]
!14 = metadata !{i32 786468, null, null, metadata !"unsigned int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 7} ; [ DW_TAG_base_type ] [unsigned int] [line 0, size 32, align 32, offset 0, enc DW_ATE_unsigned]
!15 = metadata !{i32 786688, metadata !4, metadata !"c", metadata !5, i32 9, metadata !16, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [c] [line 9]
!16 = metadata !{i32 786468, null, null, metadata !"char16_t", i32 0, i64 16, i64 16, i64 0, i32 0, i32 16} ; [ DW_TAG_base_type ] [char16_t] [line 0, size 16, align 16, offset 0, enc DW_ATE_UTF]
!17 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!18 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!19 = metadata !{metadata !"clang version 3.5.0 "}
!20 = metadata !{i32 42}
!21 = metadata !{i32 5, i32 0, metadata !4, null}
!22 = metadata !{i32 6, i32 0, metadata !4, null}
!23 = metadata !{i32 117}
!24 = metadata !{i32 7, i32 0, metadata !4, null}
!25 = metadata !{i32 8, i32 0, metadata !4, null} ; [ DW_TAG_imported_declaration ]
!26 = metadata !{i16 7}
!27 = metadata !{i32 9, i32 0, metadata !4, null}
!28 = metadata !{i32 10, i32 0, metadata !4, null}
!29 = metadata !{i32 11, i32 0, metadata !4, null}
