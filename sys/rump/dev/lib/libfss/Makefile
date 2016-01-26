#	$NetBSD: Makefile,v 1.5 2016/01/26 23:12:15 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss
COMMENT=File system snapshot device

IOCONF=	FSS.ioconf
SRCS=   fss.c

SRCS+=	fss_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
