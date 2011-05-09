# $NetBSD: Makefile,v 1.2 2011/05/09 08:51:18 manu Exp $

PROG=            perfused

.ifdef DEBUG
PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG
.endif

CFLAGS+=        ${PERFUSE_OPT_DEBUG_FLAGS}
SRCS=           perfused.c msg.c debug.c
MAN=		perfused.8
WARNS=          4

LDADD+= -lperfuse -lpuffs
DPADD+= ${LIBPUFFS}

.include <bsd.prog.mk>
