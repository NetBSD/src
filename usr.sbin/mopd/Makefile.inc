#	$NetBSD: Makefile.inc,v 1.3 1997/04/17 21:08:57 christos Exp $

BINDIR?=	/usr/sbin

CFLAGS+= -I${.CURDIR}/.. -I${.CURDIR}/../common
.PATH:	${.CURDIR}/../common
