# $NetBSD: Makefile,v 1.1 2008/05/14 16:49:48 reinoud Exp $

.include <bsd.own.mk>

WARNS=	4
PROG=	mmcformat
MAN=	mmcformat.8
SRCS=	mmcformat.c uscsi_sense.c uscsi_subr.c

CPPFLAGS+= -DSCSI -DUSCSI_SCSIPI -I${NETBSDSRCDIR}/sys

.include <bsd.prog.mk>
