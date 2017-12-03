#	$NetBSD: Makefile,v 1.3.14.3 2017/12/03 11:39:09 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev/putter

LIB=	rumpdev_putter
COMMENT=User/kernel protocol transporter (for puffs and pud)

SRCS=	putter.c
SRCS+=	putter_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
