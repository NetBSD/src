#	$Id: Makefile,v 1.3 1994/01/14 19:45:51 deraadt Exp $

CFLAGS+=-DYP

PROG=	rpc.bootparamd
SRCS=	bootparamd.c bootparam_prot_svc.c
MAN8=	rpc.bootparamd.0

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

bootparam_prot_svc.c: /usr/include/rpcsvc/bootparam_prot.x
	rm -f bootparamd_prot.x
	ln -s /usr/include/rpcsvc/bootparam_prot.x .
	rm -f bootparamd_prot.h
	ln -s /usr/include/rpcsvc/bootparam_prot.h .
	rpcgen -m -o $@ bootparam_prot.x

CLEANFILES += bootparam_prot_svc.c bootparam_prot.x bootparam_prot.h

.include <bsd.prog.mk>
