#	$NetBSD: Makefile,v 1.4.26.1 2024/06/22 11:11:52 martin Exp $

.include <bsd.init.mk>

.include "../Makefile.assym"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_16

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SRCS+=	netbsd32_compat_16.c

COMPAT_MACHDEP_SRCS=	netbsd32_machdep_16.c
.if ${MACHINE_CPU} == "powerpc" || ${MACHINE_CPU} == "x86_64"
COMPAT_MACHDEP_SRCS+=	netbsd32_sigcode.S
.endif

.include "../Makefile.compat"

.include <bsd.kmodule.mk>
