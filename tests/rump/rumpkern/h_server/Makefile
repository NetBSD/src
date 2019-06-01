#	$NetBSD: Makefile,v 1.6 2019/06/01 06:59:17 kre Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern/h_server

TESTS_C=	h_simpleserver

ATFFILE=	no

LDADD+=	-lrumpkern_sysproxy -lrump -lrumpuser -lrump -lrumpvfs -lpthread

WARNS=	4
NOMAN=

KERNSPACE != cd ${.CURDIR}/../../kernspace && ${PRINTOBJDIR}
LDADD+= -L${KERNSPACE} -lkernspace

.include <bsd.test.mk>
