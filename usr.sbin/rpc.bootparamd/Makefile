#	$Id: Makefile,v 1.9 1995/04/12 05:19:51 jtc Exp $

CFLAGS+=-DYP

PROG=	rpc.bootparamd
SRCS=	bootparamd.c bootparam_prot_svc.c
MAN=	bootparams.5 rpc.bootparamd.8
MLINKS=	rpc.bootparamd.8 bootparamd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

bootparam_prot_svc.c: /usr/include/rpcsvc/bootparam_prot.x
	rm -f bootparam_prot.x
	ln -s /usr/include/rpcsvc/bootparam_prot.x .
	rm -f bootparam_prot.h
	ln -s /usr/include/rpcsvc/bootparam_prot.h .
	rpcgen -m -o $@ bootparam_prot.x

CLEANFILES += bootparam_prot_svc.c bootparam_prot.x bootparam_prot.h

.include <bsd.prog.mk>
