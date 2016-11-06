#	$NetBSD: Makefile,v 1.4 2016/11/06 21:29:54 alnsn Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/cgd
FILES=		paramsfile
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_cgd

.if ${MKCRYPTO} != "no"
TESTS_C=	t_cgd_aes

WARNS=	4
CPPFLAGS+=	-D_KERNTYPES
LDADD+=	-lrumpdev -lrumpdev_disk -lrumpdev_cgd -lrumpkern_crypto -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser
LDADD+=	-lrump -lutil
LDADD+=	-lpthread
.endif

.include <bsd.test.mk>
