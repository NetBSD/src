#	$NetBSD: Makefile,v 1.1.1.11 1998/11/22 14:21:46 mrg Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CPPFLAGS+=-I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
