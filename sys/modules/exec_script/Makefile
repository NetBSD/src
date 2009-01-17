#	$NetBSD: Makefile,v 1.1.4.2 2009/01/17 13:29:29 mjf Exp $

.include "../Makefile.inc"

KMOD=	exec_script

CPPFLAGS+=	-DEXEC_SCRIPT

.PATH:	${S}/kern
SRCS=	exec_script.c

.include <bsd.kmodule.mk>
