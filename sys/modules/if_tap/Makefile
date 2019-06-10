# $NetBSD: Makefile,v 1.1.4.2 2019/06/10 22:09:29 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_tap
IOCONF=		tap.ioconf
SRCS=		if_tap.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
