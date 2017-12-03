# $NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:56 jdolecek Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_strip
IOCONF=		strip.ioconf
SRCS=		if_strip.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
