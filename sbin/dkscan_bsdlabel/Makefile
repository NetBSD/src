#	$NetBSD: Makefile,v 1.3 2016/01/23 21:22:47 christos Exp $

PROG=	dkscan_bsdlabel
MAN=	dkscan_bsdlabel.8

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/dev/dkwedge -D_KERNTYPES
SRCS=	dkscan_bsdlabel.c dkscan_util.c

LDADD+=	-lutil

.include <bsd.prog.mk>
