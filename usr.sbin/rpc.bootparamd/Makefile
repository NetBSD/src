#	$NetBSD: Makefile,v 1.25 2007/05/28 12:06:39 tls Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network server

PROG=	rpc.bootparamd
SRCS=	bootparamd.c bootparam_prot_svc.c
MAN=	bootparams.5 rpc.bootparamd.8
MLINKS=	rpc.bootparamd.8 bootparamd.8

.if (${USE_YP} != "no")
CPPFLAGS+=-DYP
.endif

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

RPC_SVCFILES=	bootparam_prot_svc.c
RPC_SVCFLAGS=	-L -m
RPC_XDIR=	${DESTDIR}/usr/include/rpcsvc/

CPPFLAGS+=	-I${RPC_XDIR}

.include <bsd.rpc.mk>
.include <bsd.prog.mk>

test: test.c
	cc -o test ${.ALLSRC} -lrpcsvc
