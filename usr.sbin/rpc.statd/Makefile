#	$NetBSD: Makefile,v 1.3 1997/05/26 04:00:02 cjs Exp $

PROG=	rpc.statd
SRCS=	sm_inter_svc.c statd.c stat_proc.c
MAN=	rpc.statd.8
MLINKS=	rpc.statd.8 statd.8

CFLAGS+= -I.

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

CLEANFILES+= sm_inter_svc.c sm_inter.x sm_inter.h test

.include <bsd.own.mk>	# for BUILDDIR

sm_inter_svc.c: ${RPCSRC} ${BUILDDIR}/usr/include/rpcsvc/sm_inter.x
	rm -f sm_inter.x
	ln -s ${BUILDDIR}/usr/include/rpcsvc/sm_inter.x .
	rm -f sm_inter.h
	ln -s ${BUILDDIR}/usr/include/rpcsvc/sm_inter.h .
	rpcgen -m -o $@ sm_inter.x

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc

.include <bsd.prog.mk>
