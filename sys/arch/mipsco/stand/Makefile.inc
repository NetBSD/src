#	$NetBSD: Makefile.inc,v 1.4 2002/08/27 08:53:16 lukem Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
