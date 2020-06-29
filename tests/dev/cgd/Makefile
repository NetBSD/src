#	$NetBSD: Makefile,v 1.11 2020/06/29 23:44:01 riastradh Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/cgd
FILES=		paramsfile
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_cgd

.if ${MKRUMP} != "no"
TESTS_C+=	t_cgd_3des
TESTS_C+=	t_cgd_adiantum
TESTS_C+=	t_cgd_aes
TESTS_C+=	t_cgd_blowfish

WARNS=	4
CPPFLAGS+=	-D_KERNTYPES
LDADD+=	-lrumpdev -lrumpdev_disk -lrumpdev_cgd -lrumpkern_crypto ${LIBRUMPBASE}
LDADD+=	-lutil
.endif

.include <bsd.test.mk>
