# $NetBSD: Makefile,v 1.1.54.1 2019/06/10 22:09:38 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	smsc
IOCONF=	smsc.ioconf
SRCS=	smsc.c

#CPPFLAGS+=-DLMDEBUG

.include <bsd.kmodule.mk>
