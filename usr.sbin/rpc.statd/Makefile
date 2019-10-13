#	$NetBSD: Makefile,v 1.12 2019/10/13 07:28:20 mrg Exp $

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

COPTS.sm_inter_svc.c+=	${GCC_NO_CAST_FUNCTION_TYPE} ${GCC_NO_FORMAT_TRUNCATION}
COPTS.statd.c+=		${GCC_NO_CAST_FUNCTION_TYPE} ${GCC_NO_FORMAT_TRUNCATION} ${GCC_NO_STRINGOP_TRUNCATION}
COPTS.stat_proc.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.rpcb_svc_4.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.rpc.mk>
.include <bsd.prog.mk>

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc
