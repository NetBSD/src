# $NetBSD: Makefile,v 1.1 2009/02/13 05:19:51 jmmv Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/util/bzip2

TESTS_SH=	t_bzip2

DIST=           ${NETBSDSRCDIR}/dist/bzip2
.PATH:		${DIST}

FILESDIR=	${TESTSDIR}
.for f in sample1.bz2 sample1.ref sample2.bz2 sample2.ref \
          sample3.bz2 sample3.ref
FILES+=		${f}
FILESNAME_${f}=	d_${f}
.endfor

.include <bsd.test.mk>
