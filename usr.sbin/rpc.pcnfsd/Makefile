#	$NetBSD: Makefile,v 1.7 2000/07/25 06:29:46 cgd Exp $

PROG=	rpc.pcnfsd
MAN=	pcnfsd.8
MLINKS= pcnfsd.8 rpc.pcnfsd.8
CPPFLAGS += -DUSER_CACHE -DWTMP -DUSE_YP -I${.OBJDIR}

SRCS=	pcnfsd_v1.c pcnfsd_v2.c pcnfsd_misc.c \
	pcnfsd_cache.c pcnfsd_print.c pcnfsd_svc.c pcnfsd_xdr.c

DPADD=	${LIBRPCSVC} ${LIBCRYPT}
LDADD=	-lrpcsvc -lcrypt

pcnfsd_svc.c: pcnfsd.x
	${RPCGEN} -C -s udp -s tcp -I ${.CURDIR}/pcnfsd.x -o $@

pcnfsd_xdr.c: pcnfsd.x
	${RPCGEN} -C -c ${.CURDIR}/pcnfsd.x -o $@

pcnfsd.h: pcnfsd.x
	${RPCGEN} -C -h ${.CURDIR}/pcnfsd.x -o $@

CLEANFILES += pcnfsd_svc.c pcnfsd_xdr.c pcnfsd.h
CLEANFILES += clnt.pcnfsd pcnfsd_test.o pcnfsd_test.ln pcnfsd_clnt.*

.include <bsd.prog.mk>

.depend ${OBJS}: pcnfsd.h
