#	$NetBSD: Makefile,v 1.21 2002/03/22 18:10:25 thorpej Exp $

.include <bsd.own.mk>

PROG=	rpc.bootparamd
SRCS=	bootparamd.c bootparam_prot_svc.c
MAN=	bootparams.5 rpc.bootparamd.8
MLINKS=	rpc.bootparamd.8 bootparamd.8

.if (${USE_YP} != "no")
CPPFLAGS+=-DYP
.endif

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

CLEANFILES += bootparam_prot_svc.c bootparam_prot.x bootparam_prot.h

.include <bsd.prog.mk>

bootparam_prot.x: ${DESTDIR}/usr/include/rpcsvc/bootparam_prot.x
	rm -f ${.TARGET}
	ln -s ${.ALLSRC} ${.TARGET}

bootparam_prot.h: ${DESTDIR}/usr/include/rpcsvc/bootparam_prot.h
	rm -f ${.TARGET}
	ln -s ${.ALLSRC} ${.TARGET}

bootparam_prot_svc.c: bootparam_prot.x bootparam_prot.h
	${RPCGEN} -C -L -m -o ${.TARGET} bootparam_prot.x
