# $NetBSD: Makefile,v 1.1.4.2 2008/09/24 04:54:48 wrstuden Exp $

.include <bsd.own.mk>

WARNS=	4
PROG=	mmcformat
MAN=	mmcformat.8
SRCS=	mmcformat.c uscsi_sense.c uscsi_subr.c

CPPFLAGS+= -DSCSI -DUSCSI_SCSIPI -I${NETBSDSRCDIR}/sys

.include <bsd.prog.mk>
