#	$NetBSD: Makefile,v 1.4 2016/01/26 23:12:15 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pud

LIB=	rumpdev_pud
COMMENT=Userspace character and block driver framework

SRCS=	pud.c pud_dev.c

SRCS+=	pud_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
