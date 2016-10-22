# $NetBSD: Makefile,v 1.1 2016/10/22 14:13:39 abhinav Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/uniq
TESTS_SH=	t_uniq

FILESDIR=	${TESTSDIR}
FILES=		d_basic.in
FILES+=		d_basic.out
FILES+=		d_counts.out
FILES+=		d_input.in
FILES+=		d_show_duplicates.out
FILES+=		d_show_uniques.out

.include <bsd.test.mk>
