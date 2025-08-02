# $NetBSD: Makefile,v 1.5.2.1 2025/08/02 05:21:36 perseant Exp $

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
