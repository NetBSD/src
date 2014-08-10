; REQUIRES: object-emission

; RUN: llc -mtriple=i386-linux-gnu -filetype=obj -relocation-model=pic %s -o /dev/null

; Derived from the test case in PR20367, there's nothing really positive to
; test here (hence no FileCheck, etc). All that was wrong is that the debug info
; intrinsics (introduced by inlining) in 'f1' were causing codegen to crash, but
; since 'f1' is a nodebug function, there's no positive outcome to confirm, just
; that debug info doesn't get in the way/cause a crash.

; The test case isn't particularly well reduced/tidy, but as simple as I could
; get the C++ source. I assume the complexity is mostly just about producing a
; certain amount of register pressure, so it might be able to be simplified/made
; more uniform.

; Generated from:
; $ clang-tot -cc1 -triple i386 -emit-obj -g -O3 repro.cpp
; void sink(const void *);
; int source();
; void f3(int);
; 
; extern bool b;
; 
; struct string {
;   unsigned *mem;
; };
; 
; extern string &str;
; 
; inline __attribute__((always_inline)) void s2(string *lhs) { sink(lhs->mem); }
; inline __attribute__((always_inline)) void f() {
;   string str2;
;   s2(&str2);
;   sink(&str2);
; }
; void __attribute__((nodebug)) f1() {
;   for (int iter = 0; iter != 2; ++iter) {
;     f();
;     sink(str.mem);
;     if (b) return;
;   }
; }

%struct.string = type { i32* }

@str = external constant %struct.string*
@b = external global i8

; Function Attrs: nounwind
define void @_Z2f1v() #0 {
entry:
  %str2.i = alloca %struct.string, align 4
  %0 = bitcast %struct.string* %str2.i to i8*, !dbg !26
  %1 = load %struct.string** @str, align 4
  %mem = getelementptr inbounds %struct.string* %1, i32 0, i32 0
  br label %for.body

for.body:                                         ; preds = %for.body, %entry
  %iter.02 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  call void @llvm.lifetime.start(i64 4, i8* %0), !dbg !26
  call void @llvm.dbg.value(metadata !{%struct.string* %str2.i}, i64 0, metadata !16) #3, !dbg !26
  call void @llvm.dbg.value(metadata !{%struct.string* %str2.i}, i64 0, metadata !27) #3, !dbg !29
  call void @_Z4sinkPKv(i8* undef) #3, !dbg !29
  call void @_Z4sinkPKv(i8* %0) #3, !dbg !30
  call void @llvm.lifetime.end(i64 4, i8* %0), !dbg !31
  %2 = load i32** %mem, align 4, !tbaa !32
  %3 = bitcast i32* %2 to i8*
  call void @_Z4sinkPKv(i8* %3) #3
  %4 = load i8* @b, align 1, !tbaa !37, !range !39
  %tobool = icmp ne i8 %4, 0
  %inc = add nsw i32 %iter.02, 1
  %cmp = icmp eq i32 %inc, 2
  %or.cond = or i1 %tobool, %cmp
  br i1 %or.cond, label %for.end, label %for.body

for.end:                                          ; preds = %for.body
  ret void
}

declare void @_Z4sinkPKv(i8*) #1

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata) #2

; Function Attrs: nounwind
declare void @llvm.lifetime.start(i64, i8* nocapture) #3

; Function Attrs: nounwind
declare void @llvm.lifetime.end(i64, i8* nocapture) #3

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!23, !24}
!llvm.ident = !{!25}

!0 = metadata !{i32 786449, metadata !1, i32 4, metadata !"clang version 3.5.0 ", i1 true, metadata !"", i32 0, metadata !2, metadata !3, metadata !10, metadata !2, metadata !2, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/tmp/dbginfo/<stdin>] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !"<stdin>", metadata !"/tmp/dbginfo"}
!2 = metadata !{}
!3 = metadata !{metadata !4}
!4 = metadata !{i32 786451, metadata !5, null, metadata !"string", i32 7, i64 32, i64 32, i32 0, i32 0, null, metadata !6, i32 0, null, null, metadata !"_ZTS6string"} ; [ DW_TAG_structure_type ] [string] [line 7, size 32, align 32, offset 0] [def] [from ]
!5 = metadata !{metadata !"repro.cpp", metadata !"/tmp/dbginfo"}
!6 = metadata !{metadata !7}
!7 = metadata !{i32 786445, metadata !5, metadata !"_ZTS6string", metadata !"mem", i32 8, i64 32, i64 32, i64 0, i32 0, metadata !8} ; [ DW_TAG_member ] [mem] [line 8, size 32, align 32, offset 0] [from ]
!8 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 32, i64 32, i64 0, i32 0, metadata !9} ; [ DW_TAG_pointer_type ] [line 0, size 32, align 32, offset 0] [from unsigned int]
!9 = metadata !{i32 786468, null, null, metadata !"unsigned int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 7} ; [ DW_TAG_base_type ] [unsigned int] [line 0, size 32, align 32, offset 0, enc DW_ATE_unsigned]
!10 = metadata !{metadata !11, metadata !17}
!11 = metadata !{i32 786478, metadata !5, metadata !12, metadata !"f", metadata !"f", metadata !"_Z1fv", i32 14, metadata !13, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, null, null, null, metadata !15, i32 14} ; [ DW_TAG_subprogram ] [line 14] [def] [f]
!12 = metadata !{i32 786473, metadata !5}         ; [ DW_TAG_file_type ] [/tmp/dbginfo/repro.cpp]
!13 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !14, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!14 = metadata !{null}
!15 = metadata !{metadata !16}
!16 = metadata !{i32 786688, metadata !11, metadata !"str2", metadata !12, i32 15, metadata !"_ZTS6string", i32 0, i32 0} ; [ DW_TAG_auto_variable ] [str2] [line 15]
!17 = metadata !{i32 786478, metadata !5, metadata !12, metadata !"s2", metadata !"s2", metadata !"_Z2s2P6string", i32 13, metadata !18, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, null, null, null, metadata !21, i32 13} ; [ DW_TAG_subprogram ] [line 13] [def] [s2]
!18 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !19, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!19 = metadata !{null, metadata !20}
!20 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 32, i64 32, i64 0, i32 0, metadata !"_ZTS6string"} ; [ DW_TAG_pointer_type ] [line 0, size 32, align 32, offset 0] [from _ZTS6string]
!21 = metadata !{metadata !22}
!22 = metadata !{i32 786689, metadata !17, metadata !"lhs", metadata !12, i32 16777229, metadata !20, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [lhs] [line 13]
!23 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!24 = metadata !{i32 2, metadata !"Debug Info Version", i32 1}
!25 = metadata !{metadata !"clang version 3.5.0 "}
!26 = metadata !{i32 15, i32 0, metadata !11, null}
!27 = metadata !{i32 786689, metadata !17, metadata !"lhs", metadata !12, i32 16777229, metadata !20, i32 0, metadata !28} ; [ DW_TAG_arg_variable ] [lhs] [line 13]
!28 = metadata !{i32 16, i32 0, metadata !11, null}
!29 = metadata !{i32 13, i32 0, metadata !17, metadata !28}
!30 = metadata !{i32 17, i32 0, metadata !11, null}
!31 = metadata !{i32 18, i32 0, metadata !11, null}
!32 = metadata !{metadata !33, metadata !34, i64 0}
!33 = metadata !{metadata !"_ZTS6string", metadata !34, i64 0}
!34 = metadata !{metadata !"any pointer", metadata !35, i64 0}
!35 = metadata !{metadata !"omnipotent char", metadata !36, i64 0}
!36 = metadata !{metadata !"Simple C/C++ TBAA"}
!37 = metadata !{metadata !38, metadata !38, i64 0}
!38 = metadata !{metadata !"bool", metadata !35, i64 0}
!39 = metadata !{i8 0, i8 2}
