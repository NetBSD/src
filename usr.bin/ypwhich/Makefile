#	$NetBSD: Makefile,v 1.12 2009/06/20 19:27:26 christos Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?=yes	# network client

.PATH.c: ${.CURDIR}/../ypcat
CPPFLAGS+=-I${.CURDIR}/../ypcat
PROG=	ypwhich
SRCS=	ypwhich.c ypalias_init.c

.include <bsd.prog.mk>
