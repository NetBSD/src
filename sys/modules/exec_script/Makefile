#	$NetBSD: Makefile,v 1.1 2008/11/19 18:36:08 ad Exp $

.include "../Makefile.inc"

KMOD=	exec_script

CPPFLAGS+=	-DEXEC_SCRIPT

.PATH:	${S}/kern
SRCS=	exec_script.c

.include <bsd.kmodule.mk>
