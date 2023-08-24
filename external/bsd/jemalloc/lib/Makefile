# $NetBSD: Makefile,v 1.5 2023/08/24 06:02:44 rin Exp $

WARNS?= 5
.include <bsd.init.mk>

LIB=jemalloc

# The symbols from the stub only exist in newer jemalloc.
CPPFLAGS+=	-DHAVE_JEMALLOC=${HAVE_JEMALLOC:U0}
SRCS=		jemalloc_stub.c

.include <bsd.lib.mk>
