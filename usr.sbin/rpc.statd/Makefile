#	$NetBSD: Makefile,v 1.4 1997/05/31 21:22:18 cjs Exp $

PROG=	rpc.statd
SRCS=	sm_inter_svc.c statd.c stat_proc.c
MAN=	rpc.statd.8
MLINKS=	rpc.statd.8 statd.8

CFLAGS+= -I.

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

CLEANFILES+= sm_inter_svc.c sm_inter.x sm_inter.h test

sm_inter_svc.c: ${RPCSRC} ${DESTDIR}/usr/include/rpcsvc/sm_inter.x
	rm -f sm_inter.x
	ln -s ${DESTDIR}/usr/include/rpcsvc/sm_inter.x .
	rm -f sm_inter.h
	ln -s ${DESTDIR}/usr/include/rpcsvc/sm_inter.h .
	rpcgen -m -o $@ sm_inter.x

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc

.include <bsd.prog.mk>
