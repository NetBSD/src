#	$NetBSD: Makefile,v 1.1 2018/01/10 11:02:42 knakahara Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netipsec

LIB=	rumpnet_ipsec
COMMENT= ipsec interface

IOCONF=	IPSEC.ioconf
SRCS=	if_ipsec.c ipsecif.c

SRCS+=	ipsec_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
