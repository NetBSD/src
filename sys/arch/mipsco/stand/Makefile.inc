#	$NetBSD: Makefile.inc,v 1.1.6.1 2002/01/10 19:46:23 thorpej Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
