#	$NetBSD: Makefile,v 1.11 2003/01/05 19:24:08 sommerfeld Exp $

PROG=	rpc.statd
SRCS=	sm_inter_svc.c statd.c stat_proc.c
MAN=	rpc.statd.8
MLINKS=	rpc.statd.8 statd.8

CPPFLAGS+= -I. -I${DESTDIR}/usr/include/rpcsvc/

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

CLEANFILES+=	test

RPC_SVCFILES=	sm_inter_svc.c
RPC_SVCFLAGS=	-L -m
RPC_XDIR=	${DESTDIR}/usr/include/rpcsvc/

.include <bsd.rpc.mk>
.include <bsd.prog.mk>

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc
