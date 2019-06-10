# $NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:27 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_gre
IOCONF=		gre.ioconf
SRCS=		if_gre.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6
CPPFLAGS+=	-DMPLS
CPPFLAGS+=	-DNETATALK

WARNS=		3

.include <bsd.kmodule.mk>
