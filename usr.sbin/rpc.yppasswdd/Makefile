#	$NetBSD: Makefile,v 1.3 1997/07/18 07:47:26 thorpej Exp $

PROG=	rpc.yppasswdd
SRCS=	rpc.yppasswdd.c yppasswdd_xdr.c yppasswdd_mkpw.c
MAN=	rpc.yppasswdd.8
BINOWN= root
BINMODE=555
BINDIR=/usr/sbin
MLINKS= rpc.yppasswdd.8 yppasswdd.8
DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil
WARNS=1

.include <bsd.prog.mk>
