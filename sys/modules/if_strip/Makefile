# $NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:29 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_strip
IOCONF=		strip.ioconf
SRCS=		if_strip.c

CPPFLAGS+=	-DINET

WARNS=		3

.include <bsd.kmodule.mk>
