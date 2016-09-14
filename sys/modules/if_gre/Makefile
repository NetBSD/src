# $NetBSD: Makefile,v 1.1.2.2 2016/09/14 03:04:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_gre
IOCONF=		gre.ioconf
SRCS=		if_gre.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6
CPPFLAGS+=	-DMPLS
CPPFLAGS+=	-DNETATALK

.include <bsd.kmodule.mk>
