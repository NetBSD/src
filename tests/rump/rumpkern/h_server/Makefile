#	$NetBSD: Makefile,v 1.7 2020/03/01 18:08:16 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern/h_server

TESTS_C=	h_simpleserver

ATFFILE=	no

LDADD+=	-lrumpkern_sysproxy ${LIBRUMPBASE}

WARNS=	4
NOMAN=

KERNSPACE != cd ${.CURDIR}/../../kernspace && ${PRINTOBJDIR}
LDADD+= -L${KERNSPACE} -lkernspace

.include <bsd.test.mk>
