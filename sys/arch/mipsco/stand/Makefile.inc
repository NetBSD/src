#	$NetBSD: Makefile.inc,v 1.4.156.1 2017/12/03 11:36:29 jdolecek Exp $

NOMAN=		# defined
NOPIE=		# defined

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
