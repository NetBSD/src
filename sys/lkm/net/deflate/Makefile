#	$NetBSD: Makefile,v 1.1.2.3 2004/09/21 13:36:24 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/net

CPPFLAGS+=-DDO_DEFLATE=1
MKMAN=	no
KMOD=	deflate
SRCS=	lkminit_deflate.c ppp-deflate.c zlib.c

.include <bsd.kmod.mk>

.include "../Makefile.inc"
