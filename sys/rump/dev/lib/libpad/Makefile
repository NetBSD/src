#	$NetBSD: Makefile,v 1.1.2.2 2010/05/30 05:18:04 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pad

LIB=	rumpdev_pad

SRCS=	pad.c padvol.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
