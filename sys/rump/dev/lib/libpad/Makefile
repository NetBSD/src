#	$NetBSD: Makefile,v 1.7 2017/12/16 09:20:29 pgoyette Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pad

LIB=	rumpdev_pad
COMMENT=Pseudo Audio Device

IOCONF=	PAD.ioconf

SRCS=	pad.c
SRCS+=	pad_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
