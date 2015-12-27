#	$NetBSD: Makefile,v 1.2.6.1 2015/12/27 12:10:10 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pud

LIB=	rumpdev_pud
COMMENT=Userspace character and block driver framework

SRCS=	pud.c pud_dev.c

SRCS+=	pud_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
