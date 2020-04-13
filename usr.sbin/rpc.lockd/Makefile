#	$NetBSD: Makefile,v 1.16.46.1 2020/04/13 08:05:57 martin Exp $

USE_FORT?= yes	# network server

PROG=		rpc.lockd
SRCS=		nlm_prot_svc.c lockd.c lock_proc.c lockd_lock.c
MAN=		rpc.lockd.8
MLINKS=		rpc.lockd.8 lockd.8

CPPFLAGS+=	-I. -I${DESTDIR}/usr/include/rpcsvc

DPADD=		${LIBRPCSVC} ${LIBUTIL}
LDADD=		-lrpcsvc -lutil

RPC_SVCFLAGS=	-L -m
RPC_SVCFILES=	nlm_prot_svc.c
RPC_XDIR=	${DESTDIR}/usr/include/rpcsvc/

CLEANFILES=	test

.include <bsd.rpc.mk>

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc

COPTS.lockd_lock.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.lock_proc.c+=	${GCC_NO_CAST_FUNCTION_TYPE}
COPTS.nlm_prot_svc.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
