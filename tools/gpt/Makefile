#	$NetBSD: Makefile,v 1.2 2014/09/29 21:05:11 christos Exp $

HOSTPROGNAME=	nbgpt
HOST_SRCDIR=	sbin/gpt
HOST_CPPFLAGS+=	-I${.CURDIR}
HOST_SRCS+= opendisk.c

NOMAN=	# defined
.PATH.c: ${.CURDIR}/../../lib/libutil

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
