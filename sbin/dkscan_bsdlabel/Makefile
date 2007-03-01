#	$NetBSD: Makefile,v 1.1 2007/03/01 22:01:29 martin Exp $

PROG=	dkscan_bsdlabel
MAN=	dkscan_bsdlabel.8

COPTS+=	-I ../../sys/dev/dkwedge
SRCS=	dkscan_bsdlabel.c dkscan_util.c

LDADD+=	-lutil

.include <bsd.prog.mk>
