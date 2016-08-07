# $NetBSD: Makefile,v 1.1 2016/08/07 14:27:39 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_loop
IOCONF=		loop.ioconf
SRCS=		if_loop.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6
CPPFLAGS+=	-DMPLS
CPPFLAGS+=	-DNETATALK

.include <bsd.kmodule.mk>
