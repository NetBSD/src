# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:37 jym Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/util/cut

TESTS_SH=	t_cut

FILESDIR=	${TESTSDIR}
FILES=		d_basic.out
FILES+=		d_cut.in
FILES+=		d_dflag.out
FILES+=		d_dsflag.out
FILES+=		d_latin1.in
FILES+=		d_sflag.out
FILES+=		d_utf8.in

.include <bsd.test.mk>
