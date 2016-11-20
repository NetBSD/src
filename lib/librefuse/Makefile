# $NetBSD: Makefile,v 1.11 2016/11/20 13:26:28 pho Exp $

USE_FORT?=	yes	# data driven bugs?

LIB=		refuse
LIBDPLIBS+=	puffs	${.CURDIR}/../libpuffs		\
		pthread	${.CURDIR}/../libpthread

.ifdef DEBUG
FUSE_OPT_DEBUG_FLAGS=	-g -DFUSE_OPT_DEBUG
.endif

CFLAGS+=	${FUSE_OPT_DEBUG_FLAGS}
CPPFLAGS+=	-I${.CURDIR} -D_KERNTYPES
SRCS=		refuse.c refuse_opt.c refuse_lowlevel.c
MAN=		refuse.3
WARNS?=		5
INCS=           fuse.h fuse_opt.h fuse_lowlevel.h
INCSDIR=        /usr/include

.include <bsd.lib.mk>
