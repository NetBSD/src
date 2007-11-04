#	$NetBSD: Makefile,v 1.15 2007/11/04 23:12:50 christos Exp $

USE_FORT?= yes	# network server
WARNS=4

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

.include <bsd.prog.mk>
