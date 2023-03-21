.macro cond1
.irp cond, eq, ne, gt, ge, lt, le
it \cond
wlstp.8 lr, r0, .label
.endr
.endm

.macro cond2
.irp cond, eq, ne, gt, ge, lt, le
it \cond
dlstp.8 lr, r0
.endr
.endm

.macro cond3
.irp cond, eq, ne, gt, ge, lt, le
it \cond
letp lr, .label_back
.endr
.endm

.label_back:
.syntax unified
.thumb
cond1
cond2
cond3
wlstp.8 lr, pc, .label
wlstp.8 lr, sp, .label
dlstp.16 lr, pc
dlstp.16 lr, sp
.label:
letp .label_back
wlstp.8 lr, r0, .label
letp lr, .label2
.label2:
