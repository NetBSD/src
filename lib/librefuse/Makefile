# $NetBSD: Makefile,v 1.2 2007/02/16 15:44:37 pooka Exp $

LIB=		refuse
LIBDPLIBS=	puffs	${.CURDIR}/../libpuffs

SRCS=		refuse.c
CPPFLAGS+=	-I.
MAN=		refuse.3
WARNS=		4
INCS=           fuse.h
INCSDIR=        /usr/include

.include <bsd.lib.mk>
