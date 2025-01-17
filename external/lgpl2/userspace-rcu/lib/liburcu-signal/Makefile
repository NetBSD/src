# $NetBSD: Makefile,v 1.1 2025/01/17 16:07:27 christos Exp $

.include <bsd.own.mk>

LIBISPRIVATE=yes
LIB=urcu-mb
CPPFLAGS+=-DRCU_SIGNAL -DRCU_MEMBARRIER

SRCS+= urcu.c urcu-pointer.c compat_arch.c compat_futex.c

.include <bsd.lib.mk>
