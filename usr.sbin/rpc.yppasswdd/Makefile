#	$NetBSD: Makefile,v 1.7 2019/10/13 07:28:20 mrg Exp $

PROG=	rpc.yppasswdd
SRCS=	rpc.yppasswdd.c yppasswdd_mkpw.c
MAN=	rpc.yppasswdd.8

MLINKS= rpc.yppasswdd.8 yppasswdd.8
DPADD+= ${LIBRPCSVC} ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lrpcsvc -lcrypt -lutil

COPTS.rpc.yppasswdd.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
