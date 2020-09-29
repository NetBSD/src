#	$NetBSD: Makefile,v 1.1 2020/09/29 18:33:23 roy Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_vether
COMMENT=virtual Ethernet interface

IOCONF=	VETHER.ioconf
SRCS=	if_vether.c

SRCS+=	vether_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
