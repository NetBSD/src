#	$NetBSD: Makefile,v 1.3 2019/02/20 05:11:30 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_13

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_13 -DCOMPAT_14 -DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

SRCS+=	netbsd32_compat_13.c

.if ${MACHINE_ARCH} == "x86_64"
.PATH:  ${S}/arch/amd64/amd64
SRCS+=  netbsd32_machdep_13.c
.endif

.if ${MACHINE_ARCH} == "arm"
.PATH:	${S}/arch/arm/arm32
SRCS+=  netbsd32_machdep_13.c
.endif

.if !empty(MACHINE_ARCH:Mmips64*)
WARNS=	3
.endif

.include <bsd.kmodule.mk>
