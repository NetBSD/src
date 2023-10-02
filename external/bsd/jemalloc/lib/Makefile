# $NetBSD: Makefile,v 1.3.12.2 2023/10/02 17:38:23 martin Exp $

WARNS?= 5
.include <bsd.init.mk>

LIB=jemalloc

# The symbols from the stub only exist in newer jemalloc.
CPPFLAGS+=	-DHAVE_JEMALLOC=${HAVE_JEMALLOC:U0}
SRCS=		jemalloc_stub.c

.include <bsd.lib.mk>
