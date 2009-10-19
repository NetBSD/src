#	$NetBSD: Makefile.inc,v 1.2 2009/10/19 02:31:26 christos Exp $

BINDIR?=	/usr/sbin
WARNS?=		1

LDADD+=-lz
DPADD+=${LIBZ}
