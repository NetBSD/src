# $NetBSD: Makefile,v 1.1.2.1 2025/08/25 16:17:26 martin Exp $

.include <bsd.own.mk>

LIBISPRIVATE=yes
LIB=urcu
CPPFLAGS+=-DRCU_MB

SRCS+= urcu.c urcu-pointer.c compat_arch.c compat_futex.c

.include <bsd.lib.mk>
