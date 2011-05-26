# $NetBSD: Makefile,v 1.4 2011/05/26 12:56:30 joerg Exp $

LIB=            perfuse
LIBDPLIBS+=     puffs	${.CURDIR}/../libpuffs

.ifdef DEBUG
PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG
.endif

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
