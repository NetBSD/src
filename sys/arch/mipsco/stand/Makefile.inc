#	$NetBSD: Makefile.inc,v 1.4.176.1 2016/10/05 20:55:32 skrll Exp $

NOMAN=		# defined
NOPIE=		# defined

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
