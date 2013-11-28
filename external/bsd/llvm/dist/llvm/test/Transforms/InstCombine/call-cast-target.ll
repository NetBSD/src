; RUN: opt < %s -instcombine -S | FileCheck %s

target datalayout = "e-p:32:32"
target triple = "i686-pc-linux-gnu"

define i32 @main() {
; CHECK-LABEL: @main
; CHECK: call i32 bitcast
entry:
  %tmp = call i32 bitcast (i8* (i32*)* @ctime to i32 (i32*)*)( i32* null )          ; <i32> [#uses=1]
  ret i32 %tmp
}

declare i8* @ctime(i32*)

