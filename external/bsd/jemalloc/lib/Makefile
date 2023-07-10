# $NetBSD: Makefile,v 1.4 2023/07/10 07:59:24 mrg Exp $

WARNS?= 5
.include <bsd.init.mk>

LIB=jemalloc

# The symbols from the stub only exist in newer jemalloc.
.if ${HAVE_JEMALLOC:U0} != 100
SRCS=jemalloc_stub.c
.endif

.include <bsd.lib.mk>
