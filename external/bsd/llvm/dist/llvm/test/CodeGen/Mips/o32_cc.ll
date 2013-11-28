; RUN: llc -march=mipsel < %s | FileCheck %s
; RUN: llc -march=mipsel -mattr=+fp64 < %s | FileCheck %s
; RUN: llc -march=mipsel < %s | FileCheck -check-prefix=FP32EL %s
; RUN: llc -march=mipsel -mattr=+fp64 < %s | FileCheck -check-prefix=FP64EL %s

; $f12, $f14
; CHECK-LABEL: testlowercall0:
; CHECK-DAG: ldc1 $f12, %lo
; CHECK-DAG: ldc1 $f14, %lo
define void @testlowercall0() nounwind {
entry:
  tail call void @f0(double 5.000000e+00, double 6.000000e+00) nounwind
  ret void
}

declare void @f0(double, double)

; $f12, $f14
; CHECK-LABEL: testlowercall1:
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: lwc1 $f14, %lo
define void @testlowercall1() nounwind {
entry:
  tail call void @f1(float 8.000000e+00, float 9.000000e+00) nounwind
  ret void
}

declare void @f1(float, float)

; $f12, $f14
; CHECK-LABEL: testlowercall2:
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: ldc1 $f14, %lo
define void @testlowercall2() nounwind {
entry:
  tail call void @f2(float 8.000000e+00, double 6.000000e+00) nounwind
  ret void
}

declare void @f2(float, double)

; $f12, $f14
; CHECK-LABEL: testlowercall3:
; CHECK-DAG: ldc1 $f12, %lo
; CHECK-DAG: lwc1 $f14, %lo
define void @testlowercall3() nounwind {
entry:
  tail call void @f3(double 5.000000e+00, float 9.000000e+00) nounwind
  ret void
}

declare void @f3(double, float)

; $4, $5, $6, $7
; CHECK-LABEL: testlowercall4:
; CHECK-DAG: addiu $4, $zero, 12
; CHECK-DAG: addiu $5, $zero, 13
; CHECK-DAG: addiu $6, $zero, 14
; CHECK-DAG: addiu $7, $zero, 15
define void @testlowercall4() nounwind {
entry:
  tail call void @f4(i32 12, i32 13, i32 14, i32 15) nounwind
  ret void
}

declare void @f4(i32, i32, i32, i32)

; $f12, $6, stack
; CHECK-LABEL: testlowercall5:
; CHECK-DAG: ldc1 $f12, %lo
; CHECK-DAG: addiu $6, $zero, 23
; CHECK-DAG: sw ${{[a-z0-9]+}}, 16($sp)
; CHECK-DAG: sw ${{[a-z0-9]+}}, 20($sp)
define void @testlowercall5() nounwind {
entry:
  tail call void @f5(double 1.500000e+01, i32 23, double 1.700000e+01) nounwind
  ret void
}

declare void @f5(double, i32, double)

; $f12, $6, $7
; CHECK-LABEL: testlowercall6:
; CHECK-DAG: ldc1 $f12, %lo
; CHECK-DAG: addiu $6, $zero, 33
; CHECK-DAG: addiu $7, $zero, 24
define void @testlowercall6() nounwind {
entry:
  tail call void @f6(double 2.500000e+01, i32 33, i32 24) nounwind
  ret void
}

declare void @f6(double, i32, i32)

; $f12, $5, $6
; CHECK-LABEL: testlowercall7:
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: addiu $5, $zero, 43
; CHECK-DAG: addiu $6, $zero, 34
define void @testlowercall7() nounwind {
entry:
  tail call void @f7(float 1.800000e+01, i32 43, i32 34) nounwind
  ret void
}

declare void @f7(float, i32, i32)

; $4, $5, $6, stack
; CHECK-LABEL: testlowercall8:
; CHECK-DAG: addiu $4, $zero, 22
; CHECK-DAG: addiu $5, $zero, 53
; CHECK-DAG: addiu $6, $zero, 44
; CHECK-DAG: sw ${{[a-z0-9]+}}, 16($sp)
; CHECK-DAG: sw ${{[a-z0-9]+}}, 20($sp)
define void @testlowercall8() nounwind {
entry:
  tail call void @f8(i32 22, i32 53, i32 44, double 4.000000e+00) nounwind
  ret void
}

declare void @f8(i32, i32, i32, double)

; $4, $5, $6, $7
; CHECK-LABEL: testlowercall9:
; CHECK-DAG: addiu $4, $zero, 32
; CHECK-DAG: addiu $5, $zero, 63
; CHECK-DAG: addiu $6, $zero, 54
; CHECK-DAG: lui $7, 16688
define void @testlowercall9() nounwind {
entry:
  tail call void @f9(i32 32, i32 63, i32 54, float 1.100000e+01) nounwind
  ret void
}

declare void @f9(i32, i32, i32, float)

; $4, $5, ($6, $7)
; CHECK-LABEL: testlowercall10:
; CHECK-DAG: addiu $4, $zero, 42
; CHECK-DAG: addiu $5, $zero, 73
; FP32EL-LABEL: testlowercall10:
; FP32EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP32EL-DAG: mfc1 $7, $f{{[0-9]+}}
; FP64EL-LABEL: testlowercall10:
; FP64EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP64EL-DAG: mfhc1 $7, $f{{[0-9]+}}
define void @testlowercall10() nounwind {
entry:
  tail call void @f10(i32 42, i32 73, double 2.700000e+01) nounwind
  ret void
}

declare void @f10(i32, i32, double)

; $4, ($6, $7)
; CHECK-LABEL: testlowercall11:
; CHECK-DAG: addiu $4, $zero, 52
; FP32EL-LABEL: testlowercall11:
; FP32EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP32EL-DAG: mfc1 $7, $f{{[0-9]+}}
; FP64EL-LABEL: testlowercall11:
; FP64EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP64EL-DAG: mfhc1 $7, $f{{[0-9]+}}
define void @testlowercall11() nounwind {
entry:
  tail call void @f11(i32 52, double 1.600000e+01) nounwind
  ret void
}

declare void @f11(i32, double)

; $f12, $f14, $6, $7
; CHECK-LABEL: testlowercall12:
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: lwc1 $f14, %lo
; CHECK-DAG: lui $6, 16672
; CHECK-DAG: lui $7, 16808
define void @testlowercall12() nounwind {
entry:
  tail call void @f12(float 2.800000e+01, float 1.900000e+01, float 1.000000e+01, float 2.100000e+01) nounwind
  ret void
}

declare void @f12(float, float, float, float)

; $f12, $5, $6, $7
; CHECK-LABEL: testlowercall13:
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: addiu $5, $zero, 83
; CHECK-DAG: lui $6, 16800
; CHECK-DAG: addiu $7, $zero, 25
define void @testlowercall13() nounwind {
entry:
  tail call void @f13(float 3.800000e+01, i32 83, float 2.000000e+01, i32 25) nounwind
  ret void
}


declare void @f13(float, i32, float, i32)

; $f12, $f14, $7
; CHECK-LABEL: testlowercall14:
; CHECK-DAG: ldc1 $f12, %lo
; CHECK-DAG: lwc1 $f14, %lo
; CHECK-DAG: lui $7, 16880
define void @testlowercall14() nounwind {
entry:
  tail call void @f14(double 3.500000e+01, float 2.900000e+01, float 3.000000e+01) nounwind
  ret void
}

declare void @f14(double, float, float)

; $f12, $f14, ($6, $7)
; CHECK-LABEL: testlowercall15:
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: lwc1 $f14, %lo
; FP32EL-LABEL: testlowercall15:
; FP32EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP32EL-DAG: mfc1 $7, $f{{[0-9]+}}
; FP64EL-LABEL: testlowercall15:
; FP64EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP64EL-DAG: mfhc1 $7, $f{{[0-9]+}}
define void @testlowercall15() nounwind {
entry:
  tail call void @f15(float 4.800000e+01, float 3.900000e+01, double 3.700000e+01) nounwind
  ret void
}

declare void @f15(float, float, double)

; $4, $5, $6, $7
; CHECK-LABEL: testlowercall16:
; CHECK-DAG: addiu $4, $zero, 62
; CHECK-DAG: lui $5, 16964
; CHECK-DAG: addiu $6, $zero, 64
; CHECK-DAG: lui $7, 16888
define void @testlowercall16() nounwind {
entry:
  tail call void @f16(i32 62, float 4.900000e+01, i32 64, float 3.100000e+01) nounwind
  ret void
}

declare void @f16(i32, float, i32, float)

; $4, $5, $6, $7
; CHECK-LABEL: testlowercall17:
; CHECK-DAG: addiu $4, $zero, 72
; CHECK-DAG: lui $5, 17004
; CHECK-DAG: addiu $6, $zero, 74
; CHECK-DAG: addiu $7, $zero, 35
define void @testlowercall17() nounwind {
entry:
  tail call void @f17(i32 72, float 5.900000e+01, i32 74, i32 35) nounwind
  ret void
}

declare void @f17(i32, float, i32, i32)

; $4, $5, $6, $7
; CHECK-LABEL: testlowercall18:
; CHECK-DAG: addiu $4, $zero, 82
; CHECK-DAG: addiu $5, $zero, 93
; CHECK-DAG: lui $6, 16928
; CHECK-DAG: addiu $7, $zero, 45
define void @testlowercall18() nounwind {
entry:
  tail call void @f18(i32 82, i32 93, float 4.000000e+01, i32 45) nounwind
  ret void
}

declare void @f18(i32, i32, float, i32)


; $4, ($6, $7), stack
; CHECK-LABEL: testlowercall20:
; CHECK-DAG: addiu $4, $zero, 92
; CHECK-DAG: sw ${{[a-z0-9]+}}, 16($sp)
; CHECK-DAG: sw ${{[a-z0-9]+}}, 20($sp)
; FP32EL-LABEL: testlowercall20:
; FP32EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP32EL-DAG: mfc1 $7, $f{{[0-9]+}}
; FP64EL-LABEL: testlowercall20:
; FP64EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP64EL-DAG: mfhc1 $7, $f{{[0-9]+}}
define void @testlowercall20() nounwind {
entry:
  tail call void @f20(i32 92, double 2.600000e+01, double 4.700000e+01) nounwind
  ret void
}

declare void @f20(i32, double, double)

; $f12, $5
; CHECK-LABEL: testlowercall21:
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: addiu $5, $zero, 103
define void @testlowercall21() nounwind {
entry:
  tail call void @f21(float 5.800000e+01, i32 103) nounwind
  ret void
}

declare void @f21(float, i32)

; $f12, $5, ($6, $7)
; CHECK-LABEL: testlowercall22:
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: addiu $5, $zero, 113
; FP32EL-LABEL: testlowercall22:
; FP32EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP32EL-DAG: mfc1 $7, $f{{[0-9]+}}
; FP64EL-LABEL: testlowercall22:
; FP64EL-DAG: mfc1 $6, $f{{[0-9]+}}
; FP64EL-DAG: mfhc1 $7, $f{{[0-9]+}}
define void @testlowercall22() nounwind {
entry:
  tail call void @f22(float 6.800000e+01, i32 113, double 5.700000e+01) nounwind
  ret void
}

declare void @f22(float, i32, double)

; $f12, f6
; CHECK-LABEL: testlowercall23:
; CHECK-DAG: ldc1 $f12, %lo
; CHECK-DAG: addiu $6, $zero, 123
define void @testlowercall23() nounwind {
entry:
  tail call void @f23(double 4.500000e+01, i32 123) nounwind
  ret void
}

declare void @f23(double, i32)

; $f12,$6, stack
; CHECK-LABEL: testlowercall24:
; CHECK-DAG: ldc1 $f12, %lo
; CHECK-DAG: addiu $6, $zero, 133
; CHECK-DAG: sw ${{[a-z0-9]+}}, 16($sp)
; CHECK-DAG: sw ${{[a-z0-9]+}}, 20($sp)
define void @testlowercall24() nounwind {
entry:
  tail call void @f24(double 5.500000e+01, i32 133, double 6.700000e+01) nounwind
  ret void
}

declare void @f24(double, i32, double)

; CHECK-LABEL: testlowercall25:
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: lwc1 $f14, %lo
; CHECK-DAG: lui $6
; CHECK-DAG: lui $7
; CHECK-DAG: lwc1 $f12, %lo
; CHECK-DAG: addiu $5, $zero, 83
; CHECK-DAG: lui $6
; CHECK-DAG: addiu $7, $zero, 25
; CHECK-DAG: addiu $4, $zero, 82
; CHECK-DAG: addiu $5, $zero, 93
; CHECK-DAG: lui $6
; CHECK-DAG: addiu $7, $zero, 45
define void @testlowercall25() nounwind {
entry:
  tail call void @f12(float 2.800000e+01, float 1.900000e+01, float 1.000000e+01, float 2.100000e+01) nounwind
  tail call void @f13(float 3.800000e+01, i32 83, float 2.000000e+01, i32 25) nounwind
  tail call void @f18(i32 82, i32 93, float 4.000000e+01, i32 45) nounwind
  ret void
}
