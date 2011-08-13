# $NetBSD: Makefile,v 1.7 2011/08/13 23:12:15 christos Exp $

LIB=            perfuse
LIBDPLIBS+=     puffs	${.CURDIR}/../libpuffs

PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG

CWARNFLAGS.clang+=	-Wno-format-security

CFLAGS+=        ${PERFUSE_OPT_DEBUG_FLAGS}
CPPFLAGS+=	-I${.CURDIR} 
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libpuffs 
SRCS=           perfuse.c ops.c subr.c debug.c
MAN=		libperfuse.3
WARNS=          4
INCS=           perfuse.h
INCSDIR=	/usr/include

COPTS.ops.c = -Wno-format-nonliteral
COPTS.perfuse.c = -Wno-format-nonliteral
COPTS.subr.c = -Wno-format-nonliteral

.include <bsd.lib.mk>
