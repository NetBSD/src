#	$NetBSD: Makefile,v 1.7 1997/10/25 06:58:27 lukem Exp $
# from: $Id: Makefile,v 8.2 1995/12/31 23:28:01 vixie Exp 

.PATH:	${.CURDIR}/../named \
	${.CURDIR}/../man

PROG=	named-xfer
SRCS=	named-xfer.c db_glue.c storage.c version.c
CPPFLAGS+=	-I${.CURDIR}/../named
MAN=	named-xfer.8
BINDIR=	/usr/libexec

.include "../../Makefile.inc"
.include <bsd.prog.mk>
