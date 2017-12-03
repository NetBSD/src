#	$NetBSD: Makefile,v 1.1.28.1 2017/12/03 11:39:00 jdolecek Exp $

.include "../Makefile.inc"

CPPFLAGS+=

.PATH:	${S}/dev/tprof

KMOD=	tprof
IOCONF=	tprof.ioconf
SRCS=	tprof.c

.include <bsd.kmodule.mk>
