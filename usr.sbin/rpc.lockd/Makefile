#	$NetBSD: Makefile,v 1.12 2000/08/07 16:23:31 thorpej Exp $

PROG=	rpc.lockd
SRCS=	nlm_prot_svc.c lockd.c lock_proc.c lockd_lock.c
MAN=	rpc.lockd.8
MLINKS=	rpc.lockd.8 lockd.8

CPPFLAGS+= -I. -I${DESTDIR}/usr/include/rpcsvc

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

CLEANFILES= nlm_prot_svc.c test

nlm_prot_svc.c: ${DESTDIR}/usr/include/rpcsvc/nlm_prot.x \
    ${DESTDIR}/usr/include/rpcsvc/nlm_prot.h
	${RPCGEN} -C -L -m -o ${.TARGET} \
	    ${DESTDIR}/usr/include/rpcsvc/nlm_prot.x

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc

.include <bsd.prog.mk>
