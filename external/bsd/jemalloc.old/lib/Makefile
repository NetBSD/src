# $NetBSD: Makefile,v 1.1.1.1 2024/09/23 12:52:54 christos Exp $

WARNS?= 5
.include <bsd.init.mk>

LIB=jemalloc

# The symbols from the stub only exist in newer jemalloc.
CPPFLAGS+=	-DHAVE_JEMALLOC=${HAVE_JEMALLOC:U0}
SRCS=		jemalloc_stub.c

.include <bsd.lib.mk>
