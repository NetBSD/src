#	$NetBSD: Makefile,v 1.3.12.1 2014/08/20 00:04:52 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern/h_server

TESTS_C=	h_simpleserver

ATFFILE=	no

LDADD+=	-lrump -lrumpuser -lrump -lpthread

WARNS=	4
NOMAN=

KERNSPACE != cd ${.CURDIR}/../../kernspace && ${PRINTOBJDIR}
LDADD+= -L${KERNSPACE} -lkernspace

.include <bsd.test.mk>
