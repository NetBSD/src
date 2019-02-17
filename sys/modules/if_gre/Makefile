# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:51 rin Exp $

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
