#	$NetBSD: Makefile,v 1.3 1999/03/26 20:04:50 kim Exp $

BINDIR=	/usr/bin
PROG=	pawd
SRCS=	pawd.c 
SRCS+=	amq_clnt.c amq_xdr.c
MAN+=	pawd.1
CPPFLAGS+=-I${.CURDIR}/../amq
.PATH.c: ${.CURDIR}/../amq

.include <bsd.prog.mk>
