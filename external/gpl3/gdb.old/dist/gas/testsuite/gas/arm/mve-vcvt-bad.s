.macro cond1
.irp cond, eq, ne, gt, ge, lt, le
it \cond
vcvt\().f16.s16 q0, q1, #1
.endr
.endm

.syntax unified
.thumb

vcvt.f16.s16 q0, q1, #0
vcvt.f16.s16 q0, q1, #17
vcvt.f16.u16 q0, q1, #0
vcvt.f16.u16 q0, q1, #17
vcvt.s16.f16 q0, q1, #0
vcvt.s16.f16 q0, q1, #17
vcvt.u16.f16 q0, q1, #0
vcvt.u16.f16 q0, q1, #17
vcvt.f32.s32 q0, q1, #0
vcvt.f32.s32 q0, q1, #33
vcvt.f32.u32 q0, q1, #0
vcvt.f32.u32 q0, q1, #33
vcvt.s32.f32 q0, q1, #0
vcvt.s32.f32 q0, q1, #33
vcvt.u32.f32 q0, q1, #0
vcvt.u32.f32 q0, q1, #33
vcvt.f64.s64 q0, q1, #1
vcvt.f64.u64 q0, q1, #1
vcvt.s64.f64 q0, q1, #1
vcvt.u64.f64 q0, q1, #1
cond1
it eq
vcvteq.f16.s16 q0, q1, #1
vcvteq.f16.s16 q0, q1, #1
vpst
vcvteq.f16.s16 q0, q1, #1
vcvtt.f16.s16 q0, q1, #1
vpst
vcvt.f16.s16 q0, q1, #1

.macro cond2
.irp cond, eq, ne, gt, ge, lt, le
it \cond
vcvt\().f16.s16 q0, q1
.endr
.endm

cond2
vcvt.f64.s64 q0, q1
vcvt.f64.u64 q0, q1
vcvt.s64.f64 q0, q1
vcvt.u64.f64 q0, q1
it eq
vcvteq.u32.f32 q0, q1
vcvteq.u32.f32 q0, q1
vpst
vcvteq.u32.f32 q0, q1
vcvtt.u32.f32 q0, q1
vpst
vcvt.u32.f32 q0, q1

.macro cond3 mnem
.irp cond, eq, ne, gt, ge, lt, le
it \cond
\mnem\().f16.f32 q0, q1
.endr
.endm

cond3 vcvtb
vcvtb.f16.f64 q0, q1
vcvtb.f64.f16 q0, q1
vcvtb.f32.f64 q0, q1
vcvtb.f64.f32 q0, q1
it eq
vcvtbeq.f16.f32 q0, q1
vcvtbeq.f16.f32 q0, q1
vpst
vcvtbeq.f16.f32 q0, q1
vcvtbt.f16.f32 q0, q1
vpst
vcvtb.f16.f32 q0, q1
cond3 vcvtt
vcvtt.f16.f64 q0, q1
vcvtt.f64.f16 q0, q1
vcvtt.f32.f64 q0, q1
vcvtt.f64.f32 q0, q1
it eq
vcvtteq.f16.f32 q0, q1
vcvtteq.f16.f32 q0, q1
vpst
vcvtteq.f16.f32 q0, q1
vcvttt.f16.f32 q0, q1
vpst
vcvtt.f16.f32 q0, q1
