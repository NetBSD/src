# $NetBSD: Makefile,v 1.2 2010/12/09 05:19:02 riz Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/resize_ffs

.for name in t_grow t_shrink
TESTS_SH+=	${name}
TESTS_SH_SRC_${name}=	common.sh ${name}.sh
.endfor

FILESDIR=	${TESTSDIR}
FILES=		testdata.tar.gz.base64
FILES+=		testdata.md5

.include <bsd.test.mk>
