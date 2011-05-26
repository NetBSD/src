# $NetBSD: Makefile,v 1.3 2011/05/26 12:56:34 joerg Exp $

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

CWARNFLAGS.clang+=	-Wno-format-security

.include <bsd.prog.mk>
