# $NetBSD: Makefile,v 1.8 2023/06/03 09:09:22 lukem Exp $

PROG=            perfused

PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG

#CFLAGS+=        ${PERFUSE_OPT_DEBUG_FLAGS}
SRCS=           perfused.c msg.c debug.c
MAN=		perfused.8
WARNS=          4

CPPFLAGS+=	-D_KERNTYPES
LDADD+= -lperfuse -lpuffs
DPADD+= ${LIBPUFFS}

CWARNFLAGS.clang+=	-Wno-format-security
COPTS.perfused.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
