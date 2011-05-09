# $NetBSD: Makefile,v 1.3 2011/05/09 08:51:18 manu Exp $

LIB=            perfuse
LIBDPLIBS+=     puffs	${.CURDIR}/../libpuffs

.ifdef DEBUG
PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG
.endif

CFLAGS+=        ${PERFUSE_OPT_DEBUG_FLAGS}
CPPFLAGS+=	-I${.CURDIR} 
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libpuffs 
SRCS=           perfuse.c ops.c subr.c debug.c
MAN=		libperfuse.3
WARNS=          4
INCS=           perfuse.h
INCSDIR=	/usr/include

.include <bsd.lib.mk>
