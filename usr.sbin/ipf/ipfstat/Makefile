#	$NetBSD: Makefile,v 1.1.1.2 1997/03/29 02:49:43 darrenr Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CFLAGS+=        -I${.CURDIR}/../../sbin/ipf

.PATH: ${.CURDIR}/../../sbin/ipf
.include <bsd.prog.mk>
