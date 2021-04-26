# $NetBSD: mips.inc,v 1.1 2021/04/26 18:06:09 christos Exp $

.ifndef MIPS_LE

.	if empty(MACHINE_ARCH:M*el)
MIPS_LE=0
.	else
MIPS_LE=1
.	endif

.	if empty(MACHINE_ARCH:Mmips*64*)
MIPS_64=
.	else
MIPS_64=64
.	endif

.endif
