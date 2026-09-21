# $NetBSD: Makefile,v 1.4 2026/09/21 23:23:36 christos Exp $

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
