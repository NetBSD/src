# $NetBSD: Makefile,v 1.1 2025/01/17 16:07:27 christos Exp $

NOLINT=yes	# XXX
.include <bsd.own.mk>

LIBISPRIVATE=yes
LIB=urcu-cds

SRCS+= rculfqueue.c rculfstack.c lfstack.c workqueue.c rculfhash.c
SRCS+= rculfhash-mm-order.c rculfhash-mm-chunk.c rculfhash-mm-mmap.c
SRCS+= compat_arch.c compat_futex.c

.include <bsd.lib.mk>
