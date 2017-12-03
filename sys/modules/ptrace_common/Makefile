#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:59 jdolecek Exp $
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPTRACE

KMOD=	ptrace_common
SRCS=	sys_ptrace_common.c

.include <bsd.kmodule.mk>
