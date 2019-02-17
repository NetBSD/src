#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:59 rin Exp $

.include "../Makefile.inc"

CPPFLAGS+=

.PATH:	${S}/dev/tprof

KMOD=	tprof
IOCONF=	tprof.ioconf
SRCS=	tprof.c

WARNS=	3

.include <bsd.kmodule.mk>
