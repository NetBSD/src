#	$NetBSD: Makefile,v 1.5 2019/09/26 02:01:04 christos Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_ptrace

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DPTRACE
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_ptrace.c

.include <bsd.kmodule.mk>
