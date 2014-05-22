; We specify -mcpu explicitly to avoid instruction reordering that happens on
; some setups (e.g., Atom) from affecting the output.
; RUN: llc < %s -mcpu=core2 -mtriple=i686-pc-win32 | FileCheck %s -check-prefix=WIN32
; RUN: llc < %s -mcpu=core2 -mtriple=i686-pc-mingw32 | FileCheck %s -check-prefix=MINGW_X86
; RUN: llc < %s -mcpu=core2 -mtriple=i386-pc-linux | FileCheck %s -check-prefix=LINUX
; RUN: llc < %s -mcpu=core2 -O0 -mtriple=i686-pc-win32 | FileCheck %s -check-prefix=WIN32
; RUN: llc < %s -mcpu=core2 -O0 -mtriple=i686-pc-mingw32 | FileCheck %s -check-prefix=MINGW_X86
; RUN: llc < %s -mcpu=core2 -O0 -mtriple=i386-pc-linux | FileCheck %s -check-prefix=LINUX

; The SysV ABI used by most Unixes and Mingw on x86 specifies that an sret pointer
; is callee-cleanup. However, in MSVC's cdecl calling convention, sret pointer
; arguments are caller-cleanup like normal arguments.

define void @sret1(i8* sret %x) nounwind {
entry:
; WIN32-LABEL:      _sret1:
; WIN32:      movb $42, (%eax)
; WIN32-NOT:  popl %eax
; WIN32:    {{retl$}}

; MINGW_X86-LABEL:  _sret1:
; MINGW_X86:  {{retl$}}

; LINUX-LABEL:      sret1:
; LINUX:      retl $4

  store i8 42, i8* %x, align 4
  ret void
}

define void @sret2(i8* sret %x, i8 %y) nounwind {
entry:
; WIN32-LABEL:      _sret2:
; WIN32:      movb {{.*}}, (%eax)
; WIN32-NOT:  popl %eax
; WIN32:    {{retl$}}

; MINGW_X86-LABEL:  _sret2:
; MINGW_X86:  {{retl$}}

; LINUX-LABEL:      sret2:
; LINUX:      retl $4

  store i8 %y, i8* %x
  ret void
}

define void @sret3(i8* sret %x, i8* %y) nounwind {
entry:
; WIN32-LABEL:      _sret3:
; WIN32:      movb $42, (%eax)
; WIN32-NOT:  movb $13, (%eax)
; WIN32-NOT:  popl %eax
; WIN32:    {{retl$}}

; MINGW_X86-LABEL:  _sret3:
; MINGW_X86:  {{retl$}}

; LINUX-LABEL:      sret3:
; LINUX:      retl $4

  store i8 42, i8* %x
  store i8 13, i8* %y
  ret void
}

; PR15556
%struct.S4 = type { i32, i32, i32 }

define void @sret4(%struct.S4* noalias sret %agg.result) {
entry:
; WIN32-LABEL:     _sret4:
; WIN32:     movl $42, (%eax)
; WIN32-NOT: popl %eax
; WIN32:   {{retl$}}

; MINGW_X86-LABEL: _sret4:
; MINGW_X86: {{retl$}}

; LINUX-LABEL:     sret4:
; LINUX:     retl $4

  %x = getelementptr inbounds %struct.S4* %agg.result, i32 0, i32 0
  store i32 42, i32* %x, align 4
  ret void
}

%struct.S5 = type { i32 }
%class.C5 = type { i8 }

define x86_thiscallcc void @"\01?foo@C5@@QAE?AUS5@@XZ"(%struct.S5* noalias sret %agg.result, %class.C5* %this) {
entry:
  %this.addr = alloca %class.C5*, align 4
  store %class.C5* %this, %class.C5** %this.addr, align 4
  %this1 = load %class.C5** %this.addr
  %x = getelementptr inbounds %struct.S5* %agg.result, i32 0, i32 0
  store i32 42, i32* %x, align 4
  ret void
; WIN32-LABEL:     {{^}}"?foo@C5@@QAE?AUS5@@XZ":
; MINGW_X86-LABEL: {{^}}"?foo@C5@@QAE?AUS5@@XZ":
; LINUX-LABEL:     {{^}}"?foo@C5@@QAE?AUS5@@XZ":

; The address of the return structure is passed as an implicit parameter.
; In the -O0 build, %eax is spilled at the beginning of the function, hence we
; should match both 4(%esp) and 8(%esp).
; WIN32:     {{[48]}}(%esp), %eax
; WIN32:     movl $42, (%eax)
; WIN32:     retl $4
}

define void @call_foo5() {
entry:
  %c = alloca %class.C5, align 1
  %s = alloca %struct.S5, align 4
  call x86_thiscallcc void @"\01?foo@C5@@QAE?AUS5@@XZ"(%struct.S5* sret %s, %class.C5* %c)
; WIN32-LABEL:      {{^}}_call_foo5:
; MINGW_X86-LABEL:  {{^}}_call_foo5:
; LINUX-LABEL:      {{^}}call_foo5:


; Load the address of the result and put it onto stack
; (through %ecx in the -O0 build).
; WIN32:      leal {{[0-9]+}}(%esp), %e{{[a-d]}}x
; WIN32:      movl %e{{[a-d]}}x, (%e{{([a-d]x)|(sp)}})

; The this pointer goes to ECX.
; WIN32-NEXT: leal {{[0-9]+}}(%esp), %ecx
; WIN32-NEXT: calll "?foo@C5@@QAE?AUS5@@XZ"
; WIN32:      retl
  ret void
}


%struct.test6 = type { i32, i32, i32 }
define void @test6_f(%struct.test6* %x) nounwind {
; WIN32-LABEL: _test6_f:
; MINGW_X86-LABEL: _test6_f:
; LINUX-LABEL: test6_f:

; The %x argument is moved to %ecx. It will be the this pointer.
; WIN32: movl    8(%ebp), %ecx

; The %x argument is moved to (%esp). It will be the this pointer. With -O0
; we copy esp to ecx and use (ecx) instead of (esp).
; MINGW_X86: movl    8(%ebp), %eax
; MINGW_X86: movl    %eax, (%e{{([a-d]x)|(sp)}})

; The sret pointer is (%esp)
; WIN32:          leal    8(%esp), %[[REG:e[a-d]x]]
; WIN32-NEXT:     movl    %[[REG]], (%e{{([a-d]x)|(sp)}})

; The sret pointer is %ecx
; MINGW_X86-NEXT: leal    8(%esp), %ecx
; MINGW_X86-NEXT: calll   _test6_g

  %tmp = alloca %struct.test6, align 4
  call x86_thiscallcc void @test6_g(%struct.test6* sret %tmp, %struct.test6* %x)
  ret void
}
declare x86_thiscallcc void @test6_g(%struct.test6* sret, %struct.test6*)
