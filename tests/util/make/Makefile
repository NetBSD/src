# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:39 jym Exp $

NOMAN=			# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/util/make

TESTS_SH=		t_make

CLEANFILES+=		t_make.sh
t_make.sh: t_make.awk t_make.in
	${TOOL_AWK} -f ${.CURDIR}/t_make.awk ${.CURDIR}/t_make.in > t_make.sh

FILESDIR=		${TESTSDIR}
FILES=			d_comment.mk
FILES+=			d_comment.out
FILES+=			d_cond1.mk
FILES+=			d_cond1.out
FILES+=			d_dotwait.mk
FILES+=			d_dotwait.out
FILES+=			d_export.mk
FILES+=			d_export.out
FILES+=			d_export_all.mk
FILES+=			d_export_all.out
FILES+=			d_moderrs.mk
FILES+=			d_moderrs.out
FILES+=			d_modmatch.mk
FILES+=			d_modmatch.out
FILES+=			d_modmisc.mk
FILES+=			d_modmisc.out
FILES+=			d_modorder.mk
FILES+=			d_modorder.out
FILES+=			d_modts.mk
FILES+=			d_modts.out
FILES+=			d_modword.mk
FILES+=			d_modword.out
FILES+=			d_posix.mk
FILES+=			d_posix.out
FILES+=			d_qequals.mk
FILES+=			d_qequals.out
FILES+=			d_ternary.mk
FILES+=			d_ternary.out
FILES+=			d_varcmd.mk
FILES+=			d_varcmd.out

.include <bsd.test.mk>
