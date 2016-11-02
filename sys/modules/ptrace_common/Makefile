#	$NetBSD: Makefile,v 1.1 2016/11/02 00:12:00 pgoyette Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

.include <bsd.kmodule.mk>
