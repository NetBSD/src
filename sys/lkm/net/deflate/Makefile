#	$NetBSD: Makefile,v 1.1.2.2 2004/09/18 14:54:09 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/net

CPPFLAGS+=-DDO_DEFLATE=1
MKMAN=	no
KMOD=	deflate
SRCS=	lkminit_deflate.c ppp-deflate.c zlib.c

.include <bsd.kmod.mk>

.include "../Makefile.inc"
