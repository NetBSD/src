#	$NetBSD: Makefile,v 1.1.1.1 1997/01/05 13:09:04 mrg Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CFLAGS+=	-I${.CURDIR}/../../../sbin/ipf

.PATH: ${.CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
