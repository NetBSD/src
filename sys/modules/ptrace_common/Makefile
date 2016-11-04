#	$NetBSD: Makefile,v 1.1.2.2 2016/11/04 14:49:20 pgoyette Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

.include <bsd.kmodule.mk>
