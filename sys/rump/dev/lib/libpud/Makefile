#	$NetBSD: Makefile,v 1.1.8.1 2014/05/22 11:41:12 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pud

LIB=	rumpdev_pud

SRCS=	pud.c pud_dev.c

SRCS+=	pud_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
