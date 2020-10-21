#	$NetBSD: Makefile,v 1.5 2020/10/21 13:30:02 christos Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

WARNS=	3

COPTS.sys_ptrace_common.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.kmodule.mk>
