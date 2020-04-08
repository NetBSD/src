#	$NetBSD: Makefile,v 1.8.10.1 2020/04/08 14:09:05 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/cgd
FILES=		paramsfile
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_cgd

.if ${MKRUMP} != "no"
TESTS_C=	t_cgd_3des t_cgd_aes t_cgd_blowfish

WARNS=	4
CPPFLAGS+=	-D_KERNTYPES
LDADD+=	-lrumpdev -lrumpdev_disk -lrumpdev_cgd -lrumpkern_crypto ${LIBRUMPBASE}
LDADD+=	-lutil
.endif

.include <bsd.test.mk>
