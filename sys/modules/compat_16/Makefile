#	$NetBSD: Makefile,v 1.3 2024/06/16 21:52:01 pgoyette Exp $

.include <bsd.init.mk>

.PATH:	${S}/compat/common

KMOD=	compat_16

CPPFLAGS+=	-DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_16_mod.c
SRCS+=	kern_sig_16.c

COMPAT_MACHDEP_SRCS=	compat_16_machdep.c
.if ${MACHINE_CPU} == "m68k"
COMPAT_MACHDEP_SRCS+=	compat_16_sigreturn14.s
.endif
.if ${MACHINE_CPU} == "powerpc"
COMPAT_MACHDEP_SRCS+=	sigcode.S
.endif

.include "../Makefile.compat"

.include <bsd.kmodule.mk>
