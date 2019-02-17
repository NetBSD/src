# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:58 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	smsc
IOCONF=	smsc.ioconf
SRCS=	smsc.c

#CPPFLAGS+=-DLMDEBUG

.include <bsd.kmodule.mk>
