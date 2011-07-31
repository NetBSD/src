# $NetBSD: Makefile,v 1.1 2011/07/31 16:19:21 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	smsc
IOCONF=	smsc.ioconf
SRCS=	smsc.c

#CPPFLAGS+=-DLMDEBUG

WARNS=	4

.include <bsd.kmodule.mk>
