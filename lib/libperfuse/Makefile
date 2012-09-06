# $NetBSD: Makefile,v 1.10 2012/09/06 16:39:07 riz Exp $

LIB=            perfuse
LIBDPLIBS+=     puffs	${.CURDIR}/../libpuffs

PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG

CWARNFLAGS.clang+=	-Wno-format-security

#CFLAGS+=        ${PERFUSE_OPT_DEBUG_FLAGS}
CPPFLAGS+=	-I${.CURDIR} 
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libpuffs 
SRCS=           perfuse.c ops.c subr.c debug.c
MAN=		libperfuse.3
WARNS?=		5
INCS=           perfuse.h
INCSDIR=	/usr/include

.include <bsd.lib.mk>
