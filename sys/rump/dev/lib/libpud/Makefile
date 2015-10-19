#	$NetBSD: Makefile,v 1.3 2015/10/19 16:16:34 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pud

LIB=	rumpdev_pud
COMMENT=Userspace character and block driver framework

SRCS=	pud.c pud_dev.c

SRCS+=	pud_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
