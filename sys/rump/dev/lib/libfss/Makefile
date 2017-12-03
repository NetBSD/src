#	$NetBSD: Makefile,v 1.1.24.2 2017/12/03 11:39:07 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_fss
COMMENT=File system snapshot device

IOCONF=	FSS.ioconf
SRCS=   fss.c

SRCS+=	fss_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
