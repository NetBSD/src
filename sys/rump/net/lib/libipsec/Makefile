#	$NetBSD: Makefile,v 1.1.2.2 2018/02/11 21:17:35 snj Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netipsec

LIB=	rumpnet_ipsec
COMMENT= ipsec interface

IOCONF=	IPSEC.ioconf
SRCS=	if_ipsec.c ipsecif.c

SRCS+=	ipsec_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
