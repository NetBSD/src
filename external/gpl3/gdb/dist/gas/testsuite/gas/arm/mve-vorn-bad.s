.macro cond1
.irp cond, eq, ne, gt, ge, lt, le
it \cond
vorn q0, q1, q2
.endr
.endm

.macro cond2
.irp cond, eq, ne, gt, ge, lt, le
it \cond
vorn.i16 q0, #255
.endr
.endm

.syntax unified
.thumb
cond1
it eq
vorneq q0, q1, q2
vorneq q0, q1, q2
vpst
vorneq q0, q1, q2
vpst
vorn q0, q1, q2
vornt q0, q1, q2
cond2
it eq
vorneq.i16 q0, #255
vorneq.i16 q0, #255
vpst
vorneq.i16 q0, #255
vpst
vorn.i16 q0, #255
vornt.i16 q0, #255
vorn.i8 q0, #255
vorn.i64 q0, #255
vorn.i16 q0, #0
vorn.i32 q0, #0
