; REQUIRES: object-emission

; RUN: %llc_dwarf -filetype=obj -O0 < %s | llvm-dwarfdump -debug-dump=info - | FileCheck %s

; This isn't a very pretty test case - I imagine there might be other ways to
; tickle the optimizers into producing the desired code, but I haven't found
; them.

; The issue is when a function is inlined into itself, the inlined argument
; accidentally overwrote the concrete argument and was lost.

; IR generated from the following source compiled with clang -g:
; void fn1(void *);
; void fn2(int, int, int, int);
; void fn3();
; void fn8();
; struct C {
;   int b;
;   void m_fn2() {
;     fn8();
;     if (b) fn2(0, 0, 0, 0);
;     fn3();
;   }
; };
; C *x;
; inline void fn7() {}
; void fn6() {
;   fn8();
;   x->m_fn2();
;   fn7();
; }
; void fn3() { fn6(); }
; void fn4() { x->m_fn2(); }
; void fn5() { x->m_fn2(); }

; The definition of C and declaration of C::m_fn2
; CHECK: DW_TAG_structure_type
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK: DW_TAG_member
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK: [[M_FN2_DECL:.*]]:   DW_TAG_subprogram
; CHECK-NOT: DW_TAG
; CHECK:     DW_AT_name {{.*}} "m_fn2"
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK: [[M_FN2_THIS_DECL:.*]]:     DW_TAG_formal_parameter

; The abstract definition of C::m_fn2
; CHECK: [[M_FN2_ABS_DEF:.*]]: DW_TAG_subprogram
; CHECK-NOT: DW_TAG
; CHECK:   DW_AT_specification {{.*}} {[[M_FN2_DECL]]}
; CHECK-NOT: DW_TAG
; CHECK:   DW_AT_inline
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK: [[M_FN2_THIS_ABS_DEF:.*]]:   DW_TAG_formal_parameter
; CHECK-NOT: DW_TAG
; CHECK:     DW_AT_name {{.*}} "this"

; Skip some other functions
; CHECK: DW_TAG_subprogram
; CHECK: DW_TAG_subprogram
; CHECK: DW_TAG_subprogram

; The concrete definition of C::m_fn2
; CHECK: DW_TAG_subprogram
; CHECK-NOT: DW_TAG
; CHECK:   DW_AT_abstract_origin {{.*}} {[[M_FN2_ABS_DEF]]}
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:   DW_TAG_formal_parameter
; CHECK-NOT: DW_TAG
; CHECK:     DW_AT_abstract_origin {{.*}} {[[M_FN2_THIS_ABS_DEF]]}
; CHECK-NOT: {{DW_TAG|NULL}}
; Inlined fn3:
; CHECK:     DW_TAG_inlined_subroutine
; CHECK-NOT: {{DW_TAG|NULL}}
; Inlined fn6:
; CHECK:       DW_TAG_inlined_subroutine
; CHECK-NOT: {{DW_TAG|NULL}}
; Inlined C::m_fn2:
; CHECK:         DW_TAG_inlined_subroutine
; CHECK-NOT: DW_TAG
; CHECK:           DW_AT_abstract_origin {{.*}} {[[M_FN2_ABS_DEF]]}
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:           DW_TAG_formal_parameter
; CHECK-NOT: DW_TAG
; CHECK:              DW_AT_abstract_origin {{.*}} {[[M_FN2_THIS_ABS_DEF]]}



%struct.C = type { i32 }

@x = global %struct.C* null, align 8

; Function Attrs: nounwind
define void @_Z3fn6v() #0 {
entry:
  tail call void @_Z3fn8v() #3, !dbg !31
  %0 = load %struct.C** @x, align 8, !dbg !32, !tbaa !33
  tail call void @llvm.dbg.value(metadata !{%struct.C* %0}, i64 0, metadata !37) #3, !dbg !38
  tail call void @_Z3fn8v() #3, !dbg !39
  %b.i = getelementptr inbounds %struct.C* %0, i64 0, i32 0, !dbg !40
  %1 = load i32* %b.i, align 4, !dbg !40, !tbaa !42
  %tobool.i = icmp eq i32 %1, 0, !dbg !40
  br i1 %tobool.i, label %_ZN1C5m_fn2Ev.exit, label %if.then.i, !dbg !40

if.then.i:                                        ; preds = %entry
  tail call void @_Z3fn2iiii(i32 0, i32 0, i32 0, i32 0) #3, !dbg !45
  br label %_ZN1C5m_fn2Ev.exit, !dbg !45

_ZN1C5m_fn2Ev.exit:                               ; preds = %entry, %if.then.i
  tail call void @_Z3fn3v() #3, !dbg !47
  ret void, !dbg !48
}

declare void @_Z3fn8v() #1

; Function Attrs: nounwind
define linkonce_odr void @_ZN1C5m_fn2Ev(%struct.C* nocapture readonly %this) #0 align 2 {
entry:
  tail call void @llvm.dbg.value(metadata !{%struct.C* %this}, i64 0, metadata !24), !dbg !49
  tail call void @_Z3fn8v() #3, !dbg !50
  %b = getelementptr inbounds %struct.C* %this, i64 0, i32 0, !dbg !51
  %0 = load i32* %b, align 4, !dbg !51, !tbaa !42
  %tobool = icmp eq i32 %0, 0, !dbg !51
  br i1 %tobool, label %if.end, label %if.then, !dbg !51

if.then:                                          ; preds = %entry
  tail call void @_Z3fn2iiii(i32 0, i32 0, i32 0, i32 0) #3, !dbg !52
  br label %if.end, !dbg !52

if.end:                                           ; preds = %entry, %if.then
  tail call void @_Z3fn8v() #3, !dbg !53
  %1 = load %struct.C** @x, align 8, !dbg !56, !tbaa !33
  tail call void @llvm.dbg.value(metadata !{%struct.C* %1}, i64 0, metadata !57) #3, !dbg !58
  tail call void @_Z3fn8v() #3, !dbg !59
  %b.i.i = getelementptr inbounds %struct.C* %1, i64 0, i32 0, !dbg !60
  %2 = load i32* %b.i.i, align 4, !dbg !60, !tbaa !42
  %tobool.i.i = icmp eq i32 %2, 0, !dbg !60
  br i1 %tobool.i.i, label %_Z3fn6v.exit, label %if.then.i.i, !dbg !60

if.then.i.i:                                      ; preds = %if.end
  tail call void @_Z3fn2iiii(i32 0, i32 0, i32 0, i32 0) #3, !dbg !61
  br label %_Z3fn6v.exit, !dbg !61

_Z3fn6v.exit:                                     ; preds = %if.end, %if.then.i.i
  tail call void @_Z3fn3v() #3, !dbg !62
  ret void, !dbg !63
}

; Function Attrs: nounwind
define void @_Z3fn3v() #0 {
entry:
  br label %tailrecurse

tailrecurse:                                      ; preds = %tailrecurse.backedge, %entry
  tail call void @_Z3fn8v() #3, !dbg !64
  %0 = load %struct.C** @x, align 8, !dbg !66, !tbaa !33
  tail call void @llvm.dbg.value(metadata !{%struct.C* %0}, i64 0, metadata !67) #3, !dbg !68
  tail call void @_Z3fn8v() #3, !dbg !69
  %b.i.i = getelementptr inbounds %struct.C* %0, i64 0, i32 0, !dbg !70
  %1 = load i32* %b.i.i, align 4, !dbg !70, !tbaa !42
  %tobool.i.i = icmp eq i32 %1, 0, !dbg !70
  br i1 %tobool.i.i, label %tailrecurse.backedge, label %if.then.i.i, !dbg !70

tailrecurse.backedge:                             ; preds = %tailrecurse, %if.then.i.i
  br label %tailrecurse

if.then.i.i:                                      ; preds = %tailrecurse
  tail call void @_Z3fn2iiii(i32 0, i32 0, i32 0, i32 0) #3, !dbg !71
  br label %tailrecurse.backedge, !dbg !71
}

; Function Attrs: nounwind
define void @_Z3fn4v() #0 {
entry:
  %0 = load %struct.C** @x, align 8, !dbg !72, !tbaa !33
  tail call void @_ZN1C5m_fn2Ev(%struct.C* %0), !dbg !72
  ret void, !dbg !72
}

; Function Attrs: nounwind
define void @_Z3fn5v() #0 {
entry:
  %0 = load %struct.C** @x, align 8, !dbg !73, !tbaa !33
  tail call void @_ZN1C5m_fn2Ev(%struct.C* %0), !dbg !73
  ret void, !dbg !73
}

declare void @_Z3fn2iiii(i32, i32, i32, i32) #1

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata) #2

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!28, !29}
!llvm.ident = !{!30}

!0 = metadata !{i32 786449, metadata !1, i32 4, metadata !"clang version 3.6.0 ", i1 true, metadata !"", i32 0, metadata !2, metadata !3, metadata !13, metadata !26, metadata !2, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/usr/local/google/home/blaikie/dev/scratch/missing_concrete_variable_on_darwin/reduce/<stdin>] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !"<stdin>", metadata !"/usr/local/google/home/blaikie/dev/scratch/missing_concrete_variable_on_darwin/reduce"}
!2 = metadata !{}
!3 = metadata !{metadata !4}
!4 = metadata !{i32 786451, metadata !5, null, metadata !"C", i32 5, i64 32, i64 32, i32 0, i32 0, null, metadata !6, i32 0, null, null, metadata !"_ZTS1C"} ; [ DW_TAG_structure_type ] [C] [line 5, size 32, align 32, offset 0] [def] [from ]
!5 = metadata !{metadata !"recursive_inlining.cpp", metadata !"/usr/local/google/home/blaikie/dev/scratch/missing_concrete_variable_on_darwin/reduce"}
!6 = metadata !{metadata !7, metadata !9}
!7 = metadata !{i32 786445, metadata !5, metadata !"_ZTS1C", metadata !"b", i32 6, i64 32, i64 32, i64 0, i32 0, metadata !8} ; [ DW_TAG_member ] [b] [line 6, size 32, align 32, offset 0] [from int]
!8 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!9 = metadata !{i32 786478, metadata !5, metadata !"_ZTS1C", metadata !"m_fn2", metadata !"m_fn2", metadata !"_ZN1C5m_fn2Ev", i32 7, metadata !10, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 true, null, null, i32 0, null, i32 7} ; [ DW_TAG_subprogram ] [line 7] [m_fn2]
!10 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !11, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!11 = metadata !{null, metadata !12}
!12 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 1088, metadata !"_ZTS1C"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from _ZTS1C]
!13 = metadata !{metadata !14, metadata !18, metadata !19, metadata !20, metadata !21, metadata !22}
!14 = metadata !{i32 786478, metadata !5, metadata !15, metadata !"fn6", metadata !"fn6", metadata !"_Z3fn6v", i32 15, metadata !16, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void ()* @_Z3fn6v, null, null, metadata !2, i32 15} ; [ DW_TAG_subprogram ] [line 15] [def] [fn6]
!15 = metadata !{i32 786473, metadata !5}         ; [ DW_TAG_file_type ] [/usr/local/google/home/blaikie/dev/scratch/missing_concrete_variable_on_darwin/reduce/recursive_inlining.cpp]
!16 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !17, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!17 = metadata !{null}
!18 = metadata !{i32 786478, metadata !5, metadata !15, metadata !"fn3", metadata !"fn3", metadata !"_Z3fn3v", i32 20, metadata !16, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void ()* @_Z3fn3v, null, null, metadata !2, i32 20} ; [ DW_TAG_subprogram ] [line 20] [def] [fn3]
!19 = metadata !{i32 786478, metadata !5, metadata !15, metadata !"fn4", metadata !"fn4", metadata !"_Z3fn4v", i32 21, metadata !16, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void ()* @_Z3fn4v, null, null, metadata !2, i32 21} ; [ DW_TAG_subprogram ] [line 21] [def] [fn4]
!20 = metadata !{i32 786478, metadata !5, metadata !15, metadata !"fn5", metadata !"fn5", metadata !"_Z3fn5v", i32 22, metadata !16, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void ()* @_Z3fn5v, null, null, metadata !2, i32 22} ; [ DW_TAG_subprogram ] [line 22] [def] [fn5]
!21 = metadata !{i32 786478, metadata !5, metadata !15, metadata !"fn7", metadata !"fn7", metadata !"_Z3fn7v", i32 14, metadata !16, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, null, null, null, metadata !2, i32 14} ; [ DW_TAG_subprogram ] [line 14] [def] [fn7]
!22 = metadata !{i32 786478, metadata !5, metadata !"_ZTS1C", metadata !"m_fn2", metadata !"m_fn2", metadata !"_ZN1C5m_fn2Ev", i32 7, metadata !10, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (%struct.C*)* @_ZN1C5m_fn2Ev, null, metadata !9, metadata !23, i32 7} ; [ DW_TAG_subprogram ] [line 7] [def] [m_fn2]
!23 = metadata !{metadata !24}
!24 = metadata !{i32 786689, metadata !22, metadata !"this", null, i32 16777216, metadata !25, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!25 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !"_ZTS1C"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS1C]
!26 = metadata !{metadata !27}
!27 = metadata !{i32 786484, i32 0, null, metadata !"x", metadata !"x", metadata !"", metadata !15, i32 13, metadata !25, i32 0, i32 1, %struct.C** @x, null} ; [ DW_TAG_variable ] [x] [line 13] [def]
!28 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!29 = metadata !{i32 2, metadata !"Debug Info Version", i32 1}
!30 = metadata !{metadata !"clang version 3.6.0 "}
!31 = metadata !{i32 16, i32 0, metadata !14, null}
!32 = metadata !{i32 17, i32 0, metadata !14, null}
!33 = metadata !{metadata !34, metadata !34, i64 0}
!34 = metadata !{metadata !"any pointer", metadata !35, i64 0}
!35 = metadata !{metadata !"omnipotent char", metadata !36, i64 0}
!36 = metadata !{metadata !"Simple C/C++ TBAA"}
!37 = metadata !{i32 786689, metadata !22, metadata !"this", null, i32 16777216, metadata !25, i32 1088, metadata !32} ; [ DW_TAG_arg_variable ] [this] [line 0]
!38 = metadata !{i32 0, i32 0, metadata !22, metadata !32}
!39 = metadata !{i32 8, i32 0, metadata !22, metadata !32} ; [ DW_TAG_imported_declaration ]
!40 = metadata !{i32 9, i32 0, metadata !41, metadata !32}
!41 = metadata !{i32 786443, metadata !5, metadata !22, i32 9, i32 0, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [/usr/local/google/home/blaikie/dev/scratch/missing_concrete_variable_on_darwin/reduce/recursive_inlining.cpp]
!42 = metadata !{metadata !43, metadata !44, i64 0}
!43 = metadata !{metadata !"_ZTS1C", metadata !44, i64 0}
!44 = metadata !{metadata !"int", metadata !35, i64 0}
!45 = metadata !{i32 9, i32 0, metadata !46, metadata !32}
!46 = metadata !{i32 786443, metadata !5, metadata !41, i32 9, i32 0, i32 1, i32 1} ; [ DW_TAG_lexical_block ] [/usr/local/google/home/blaikie/dev/scratch/missing_concrete_variable_on_darwin/reduce/recursive_inlining.cpp]
!47 = metadata !{i32 10, i32 0, metadata !22, metadata !32}
!48 = metadata !{i32 19, i32 0, metadata !14, null}
!49 = metadata !{i32 0, i32 0, metadata !22, null}
!50 = metadata !{i32 8, i32 0, metadata !22, null} ; [ DW_TAG_imported_declaration ]
!51 = metadata !{i32 9, i32 0, metadata !41, null}
!52 = metadata !{i32 9, i32 0, metadata !46, null}
!53 = metadata !{i32 16, i32 0, metadata !14, metadata !54}
!54 = metadata !{i32 20, i32 0, metadata !18, metadata !55}
!55 = metadata !{i32 10, i32 0, metadata !22, null}
!56 = metadata !{i32 17, i32 0, metadata !14, metadata !54}
!57 = metadata !{i32 786689, metadata !22, metadata !"this", null, i32 16777216, metadata !25, i32 1088, metadata !56} ; [ DW_TAG_arg_variable ] [this] [line 0]
!58 = metadata !{i32 0, i32 0, metadata !22, metadata !56}
!59 = metadata !{i32 8, i32 0, metadata !22, metadata !56} ; [ DW_TAG_imported_declaration ]
!60 = metadata !{i32 9, i32 0, metadata !41, metadata !56}
!61 = metadata !{i32 9, i32 0, metadata !46, metadata !56}
!62 = metadata !{i32 10, i32 0, metadata !22, metadata !56}
!63 = metadata !{i32 11, i32 0, metadata !22, null}
!64 = metadata !{i32 16, i32 0, metadata !14, metadata !65}
!65 = metadata !{i32 20, i32 0, metadata !18, null}
!66 = metadata !{i32 17, i32 0, metadata !14, metadata !65}
!67 = metadata !{i32 786689, metadata !22, metadata !"this", null, i32 16777216, metadata !25, i32 1088, metadata !66} ; [ DW_TAG_arg_variable ] [this] [line 0]
!68 = metadata !{i32 0, i32 0, metadata !22, metadata !66}
!69 = metadata !{i32 8, i32 0, metadata !22, metadata !66} ; [ DW_TAG_imported_declaration ]
!70 = metadata !{i32 9, i32 0, metadata !41, metadata !66}
!71 = metadata !{i32 9, i32 0, metadata !46, metadata !66}
!72 = metadata !{i32 21, i32 0, metadata !19, null}
!73 = metadata !{i32 22, i32 0, metadata !20, null}
