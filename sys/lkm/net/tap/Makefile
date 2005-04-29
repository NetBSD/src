# $NetBSD: Makefile,v 1.2.10.2 2005/04/29 11:29:29 kent Exp $

.include "../Makefile.inc"

.PATH: ${S}/net

SRCS=	if_tap_lkm.c if_tap.c
KMOD=	tap
WARNS=	3

CPPFLAGS+=	-DNBPFILTER=1

.include <bsd.kmod.mk>
