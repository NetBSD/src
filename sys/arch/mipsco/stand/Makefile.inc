#	$NetBSD: Makefile.inc,v 1.5 2016/09/08 14:40:52 christos Exp $

NOMAN=		# defined
NOPIE=		# defined

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
