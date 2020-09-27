#	$NetBSD: Makefile.inc,v 1.8 2020/09/27 00:31:51 mrg Exp $

GMP_LIMB_BITS=32

M4FLAGS= -DPIC
COPTS+= -fPIC

.if ${MACHINE_ARCH} == "arm" || !empty(MACHINE_ARCH:Mearmv4*)
M4FLAGS+=	-DNOTHUMB
.endif

.include "srcs.mk"
