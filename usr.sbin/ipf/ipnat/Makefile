#	$NetBSD: Makefile,v 1.2 1997/03/29 04:28:43 darrenr Exp $

PROG=	ipnat
SRCS=	kmem.c ipnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
CFLAGS+=	-DIPL_NAME=\"/dev/ipl\" -I${.CURDIR}/../ipfstat -I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../../../sys

.PATH: ${.CURDIR}/../../../sbin/ipf ${.CURDIR}/../ipfstat
.include <bsd.prog.mk>
