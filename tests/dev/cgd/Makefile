#	$NetBSD: Makefile,v 1.8 2017/05/21 15:28:42 riastradh Exp $
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
LDADD+=	-lrumpdev -lrumpdev_disk -lrumpdev_cgd -lrumpkern_crypto -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser
LDADD+=	-lrump -lutil
LDADD+=	-lpthread
.endif

.include <bsd.test.mk>
