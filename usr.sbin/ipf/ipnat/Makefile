#	$NetBSD: Makefile,v 1.1.1.2 1997/03/29 02:50:04 darrenr Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
CFLAGS+=	-I${.CURDIR}/../../sbin/ipf -I${.CURDIR}/../ipfstat

.PATH: ${.CURDIR}/../../sbin/ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
