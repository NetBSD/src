.macro cond op
.irp cond, eq, ne, gt, ge, lt, le
it \cond
\op\().s32 q0, q1, q2
.endr
.endm



.syntax unified
.text
.thumb
vmullb.s64 q0, q1, q2
vmullb.f16 q0, q1, q2
vmullb.f32 q0, q1, q2
vmullb.s32  q1, q1, q2
vmullb.s32  q2, q1, q2
cond vmullb
vmullt.s64 q0, q1, q2
vmullt.f16 q0, q1, q2
vmullt.f32 q0, q1, q2
vmullt.u32  q1, q1, q2
vmullt.u32  q2, q1, q2
cond vmullt
it eq
vmullbeq.s32 q0, q1, q2
vmullbeq.s32 q0, q1, q2
vpst
vmullbeq.s32 q0, q1, q2
vpst
vmullb.s32 q0, q1, q2
vmullbt.s32 q0, q1, q2
it eq
vmullteq.s32 q0, q1, q2
vmullteq.s32 q0, q1, q2
vpst
vmullteq.s32 q0, q1, q2
vpst
vmullt.s32 q0, q1, q2
vmulltt.s32 q0, q1, q2
