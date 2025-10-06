#	$NetBSD: Makefile,v 1.1 2025/10/06 13:16:44 riastradh Exp $
#

TESTSDIR=	${TESTSBASE}/lib/libpthread/weak

TESTS_C+=	t_pthread_weak_nothread
TESTS_C+=	t_pthread_weak_threaded

CPPFLAGS+=	-I${.CURDIR}/../lib

.include <bsd.own.mk>		# PRINTOBJDIR

.if !defined(H_PTHREAD_WEAK_OBJDIR)
H_PTHREAD_WEAK_OBJDIR!=	cd ../lib && ${PRINTOBJDIR}
.MAKEOVERRIDES+=	H_PTHREAD_WEAK_OBJDIR
.endif

LDADD+=		-L${H_PTHREAD_WEAK_OBJDIR}
LDADD+=		-Wl,-rpath,${TESTSBASE}/lib/libpthread/weak
LDADD+=		-lh_pthread_weak
LDADD.t_pthread_weak_threaded+=	-lpthread

.include <bsd.test.mk>
