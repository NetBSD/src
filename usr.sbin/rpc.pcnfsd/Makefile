#	$NetBSD: Makefile,v 1.12.2.1 2002/11/30 15:20:30 he Exp $

.include <bsd.own.mk>

PROG=	rpc.pcnfsd
MAN=	pcnfsd.8
MLINKS= pcnfsd.8 rpc.pcnfsd.8
CPPFLAGS += -DUSER_CACHE -DWTMP -I.

.if (${USE_YP} != "no")
CPPFLAGS+=-DUSE_YP
.endif

SRCS=	pcnfsd_v1.c pcnfsd_v2.c pcnfsd_misc.c \
	pcnfsd_cache.c pcnfsd_print.c pcnfsd_svc.c pcnfsd_xdr.c

DPADD=	${LIBRPCSVC} ${LIBCRYPT}
LDADD=	-lrpcsvc -lcrypt

# Resolve rpcgen's path, to allow it to be a dependency.
_RPCGEN:=	${RPCGEN:M*rpcgen}
.if ${_RPCGEN:M/*} == ""
_RPCGEN!=	type ${RPCGEN} | awk '{print $$NF}'
.endif

pcnfsd_svc.c: pcnfsd.x ${_RPCGEN}
	${RPCGEN} -C -s udp -s tcp -I ${.CURDIR}/pcnfsd.x -o $@

pcnfsd_xdr.c: pcnfsd.x ${_RPCGEN}
	${RPCGEN} -C -c ${.CURDIR}/pcnfsd.x -o $@

pcnfsd.h: pcnfsd.x ${_RPCGEN}
	${RPCGEN} -C -h ${.CURDIR}/pcnfsd.x -o $@

CLEANFILES += pcnfsd_svc.c pcnfsd_xdr.c pcnfsd.h
CLEANFILES += clnt.pcnfsd pcnfsd_test.o pcnfsd_test.ln pcnfsd_clnt.*

.include <bsd.prog.mk>

.depend ${OBJS}: pcnfsd.h
