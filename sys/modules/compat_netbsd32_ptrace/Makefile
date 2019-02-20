#	$NetBSD: Makefile,v 1.4 2019/02/20 05:11:31 rin Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_ptrace

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DPTRACE
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_ptrace.c

.if !empty(MACHINE_ARCH:Mmips64*)
WARNS=	3
.endif

.include <bsd.kmodule.mk>
