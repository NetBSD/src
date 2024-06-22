#	$NetBSD: Makefile,v 1.2.32.1 2024/06/22 11:11:53 martin Exp $

.include <bsd.init.mk>

.PATH:	${S}/compat/common

KMOD=	compat_16

CPPFLAGS+=	-DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_16_mod.c
SRCS+=	kern_sig_16.c

COMPAT_MACHDEP_SRCS=	compat_16_machdep.c
.if ${MACHINE_CPU} == "m68k"
.include "../Makefile.assym"

COMPAT_MACHDEP_SRCS+=	compat_16_sigreturn14.s
COPTS.compat_16_sigreturn14.s+=	-I. -x assembler-with-cpp
.endif
.if ${MACHINE_CPU} == "powerpc"
COMPAT_MACHDEP_SRCS+=	sigcode.S
.endif

.include "../Makefile.compat"

.include <bsd.kmodule.mk>
