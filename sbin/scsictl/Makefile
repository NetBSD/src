#	$NetBSD: Makefile,v 1.2 1998/10/18 01:14:28 thorpej Exp $

SRCS=	scsictl.c scsi_sense.c scsi_subr.c

PROG=	scsictl
MAN=	scsictl.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
