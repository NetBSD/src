# $NetBSD: Makefile,v 1.3 2005/12/11 12:24:49 christos Exp $

.include "../Makefile.inc"

.PATH: ${S}/net

SRCS=	if_tap_lkm.c if_tap.c
KMOD=	tap
WARNS=	3

CPPFLAGS+=	-DNBPFILTER=1

.include <bsd.kmod.mk>
