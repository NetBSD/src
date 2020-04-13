#	$NetBSD: Makefile,v 1.1.22.2 2020/04/13 08:05:13 martin Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

WARNS=	3

COPTS.sys_ptrace_common.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.kmodule.mk>
