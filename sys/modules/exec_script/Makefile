#	$NetBSD: Makefile,v 1.1.6.2 2009/01/19 13:20:09 skrll Exp $

.include "../Makefile.inc"

KMOD=	exec_script

CPPFLAGS+=	-DEXEC_SCRIPT

.PATH:	${S}/kern
SRCS=	exec_script.c

.include <bsd.kmodule.mk>
