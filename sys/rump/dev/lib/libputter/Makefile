#	$NetBSD: Makefile,v 1.6.6.1 2015/12/27 12:10:10 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/putter

LIB=	rumpdev_putter
COMMENT=User/kernel protocol transporter (for puffs and pud)

SRCS=	putter.c
SRCS+=	putter_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
