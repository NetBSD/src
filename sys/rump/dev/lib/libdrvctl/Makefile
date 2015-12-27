#	$NetBSD: Makefile,v 1.2.6.1 2015/12/27 12:10:09 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpdev_drvctl
COMMENT=/dev/drvctl driver

IOCONF=	DRVCTL.ioconf

SRCS=	kern_drvctl.c
SRCS+=	drvctl_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
