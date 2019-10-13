# $NetBSD: Makefile,v 1.7 2019/10/13 07:28:19 mrg Exp $

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
COPTS.perfused.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
