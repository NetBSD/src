# $NetBSD: Makefile,v 1.2 2009/04/22 15:23:05 lukem Exp $

.include <bsd.own.mk>

PROG=	mmcformat
MAN=	mmcformat.8
SRCS=	mmcformat.c uscsi_sense.c uscsi_subr.c

CPPFLAGS+= -DSCSI -DUSCSI_SCSIPI -I${NETBSDSRCDIR}/sys

.include <bsd.prog.mk>
