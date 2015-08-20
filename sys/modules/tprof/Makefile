#	$NetBSD: Makefile,v 1.2 2015/08/20 11:05:01 christos Exp $

.include "../Makefile.inc"

CPPFLAGS+=

.PATH:	${S}/dev/tprof

KMOD=	tprof
IOCONF=	tprof.ioconf
SRCS=	tprof.c

.include <bsd.kmodule.mk>
