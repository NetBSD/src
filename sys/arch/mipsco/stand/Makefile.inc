#	$NetBSD: Makefile.inc,v 1.1.6.2 2002/09/06 08:37:52 jdolecek Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
