#	$NetBSD: Makefile,v 1.1.2.1 2004/08/03 10:53:59 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/net

CPPFLAGS+=-DDO_DEFLATE=1
MKMAN=	no
KMOD=	deflate
SRCS=	lkminit_deflate.c ppp-deflate.c zlib.c

.include <bsd.kmod.mk>

.include "../Makefile.inc"
