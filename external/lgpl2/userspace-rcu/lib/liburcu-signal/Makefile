# $NetBSD: Makefile,v 1.2 2025/08/24 04:24:17 mrg Exp $

.include <bsd.own.mk>

LIBISPRIVATE=yes
LIB=urcu-signal
CPPFLAGS+=-DRCU_SIGNAL -DRCU_MEMBARRIER

SRCS+= urcu.c urcu-pointer.c compat_arch.c compat_futex.c

.include <bsd.lib.mk>
