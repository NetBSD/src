#	$NetBSD: Makefile,v 1.9 2001/01/16 02:42:40 cgd Exp $

PROG=	rpc.pcnfsd
MAN=	pcnfsd.8
MLINKS= pcnfsd.8 rpc.pcnfsd.8
CPPFLAGS += -DUSER_CACHE -DWTMP -DUSE_YP -I${.OBJDIR}

SRCS=	pcnfsd_v1.c pcnfsd_v2.c pcnfsd_misc.c \
	pcnfsd_cache.c pcnfsd_print.c pcnfsd_svc.c pcnfsd_xdr.c

DPADD=	${LIBRPCSVC} ${LIBCRYPT}
LDADD=	-lrpcsvc -lcrypt

.ifnmake getrpcgen
XRPCGEN != (cd ${.CURDIR} && ${MAKE} getrpcgen)
.endif

getrpcgen:
	@set -- X `type ${RPCGEN}` && shift `expr $$# - 1` && echo "$$1"

pcnfsd_svc.c: pcnfsd.x ${XRPCGEN}
	${RPCGEN} -C -s udp -s tcp -I ${.CURDIR}/pcnfsd.x -o $@

pcnfsd_xdr.c: pcnfsd.x ${XRPCGEN}
	${RPCGEN} -C -c ${.CURDIR}/pcnfsd.x -o $@

pcnfsd.h: pcnfsd.x ${XRPCGEN}
	${RPCGEN} -C -h ${.CURDIR}/pcnfsd.x -o $@

CLEANFILES += pcnfsd_svc.c pcnfsd_xdr.c pcnfsd.h
CLEANFILES += clnt.pcnfsd pcnfsd_test.o pcnfsd_test.ln pcnfsd_clnt.*

.include <bsd.prog.mk>

.depend ${OBJS}: pcnfsd.h
