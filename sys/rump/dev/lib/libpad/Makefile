#	$NetBSD: Makefile,v 1.4.2.1 2015/12/27 12:10:09 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pad

LIB=	rumpdev_pad
COMMENT=Pseudo Audio Device

SRCS=	pad.c
SRCS+=	pad_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
