#	$NetBSD: Makefile,v 1.2 1997/03/29 04:28:28 darrenr Exp $

PROG=	ipfstat
SRCS=	kmem.c fils.c opt.c parse.c
MAN=	ipfstat.8
CFLAGS+=        -DIPL_NAME=\"/dev/ipl\" -I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../../../sys

.PATH: ${.CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
