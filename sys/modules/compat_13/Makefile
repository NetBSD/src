#	$NetBSD: Makefile,v 1.2.4.2 2019/06/10 22:09:13 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_13

CPPFLAGS+=	-DCOMPAT_13 -DCOMPAT_14 -DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_13_mod.c
SRCS+=	uvm_13.c kern_sig_13.c

.if ${MACHINE_ARCH} == "x86_64"
.PATH:	${S}/arch/amd64/amd64

SRCS+=	compat_13_machdep.c
.endif

.include <bsd.kmodule.mk>
