#	$NetBSD: Makefile,v 1.1.24.1 2014/05/18 17:46:15 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpdev_drvctl
IOCONF=	DRVCTL.ioconf

SRCS=	kern_drvctl.c
SRCS+=	drvctl_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
