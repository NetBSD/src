#	$NetBSD: Makefile,v 1.1 2011/03/31 08:36:25 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pud

LIB=	rumpdev_pud

SRCS=	pud.c pud_dev.c

SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
