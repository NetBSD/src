#	$NetBSD: Makefile,v 1.2 1997/03/11 04:13:04 scottr Exp $

PROG=	rpc.lockd
SRCS=	nlm_prot_svc.c lockd.c lock_proc.c
MAN=	rpc.lockd.8
MLINKS=	rpc.lockd.8 lockd.8

CFLAGS+= -I.

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

CLEANFILES= nlm_prot_svc.c nlm_prot.x nlm_prot.h test

nlm_prot_svc.c: ${DESTDIR}/usr/include/rpcsvc/nlm_prot.x
	rm -f nlm_prot.x
	ln -s ${DESTDIR}/usr/include/rpcsvc/nlm_prot.x .
	rm -f nlm_prot.h
	ln -s ${DESTDIR}/usr/include/rpcsvc/nlm_prot.h .
	rpcgen -m -o $@ nlm_prot.x

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc

.include <bsd.prog.mk>
