#	$NetBSD: Makefile,v 1.12 1997/05/26 03:59:47 cjs Exp $

CFLAGS+=-DYP

PROG=	rpc.bootparamd
SRCS=	bootparamd.c bootparam_prot_svc.c
MAN=	bootparams.5 rpc.bootparamd.8
MLINKS=	rpc.bootparamd.8 bootparamd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.own.mk>	# for BUILDDIR

bootparam_prot_svc.c: ${BUILDDIR}/usr/include/rpcsvc/bootparam_prot.x
	rm -f bootparam_prot.x
	ln -s ${BUILDDIR}/usr/include/rpcsvc/bootparam_prot.x .
	rm -f bootparam_prot.h
	ln -s ${BUILDDIR}/usr/include/rpcsvc/bootparam_prot.h .
	rpcgen -m -o $@ bootparam_prot.x

CLEANFILES += bootparam_prot_svc.c bootparam_prot.x bootparam_prot.h

.include <bsd.prog.mk>
