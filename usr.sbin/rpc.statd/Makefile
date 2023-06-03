#	$NetBSD: Makefile,v 1.13 2023/06/03 09:09:23 lukem Exp $

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

COPTS.sm_inter_svc.c+=	${CC_WNO_CAST_FUNCTION_TYPE} ${CC_WNO_FORMAT_TRUNCATION}
COPTS.statd.c+=		${CC_WNO_CAST_FUNCTION_TYPE} ${CC_WNO_FORMAT_TRUNCATION} ${CC_WNO_STRINGOP_TRUNCATION}
COPTS.stat_proc.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.rpcb_svc_4.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.rpc.mk>
.include <bsd.prog.mk>

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc
