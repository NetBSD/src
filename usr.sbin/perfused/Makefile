# $NetBSD: Makefile,v 1.4 2011/09/09 15:45:28 manu Exp $

PROG=            perfused

PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG

CFLAGS+=        ${PERFUSE_OPT_DEBUG_FLAGS}
SRCS=           perfused.c msg.c debug.c
MAN=		perfused.8
WARNS=          4

LDADD+= -lperfuse -lpuffs
DPADD+= ${LIBPUFFS}

CWARNFLAGS.clang+=	-Wno-format-security

.include <bsd.prog.mk>
