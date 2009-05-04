#	$NetBSD: Makefile,v 1.1.10.2 2009/05/04 08:14:09 yamt Exp $

.include "../Makefile.inc"

KMOD=	exec_script

CPPFLAGS+=	-DEXEC_SCRIPT

.PATH:	${S}/kern
SRCS=	exec_script.c

.include <bsd.kmodule.mk>
