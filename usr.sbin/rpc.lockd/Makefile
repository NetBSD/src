#	$NetBSD: Makefile,v 1.9 2000/06/12 01:57:26 mycroft Exp $

PROG=	rpc.lockd
SRCS=	nlm_prot_svc.c lockd.c lock_proc.c lockd_lock.c
MAN=	rpc.lockd.8
MLINKS=	rpc.lockd.8 lockd.8

CPPFLAGS+= -I.

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

CLEANFILES= nlm_prot_svc.c nlm_prot.x nlm_prot.h test

nlm_prot.x: ${DESTDIR}/usr/include/rpcsvc/nlm_prot.x
	rm -f ${.TARGET}
	ln -s ${.ALLSRC} ${.TARGET}

nlm_prot.h: ${DESTDIR}/usr/include/rpcsvc/nlm_prot.h
	rm -f ${.TARGET}
	ln -s ${.ALLSRC} ${.TARGET}

nlm_prot_svc.c: nlm_prot.x nlm_prot.h
	rpcgen -C -L -m -o ${.TARGET} nlm_prot.x

test: ${.CURDIR}/test.c
	cc -o test ${.CURDIR}/test.c -lrpcsvc

.include <bsd.prog.mk>
