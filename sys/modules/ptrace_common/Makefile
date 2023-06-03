#	$NetBSD: Makefile,v 1.6 2023/06/03 09:09:14 lukem Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

WARNS=	3

COPTS.sys_ptrace_common.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.kmodule.mk>
