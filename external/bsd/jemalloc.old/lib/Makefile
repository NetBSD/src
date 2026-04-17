# $NetBSD: Makefile,v 1.3 2026/04/17 16:19:42 christos Exp $

WARNS?= 5
.include <bsd.init.mk>

LIB=jemalloc

# The symbols from the stub only exist in newer jemalloc.
CPPFLAGS+=	-DHAVE_JEMALLOC=${HAVE_JEMALLOC:U0}
SRCS=		jemalloc_stub.c

.if ${HAVE_JEMALLOC:U0} <= 100
LIB_EXPSYM=	/dev/null
.endif

.include <bsd.lib.mk>
