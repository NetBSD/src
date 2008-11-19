#	$NetBSD: Makefile,v 1.1 2008/11/19 18:36:08 ad Exp $

.include "../Makefile.inc"

KMOD=	exec_aout

CPPFLAGS+=	-DEXEC_AOUT -DCOREDUMP

.PATH:	${S}/kern
SRCS=	exec_aout.c

.include <bsd.kmodule.mk>
