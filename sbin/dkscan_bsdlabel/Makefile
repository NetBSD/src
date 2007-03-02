#	$NetBSD: Makefile,v 1.2 2007/03/02 04:03:17 riz Exp $

PROG=	dkscan_bsdlabel
MAN=	dkscan_bsdlabel.8

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/dev/dkwedge
SRCS=	dkscan_bsdlabel.c dkscan_util.c

LDADD+=	-lutil

.include <bsd.prog.mk>
