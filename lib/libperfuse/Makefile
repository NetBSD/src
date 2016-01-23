# $NetBSD: Makefile,v 1.11 2016/01/23 21:22:46 christos Exp $

LIB=            perfuse
LIBDPLIBS+=     puffs	${.CURDIR}/../libpuffs

PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG

CWARNFLAGS.clang+=	-Wno-format-security

#CFLAGS+=        ${PERFUSE_OPT_DEBUG_FLAGS}
CPPFLAGS+=	-I${.CURDIR}  -D_KERNTYPES
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libpuffs 
SRCS=           perfuse.c ops.c subr.c debug.c
MAN=		libperfuse.3
WARNS?=		5
INCS=           perfuse.h
INCSDIR=	/usr/include

.include <bsd.lib.mk>
