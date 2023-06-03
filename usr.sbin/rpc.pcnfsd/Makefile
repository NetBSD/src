#	$NetBSD: Makefile,v 1.25 2023/06/03 09:09:23 lukem Exp $

.include <bsd.own.mk>

PROG=	rpc.pcnfsd
MAN=	rpc.pcnfsd.8
BUILDSYMLINKS+=	pcnfsd.8 rpc.pcnfsd.8
MLINKS= rpc.pcnfsd.8 pcnfsd.8
CPPFLAGS += -DUSER_CACHE -DWTMP -I. -DSUPPORT_UTMP -DSUPPORT_UTMPX

.if (${USE_YP} != "no")
CPPFLAGS+=-DUSE_YP
.endif

SRCS=	pcnfsd_v1.c pcnfsd_v2.c pcnfsd_misc.c \
	pcnfsd_cache.c pcnfsd_print.c pcnfsd_svc.c pcnfsd_xdr.c

DPADD=	${LIBRPCSVC} ${LIBCRYPT} ${LIBUTIL}
LDADD=	-lrpcsvc -lcrypt -lutil

RPC_INCS=	pcnfsd.h
RPC_XDRFILES=	pcnfsd_xdr.c
RPC_SVCFILES=	pcnfsd_svc.c
RPC_SVCCLASS=	udp tcp
RPC_SVCFLAGS=	-I

COPTS.pcnfsd_v1.c+=	-Wno-pointer-sign
COPTS.pcnfsd_v2.c+=	-Wno-pointer-sign

.include <bsd.rpc.mk>

CLEANFILES += clnt.pcnfsd pcnfsd_test.o pcnfsd_test.ln pcnfsd_clnt.*

COPTS.pcnfsd_misc.c+=	${CC_WNO_FORMAT_TRUNCATION} ${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.pcnfsd_print.c+=	${CC_WNO_FORMAT_TRUNCATION} ${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.pcnfsd_svc.c+=	${CC_WNO_FORMAT_TRUNCATION} ${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
