.macro cond
.irp cond, eq, ne, gt, ge, lt, le
it \cond
vpnot
.endr
.endm

.syntax unified
.thumb
cond
it eq
vpnoteq
vpnoteq
vpst
vpnoteq
vpnott
vpst
vpnot
