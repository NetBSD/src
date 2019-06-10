#	$NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:36 christos Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

WARNS=	3

.include <bsd.kmodule.mk>
