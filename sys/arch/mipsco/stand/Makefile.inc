#	$NetBSD: Makefile.inc,v 1.1 2000/09/18 11:40:46 wdk Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1
MKMAN=		no

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
