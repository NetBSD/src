#	$NetBSD: Makefile,v 1.1.1.1 1997/01/05 13:09:04 mrg Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
CFLAGS+=	-I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../ipfstat

.PATH: ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
