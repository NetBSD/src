#	$NetBSD: Makefile,v 1.1.46.1 2015/09/22 12:06:10 skrll Exp $

.include "../Makefile.inc"

CPPFLAGS+=

.PATH:	${S}/dev/tprof

KMOD=	tprof
IOCONF=	tprof.ioconf
SRCS=	tprof.c

.include <bsd.kmodule.mk>
