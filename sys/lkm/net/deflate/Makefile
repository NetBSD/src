#	$NetBSD: Makefile,v 1.3.70.1 2008/04/03 12:43:06 mjf Exp $

.include "../Makefile.inc"

.PATH:	$S/net

CPPFLAGS+=-DDO_DEFLATE=1
KMOD=	deflate
SRCS=	lkminit_deflate.c ppp-deflate.c zlib.c

.include <bsd.kmod.mk>

.include "../Makefile.inc"
