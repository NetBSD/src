# $NetBSD: Makefile,v 1.4 2007/04/12 15:09:01 pooka Exp $

LIB=		refuse
LIBDPLIBS=	puffs	${.CURDIR}/../libpuffs

.ifdef DEBUG
FUSE_OPT_DEBUG_FLAGS=	-g -DFUSE_OPT_DEBUG
.endif

CFLAGS+=	${FUSE_OPT_DEBUG_FLAGS}
SRCS=		refuse.c refuse_opt.c
CPPFLAGS+=	-I.
MAN=		refuse.3
WARNS=		4
INCS=           fuse.h fuse_opt.h
INCSDIR=        /usr/include
#DBG=		-g -O0

.include <bsd.lib.mk>
