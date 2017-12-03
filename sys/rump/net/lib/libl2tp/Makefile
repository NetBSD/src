#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:17 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netinet	\
	${.CURDIR}/../../../../netinet6

LIB=	rumpnet_l2tp
COMMENT= L2TPv3 interface

IOCONF=	L2TP.ioconf
SRCS=	if_l2tp.c in_l2tp.c in6_l2tp.c

SRCS+=	l2tp_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
