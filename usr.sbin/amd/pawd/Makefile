#	$NetBSD: Makefile,v 1.1.1.1 1998/08/08 22:05:39 christos Exp $

PROG= pawd
SRCS=	pawd.c 
SRCS+= amq_clnt.c amq_xdr.c
MAN+= pawd.1
CFLAGS+=-I${.CURDIR}/../amq
.PATH.c: ${.CURDIR}/../amq

.include <bsd.prog.mk>
