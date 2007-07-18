# $NetBSD: Makefile,v 1.6 2007/07/18 22:38:59 pooka Exp $

USE_FORT?=	yes	# data driven bugs?

LIB=		refuse
LIBDPLIBS+=	puffs	${.CURDIR}/../libpuffs

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

.include <bsd.lib.mk>
