#	$NetBSD: Makefile,v 1.6.46.1 2020/04/13 08:05:58 martin Exp $

PROG=	rpc.yppasswdd
SRCS=	rpc.yppasswdd.c yppasswdd_mkpw.c
MAN=	rpc.yppasswdd.8

MLINKS= rpc.yppasswdd.8 yppasswdd.8
DPADD+= ${LIBRPCSVC} ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lrpcsvc -lcrypt -lutil

COPTS.rpc.yppasswdd.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
