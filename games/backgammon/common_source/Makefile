#	$NetBSD: Makefile,v 1.10 2019/02/04 09:51:48 mrg Exp $

LIBISPRIVATE=	yes

LIB=		common
SRCS=		allow.c board.c check.c fancy.c init.c odds.c \
		one.c save.c subs.c table.c

.include <bsd.own.mk>

# Has fallthru comment that is missed
.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.check.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.lib.mk>
