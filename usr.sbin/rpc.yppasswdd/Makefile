#	$NetBSD: Makefile,v 1.5 1998/07/29 01:19:00 simonb Exp $

PROG=	rpc.yppasswdd
SRCS=	rpc.yppasswdd.c yppasswdd_xdr.c yppasswdd_mkpw.c
MAN=	rpc.yppasswdd.8

MLINKS= rpc.yppasswdd.8 yppasswdd.8
DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
