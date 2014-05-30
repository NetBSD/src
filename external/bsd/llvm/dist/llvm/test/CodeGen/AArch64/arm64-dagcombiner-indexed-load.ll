; RUN: llc -O3 < %s | FileCheck %s
; RUN: llc -O3 -addr-sink-using-gep=1 < %s | FileCheck %s
; Test case for a DAG combiner bug where we combined an indexed load
; with an extension (sext, zext, or any) into a regular extended load,
; i.e., dropping the indexed value.
; <rdar://problem/16389332>

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-ios"

%class.A = type { i64, i64 }
%class.C = type { i64 }

; CHECK-LABEL: XX:
; CHECK: ldr
define i32 @XX(%class.A* %K, i1 %tst, i32* %addr, %class.C** %ppC, %class.C* %pC) {
entry:
  br i1 %tst, label %if.then, label %lor.rhs.i

lor.rhs.i:                                        ; preds = %entry
  %tmp = load i32* %addr, align 4
  %y.i.i.i = getelementptr inbounds %class.A* %K, i64 0, i32 1
  %tmp1 = load i64* %y.i.i.i, align 8
  %U.sroa.3.8.extract.trunc.i = trunc i64 %tmp1 to i32
  %div11.i = sdiv i32 %U.sroa.3.8.extract.trunc.i, 17
  %add12.i = add nsw i32 0, %div11.i
  %U.sroa.3.12.extract.shift.i = lshr i64 %tmp1, 32
  %U.sroa.3.12.extract.trunc.i = trunc i64 %U.sroa.3.12.extract.shift.i to i32
  %div15.i = sdiv i32 %U.sroa.3.12.extract.trunc.i, 13
  %add16.i = add nsw i32 %add12.i, %div15.i
  %rem.i.i = srem i32 %add16.i, %tmp
  %idxprom = sext i32 %rem.i.i to i64
  %arrayidx = getelementptr inbounds %class.C** %ppC, i64 %idxprom
  %tobool533 = icmp eq %class.C* %pC, null
  br i1 %tobool533, label %while.end, label %while.body

if.then:                                          ; preds = %entry
  ret i32 42

while.body:                                       ; preds = %lor.rhs.i
  ret i32 5

while.end:                                        ; preds = %lor.rhs.i
  %tmp3 = load %class.C** %arrayidx, align 8
  ret i32 50
}
