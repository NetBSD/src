#	$NetBSD: Makefile,v 1.4 2005/06/27 01:00:06 christos Exp $

SRCS=	scsictl.c scsi_sense.c scsi_subr.c

PROG=	scsictl
MAN=	scsictl.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
