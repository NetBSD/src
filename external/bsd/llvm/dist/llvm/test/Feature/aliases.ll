; RUN: llvm-as < %s | llvm-dis > %t1.ll
; RUN: llvm-as %t1.ll -o - | llvm-dis > %t2.ll
; RUN: diff %t1.ll %t2.ll

@llvm.used = appending global [1 x i8*] [i8* bitcast (i32* @foo1 to i8*)], section "llvm.metadata"

@bar = global i32 0
@foo1 = alias i32* @bar
@foo2 = alias i32* @bar

%FunTy = type i32()

define i32 @foo_f() {
  ret i32 0
}
@bar_f = alias weak_odr %FunTy* @foo_f

@bar_i = alias internal i32* @bar

@A = alias i64, i32* @bar

define i32 @test() {
entry:
   %tmp = load i32* @foo1
   %tmp1 = load i32* @foo2
   %tmp0 = load i32* @bar_i
   %tmp2 = call i32 @foo_f()
   %tmp3 = add i32 %tmp, %tmp2
   %tmp4 = call %FunTy* @bar_f()
   %tmp5 = add i32 %tmp3, %tmp4
   %tmp6 = add i32 %tmp1, %tmp5
   %tmp7 = add i32 %tmp6, %tmp0
   ret i32 %tmp7
}
