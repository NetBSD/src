#	$NetBSD: Makefile,v 1.4 1997/10/18 06:42:56 lukem Exp $

PROG=	rpc.yppasswdd
SRCS=	rpc.yppasswdd.c yppasswdd_xdr.c yppasswdd_mkpw.c
MAN=	rpc.yppasswdd.8

BINOWN= root
BINMODE=555
BINDIR=/usr/sbin
MLINKS= rpc.yppasswdd.8 yppasswdd.8
DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
