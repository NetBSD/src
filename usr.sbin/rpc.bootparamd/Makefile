#	$NetBSD: Makefile,v 1.19 2002/01/31 15:16:21 tv Exp $

PROG=	rpc.bootparamd
SRCS=	bootparamd.c bootparam_prot_svc.c
MAN=	bootparams.5 rpc.bootparamd.8
MLINKS=	rpc.bootparamd.8 bootparamd.8
CPPFLAGS+=-DYP

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

.include <bsd.prog.mk>

bootparam_prot.x: ${DESTDIR}/usr/include/rpcsvc/bootparam_prot.x
	rm -f ${.TARGET}
	ln -s ${.ALLSRC} ${.TARGET}

bootparam_prot.h: ${DESTDIR}/usr/include/rpcsvc/bootparam_prot.h
	rm -f ${.TARGET}
	ln -s ${.ALLSRC} ${.TARGET}

bootparam_prot_svc.c: bootparam_prot.x bootparam_prot.h
	${RPCGEN} -C -L -m -o ${.TARGET} bootparam_prot.x

CLEANFILES += bootparam_prot_svc.c bootparam_prot.x bootparam_prot.h
