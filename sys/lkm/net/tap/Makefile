# $NetBSD: Makefile,v 1.1 2005/01/19 11:09:05 cube Exp $

.include "../Makefile.inc"

.PATH: ${S}/net

SRCS=	if_tap_lkm.c if_tap.c
KMOD=	tap
WARNS=	3

.include <bsd.kmod.mk>
