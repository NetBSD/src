#	$NetBSD: Makefile,v 1.10 2002/01/31 15:21:09 tv Exp $

PROG=	rpc.statd
SRCS=	sm_inter_svc.c statd.c stat_proc.c
MAN=	rpc.statd.8
MLINKS=	rpc.statd.8 statd.8

CPPFLAGS+= -I.

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

CLEANFILES+= sm_inter_svc.c sm_inter.x sm_inter.h test

.include <bsd.prog.mk>

sm_inter.x: ${DESTDIR}/usr/include/rpcsvc/sm_inter.x
	rm -f ${.TARGET}
	ln -s ${.ALLSRC} ${.TARGET}

sm_inter.h: ${DESTDIR}/usr/include/rpcsvc/sm_inter.h
	rm -f ${.TARGET}
	ln -s ${.ALLSRC} ${.TARGET}

sm_inter_svc.c: sm_inter.x sm_inter.h
	${RPCGEN} -C -L -m -o ${.TARGET} sm_inter.x

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc
