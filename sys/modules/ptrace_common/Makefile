#	$NetBSD: Makefile,v 1.4 2020/10/19 19:33:02 christos Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

.include "machdep.mk"

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c process_machdep.c

WARNS=	3

COPTS.sys_ptrace_common.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.kmodule.mk>
