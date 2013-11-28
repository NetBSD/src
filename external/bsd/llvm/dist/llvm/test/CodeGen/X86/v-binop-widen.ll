; RUN: llc -mcpu=generic -march=x86 -mattr=+sse < %s | FileCheck %s
; CHECK: divps
; CHECK: divps
; CHECK: divss

%vec = type <9 x float>
define %vec @vecdiv( %vec %p1, %vec %p2)
{
  %result = fdiv %vec %p1, %p2
  ret %vec %result
}
