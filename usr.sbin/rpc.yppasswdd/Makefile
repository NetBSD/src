#	$NetBSD: Makefile,v 1.8 2023/06/03 09:09:23 lukem Exp $

PROG=	rpc.yppasswdd
SRCS=	rpc.yppasswdd.c yppasswdd_mkpw.c
MAN=	rpc.yppasswdd.8

MLINKS= rpc.yppasswdd.8 yppasswdd.8
DPADD+= ${LIBRPCSVC} ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lrpcsvc -lcrypt -lutil

COPTS.rpc.yppasswdd.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
