#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:56 rin Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

WARNS=	3

.include <bsd.kmodule.mk>
