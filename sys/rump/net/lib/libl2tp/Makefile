#	$NetBSD: Makefile,v 1.1 2017/02/16 08:39:10 knakahara Exp $
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
