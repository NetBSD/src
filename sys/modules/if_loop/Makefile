# $NetBSD: Makefile,v 1.1.4.2 2016/10/05 20:56:05 skrll Exp $

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
