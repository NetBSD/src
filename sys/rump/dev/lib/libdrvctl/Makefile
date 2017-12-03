#	$NetBSD: Makefile,v 1.1.20.2 2017/12/03 11:39:07 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpdev_drvctl
COMMENT=/dev/drvctl driver

IOCONF=	DRVCTL.ioconf

SRCS=	kern_drvctl.c
SRCS+=	drvctl_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
