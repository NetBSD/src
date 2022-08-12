#	$NetBSD: Makefile,v 1.13 2022/08/12 10:49:17 riastradh Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/cgd
FILES=		paramsfile
FILESDIR=	${TESTSDIR}

TESTS_SH+=	t_cgd
TESTS_SH+=	t_cgdconfig

SCRIPTSDIR=	${TESTSDIR}
SCRIPTS+=	h_countkey.sh

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
