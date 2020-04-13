#	$NetBSD: Makefile,v 1.9.62.2 2020/04/13 08:03:06 martin Exp $

LIBISPRIVATE=	yes

LIB=		common
SRCS=		allow.c board.c check.c fancy.c init.c odds.c \
		one.c save.c subs.c table.c

.include <bsd.own.mk>

# Has fallthru comment that is missed
.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.check.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.lib.mk>
