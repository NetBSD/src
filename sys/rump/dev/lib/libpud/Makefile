#	$NetBSD: Makefile,v 1.2.6.2 2016/03/19 11:30:35 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pud

LIB=	rumpdev_pud
COMMENT=Userspace character and block driver framework

SRCS=	pud.c pud_dev.c

SRCS+=	pud_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
