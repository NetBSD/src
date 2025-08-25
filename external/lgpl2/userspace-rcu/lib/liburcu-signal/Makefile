# $NetBSD: Makefile,v 1.1.2.1 2025/08/25 16:17:25 martin Exp $

.include <bsd.own.mk>

LIBISPRIVATE=yes
LIB=urcu-signal
CPPFLAGS+=-DRCU_SIGNAL -DRCU_MEMBARRIER

SRCS+= urcu.c urcu-pointer.c compat_arch.c compat_futex.c

.include <bsd.lib.mk>
