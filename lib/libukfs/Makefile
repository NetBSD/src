#	$NetBSD: Makefile,v 1.8 2016/01/23 21:22:47 christos Exp $
#

WARNS=		2

LIB=		ukfs
LIBDPLIBS+=	rump	${.CURDIR}/../librump		\
		rumpvfs	${.CURDIR}/../librumpvfs \
		pthread	${.CURDIR}/../libpthread
CPPFLAGS+=	-I${.CURDIR} -D_KERNTYPES

INCS=		ukfs.h
INCSDIR=	/usr/include/rump

SRCS=		ukfs.c ukfs_disklabel.c
MAN=		ukfs.3

.include <bsd.lib.mk>
