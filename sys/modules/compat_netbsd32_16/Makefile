#	$NetBSD: Makefile,v 1.1.2.4 2018/09/23 01:33:26 pgoyette Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

.PATH:	${S}/compat/netbsd32

KMOD=	compat_netbsd32_16

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_16 -DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	netbsd32_compat_16.c

.if ${MACHINE_ARCH} == "x86_64"
.PATH:  ${S}/arch/amd64/amd64
SRCS+=  netbsd32_sigcode.S
.endif

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
SRCS+=	netbsd32_machdep_16.c

.include <bsd.kmodule.mk>
