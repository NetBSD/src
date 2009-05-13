# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:38 jym Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/util/grep

TESTS_SH=	t_grep

FILESDIR=	${TESTSDIR}
FILES=		d_basic.out
FILES+=		d_begin_end_a.out
FILES+=		d_begin_end_b.out
FILES+=		d_binary.out
FILES+=		d_context2_a.out
FILES+=		d_context2_b.out
FILES+=		d_context2_c.out
FILES+=		d_context_a.in
FILES+=		d_context_a.out
FILES+=		d_context_b.in
FILES+=		d_context_b.out
FILES+=		d_context_c.out
FILES+=		d_context_d.out
FILES+=		d_egrep.out
FILES+=		d_file_exp.inout
FILES+=		d_ignore_case.out
FILES+=		d_input
FILES+=		d_invert.in
FILES+=		d_invert.out
FILES+=		d_recurse.out
FILES+=		d_recurse_symlink.err
FILES+=		d_recurse_symlink.out
FILES+=		d_whole_line.out
FILES+=		d_word_regexps.out
FILES+=		d_zgrep.out

.include <bsd.test.mk>
