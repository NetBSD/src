#	$NetBSD: Makefile,v 1.1.1.7 1997/10/30 05:27:48 mrg Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CPPFLAGS+=-I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
