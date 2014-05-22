#	$NetBSD: Makefile,v 1.1.10.1 2014/05/22 11:41:11 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpdev_drvctl
IOCONF=	DRVCTL.ioconf

SRCS=	kern_drvctl.c
SRCS+=	drvctl_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
