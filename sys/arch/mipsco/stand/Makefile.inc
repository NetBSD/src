#	$NetBSD: Makefile.inc,v 1.3.8.1 2002/08/31 13:45:25 gehenna Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
