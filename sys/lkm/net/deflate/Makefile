#	Makefile,v 1.3 2005/12/11 12:24:49 christos Exp

.include "../Makefile.inc"

.PATH:	$S/net

CPPFLAGS+=-DDO_DEFLATE=1
KMOD=	deflate
SRCS=	lkminit_deflate.c ppp-deflate.c zlib.c

.include <bsd.kmod.mk>

.include "../Makefile.inc"
