; Test to check the callgraph in summary when there is PGO
; RUN: opt -module-summary %s -o %t.o
; RUN: llvm-bcanalyzer -dump %t.o | FileCheck %s
; RUN: opt -module-summary %p/Inputs/thinlto-function-summary-callgraph-profile-summary.ll -o %t2.o
; RUN: llvm-lto -thinlto -o %t3 %t.o %t2.o
; RUN: llvm-bcanalyzer -dump %t3.thinlto.bc | FileCheck %s --check-prefix=COMBINED


; CHECK-LABEL:       <GLOBALVAL_SUMMARY_BLOCK
; CHECK-NEXT:    <VERSION
; See if the call to func is registered, using the expected callsite count
; and profile count, with value id matching the subsequent value symbol table.
; CHECK-NEXT:    <PERMODULE_PROFILE {{.*}} op4=[[HOT1:.*]] op5=3 op6=[[COLD:.*]] op7=1 op8=[[HOT2:.*]] op9=3 op10=[[NONE1:.*]] op11=2 op12=[[HOT3:.*]] op13=3 op14=[[NONE2:.*]] op15=2 op16=[[NONE3:.*]] op17=2/>
; CHECK-NEXT:  </GLOBALVAL_SUMMARY_BLOCK>
; CHECK-LABEL:  <VALUE_SYMTAB
; CHECK-NEXT:       <FNENTRY {{.*}} record string = 'hot_function
; CHECK-DAG:        <ENTRY abbrevid=6 op0=[[NONE1]] {{.*}} record string = 'none1'
; CHECK-DAG:        <ENTRY abbrevid=6 op0=[[COLD]] {{.*}} record string = 'cold'
; CHECK-DAG:        <ENTRY abbrevid=6 op0=[[NONE2]] {{.*}} record string = 'none2'
; CHECK-DAG:        <ENTRY abbrevid=6 op0=[[NONE3]] {{.*}} record string = 'none3'
; CHECK-DAG:        <ENTRY abbrevid=6 op0=[[HOT1]] {{.*}} record string = 'hot1'
; CHECK-DAG:        <ENTRY abbrevid=6 op0=[[HOT2]] {{.*}} record string = 'hot2'
; CHECK-DAG:        <ENTRY abbrevid=6 op0=[[HOT3]] {{.*}} record string = 'hot3'
; CHECK-LABEL:  </VALUE_SYMTAB>

; COMBINED:       <GLOBALVAL_SUMMARY_BLOCK
; COMBINED-NEXT:    <VERSION
; COMBINED-NEXT:    <COMBINED abbrevid=
; COMBINED-NEXT:    <COMBINED abbrevid=
; COMBINED-NEXT:    <COMBINED abbrevid=
; COMBINED-NEXT:    <COMBINED abbrevid=
; COMBINED-NEXT:    <COMBINED abbrevid=
; COMBINED-NEXT:    <COMBINED abbrevid=
; COMBINED-NEXT:    <COMBINED_PROFILE {{.*}} op5=[[HOT1:.*]] op6=3 op7=[[COLD:.*]] op8=1 op9=[[HOT2:.*]] op10=3 op11=[[NONE1:.*]] op12=2 op13=[[HOT3:.*]] op14=3 op15=[[NONE2:.*]] op16=2 op17=[[NONE3:.*]] op18=2/>
; COMBINED_NEXT:    <COMBINED abbrevid=
; COMBINED_NEXT:  </GLOBALVAL_SUMMARY_BLOCK>


; ModuleID = 'thinlto-function-summary-callgraph.ll'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; This function have high profile count, so entry block is hot.
define void @hot_function(i1 %a, i1 %a2) !prof !20 {
entry:
    call void @hot1()
    br i1 %a, label %Cold, label %Hot, !prof !41
Cold:           ; 1/1000 goes here
  call void @cold()
  call void @hot2()
  call void @none1()
  br label %exit
Hot:            ; 999/1000 goes here
  call void @hot2()
  call void @hot3()
  br i1 %a2, label %None1, label %None2, !prof !42
None1:          ; half goes here
  call void @none1()
  call void @none2()
  br label %exit
None2:          ; half goes here
  call void @none3()
  br label %exit
exit:
  ret void
}

declare void @hot1() #1
declare void @hot2() #1
declare void @hot3() #1
declare void @cold() #1
declare void @none1() #1
declare void @none2() #1
declare void @none3() #1


!41 = !{!"branch_weights", i32 1, i32 1000}
!42 = !{!"branch_weights", i32 1, i32 1}



!llvm.module.flags = !{!1}
!20 = !{!"function_entry_count", i64 110}

!1 = !{i32 1, !"ProfileSummary", !2}
!2 = !{!3, !4, !5, !6, !7, !8, !9, !10}
!3 = !{!"ProfileFormat", !"InstrProf"}
!4 = !{!"TotalCount", i64 10000}
!5 = !{!"MaxCount", i64 10}
!6 = !{!"MaxInternalCount", i64 1}
!7 = !{!"MaxFunctionCount", i64 1000}
!8 = !{!"NumCounts", i64 3}
!9 = !{!"NumFunctions", i64 3}
!10 = !{!"DetailedSummary", !11}
!11 = !{!12, !13, !14}
!12 = !{i32 10000, i64 100, i32 1}
!13 = !{i32 999000, i64 100, i32 1}
!14 = !{i32 999999, i64 1, i32 2}
