#	$NetBSD: Makefile,v 1.2.18.1 2019/06/10 22:09:41 christos Exp $

.include "../Makefile.inc"

CPPFLAGS+=

.PATH:	${S}/dev/tprof

KMOD=	tprof
IOCONF=	tprof.ioconf
SRCS=	tprof.c

WARNS=	3

.include <bsd.kmodule.mk>
