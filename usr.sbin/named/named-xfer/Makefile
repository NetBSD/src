#	$NetBSD: Makefile,v 1.5 1997/05/06 20:46:30 gwr Exp $
# from: $Id: Makefile,v 8.2 1995/12/31 23:28:01 vixie Exp 

.PATH.c: ${.CURDIR}/../named \
	${.CURDIR}/../man

PROG=	named-xfer
SRCS=	named-xfer.c db_glue.c storage.c version.c
CFLAGS+=	-I${.CURDIR}/../named
MAN=	named-xfer.8
BINDIR=	/usr/libexec

.include "../../Makefile.inc"
.include <bsd.prog.mk>
