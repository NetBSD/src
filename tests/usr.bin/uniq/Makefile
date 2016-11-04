# $NetBSD: Makefile,v 1.1.2.2 2016/11/04 14:49:24 pgoyette Exp $

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
