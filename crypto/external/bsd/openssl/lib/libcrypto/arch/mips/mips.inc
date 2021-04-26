# $NetBSD: mips.inc,v 1.2 2021/04/26 20:01:13 christos Exp $

.ifndef MIPS_LE

.	if empty(MACHINE_ARCH:M*el)
MIPS_LE=0
.	else
MIPS_LE=1
.	endif

.	if (!empty(MACHINE_ARCH:Mmipsn64*) && empty(COPTS:M-mabi=*32*)) || \
	   (!empty(MACHINE_ARCH:Mmips64*) && !empty(COPTS:M-mabi=*64*))
MIPS_64=64
.	else
MIPS_64=
.	endif

.endif
