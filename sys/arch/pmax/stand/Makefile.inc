# $NetBSD: Makefile.inc,v 1.14 2001/12/12 01:49:49 tv Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1
NOMAN=		# defined

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
