#	$NetBSD: Makefile,v 1.2.14.2 2017/12/03 11:38:51 jdolecek Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_ptrace

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DPTRACE

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_ptrace.c

.include <bsd.kmodule.mk>
