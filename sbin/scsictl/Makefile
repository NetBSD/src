#	$NetBSD: Makefile,v 1.1 1998/10/15 21:44:39 thorpej Exp $

SRCS=	scsictl.c scsi_sense.c scsi_subr.c

PROG=	scsictl
NOMAN=

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
