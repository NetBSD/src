#	$NetBSD: Makefile,v 1.1.4.2 2016/12/05 10:55:27 skrll Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_ptrace

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DPTRACE
CPPFLAGS+=	-DPT_SETREGS -DPT_GETREGS

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_ptrace.c

.include <bsd.kmodule.mk>
