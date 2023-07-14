# $NetBSD: Makefile,v 1.3.12.1 2023/07/14 08:33:25 martin Exp $

WARNS?= 5
.include <bsd.init.mk>

LIB=jemalloc

# The symbols from the stub only exist in newer jemalloc.
.if ${HAVE_JEMALLOC:U0} != 100
SRCS=jemalloc_stub.c
.endif

.include <bsd.lib.mk>
