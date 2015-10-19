#	$NetBSD: Makefile,v 1.7 2015/10/19 16:16:34 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/putter

LIB=	rumpdev_putter
COMMENT=User/kernel protocol transporter (for puffs and pud)

SRCS=	putter.c
SRCS+=	putter_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
