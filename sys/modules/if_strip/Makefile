# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:52 rin Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_strip
IOCONF=		strip.ioconf
SRCS=		if_strip.c

CPPFLAGS+=	-DINET

WARNS=		3

.include <bsd.kmodule.mk>
