# $NetBSD: Makefile,v 1.8 2011/08/14 08:19:04 christos Exp $

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

.include <bsd.lib.mk>
