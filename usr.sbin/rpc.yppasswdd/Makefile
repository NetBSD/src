#	$NetBSD: Makefile,v 1.1.1.1 1996/08/09 10:19:49 thorpej Exp $

PROG=	rpc.yppasswdd
SRCS=	rpc.yppasswdd.c yppasswdd_xdr.c yppasswdd_mkpw.c
MAN=	rpc.yppasswdd.8
BINOWN= root
BINMODE=555
BINDIR=/usr/sbin
MLINKS= rpc.yppasswdd.8 yppasswdd.8
DPADD+= ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
