.macro cond
.irp cond, eq, ne, gt, ge, lt, le
it \cond
vpsel.i16 q0, q1, q2
.endr
.endm

.syntax unified
.thumb
cond
it eq
vpseleq.i16 q0, q1, q2
vpseleq.i16 q0, q1, q2
vpst
vpseleq.i16 q0, q1, q2
vpselt.i16 q0, q1, q2
vpst
vpsel.i16 q0, q1, q2

