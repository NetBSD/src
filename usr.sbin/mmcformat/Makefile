# $NetBSD: Makefile,v 1.1.10.1 2009/05/13 19:20:29 jym Exp $

.include <bsd.own.mk>

PROG=	mmcformat
MAN=	mmcformat.8
SRCS=	mmcformat.c uscsi_sense.c uscsi_subr.c

CPPFLAGS+= -DSCSI -DUSCSI_SCSIPI -I${NETBSDSRCDIR}/sys

.include <bsd.prog.mk>
