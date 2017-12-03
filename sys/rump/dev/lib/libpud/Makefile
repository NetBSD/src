#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:08 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pud

LIB=	rumpdev_pud
COMMENT=Userspace character and block driver framework

SRCS=	pud.c pud_dev.c

SRCS+=	pud_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
