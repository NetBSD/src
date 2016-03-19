#	$NetBSD: Makefile,v 1.2.6.3 2016/03/19 11:30:34 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss
COMMENT=File system snapshot device

IOCONF=	FSS.ioconf
SRCS=   fss.c

SRCS+=	fss_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
