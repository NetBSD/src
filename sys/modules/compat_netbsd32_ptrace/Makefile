#	$NetBSD: Makefile,v 1.1 2016/10/19 09:44:01 skrll Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_ptrace

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DPTRACE
CPPFLAGS+=	-DPT_SETREGS -DPT_GETREGS

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_ptrace.c

.include <bsd.kmodule.mk>
