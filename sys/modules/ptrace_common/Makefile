#	$NetBSD: Makefile,v 1.3 2019/10/13 07:28:13 mrg Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

WARNS=	3

COPTS.sys_ptrace_common.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.kmodule.mk>
