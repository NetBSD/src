#	$NetBSD: Makefile,v 1.16 1999/06/06 02:47:47 thorpej Exp $

PROG=	rpc.bootparamd
SRCS=	bootparamd.c bootparam_prot_svc.c
MAN=	bootparams.5 rpc.bootparamd.8
MLINKS=	rpc.bootparamd.8 bootparamd.8
CPPFLAGS+=-DYP

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

bootparam_prot_svc.c: ${DESTDIR}/usr/include/rpcsvc/bootparam_prot.x
	rm -f bootparam_prot.x
	ln -s ${DESTDIR}/usr/include/rpcsvc/bootparam_prot.x .
	rm -f bootparam_prot.h
	ln -s ${DESTDIR}/usr/include/rpcsvc/bootparam_prot.h .
	rpcgen -C -L -m -o ${.TARGET} bootparam_prot.x

CLEANFILES += bootparam_prot_svc.c bootparam_prot.x bootparam_prot.h

.include <bsd.prog.mk>
