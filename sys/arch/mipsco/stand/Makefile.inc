#	$NetBSD: Makefile.inc,v 1.1.2.2 2000/11/20 20:14:12 bouyer Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1
MKMAN=		no

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
