#	$NetBSD: Makefile,v 1.3 1997/05/26 03:59:53 cjs Exp $

PROG=	rpc.lockd
SRCS=	nlm_prot_svc.c lockd.c lock_proc.c
MAN=	rpc.lockd.8
MLINKS=	rpc.lockd.8 lockd.8

CFLAGS+= -I.

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

CLEANFILES= nlm_prot_svc.c nlm_prot.x nlm_prot.h test

.include <bsd.own.mk>	# for BUILDDIR

nlm_prot_svc.c: ${BUILDDIR}/usr/include/rpcsvc/nlm_prot.x
	rm -f nlm_prot.x
	ln -s ${BUILDDIR}/usr/include/rpcsvc/nlm_prot.x .
	rm -f nlm_prot.h
	ln -s ${BUILDDIR}/usr/include/rpcsvc/nlm_prot.h .
	rpcgen -m -o $@ nlm_prot.x

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc

.include <bsd.prog.mk>
