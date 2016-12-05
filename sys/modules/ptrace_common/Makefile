#	$NetBSD: Makefile,v 1.1.4.2 2016/12/05 10:55:27 skrll Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

.include <bsd.kmodule.mk>
