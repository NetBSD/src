#	$NetBSD: Makefile,v 1.3 2005/02/05 13:37:39 xtraeme Exp $

SRCS=	scsictl.c scsi_sense.c scsi_subr.c

PROG=	scsictl
MAN=	scsictl.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

WARNS=	3

.include <bsd.prog.mk>
