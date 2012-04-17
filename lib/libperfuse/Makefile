# $NetBSD: Makefile,v 1.8.2.1 2012/04/17 00:05:30 yamt Exp $

LIB=            perfuse
LIBDPLIBS+=     puffs	${.CURDIR}/../libpuffs

PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG

CWARNFLAGS.clang+=	-Wno-format-security

CFLAGS+=        ${PERFUSE_OPT_DEBUG_FLAGS}
CPPFLAGS+=	-I${.CURDIR} 
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libpuffs 
SRCS=           perfuse.c ops.c subr.c debug.c
MAN=		libperfuse.3
WARNS?=		5
INCS=           perfuse.h
INCSDIR=	/usr/include

.include <bsd.lib.mk>
