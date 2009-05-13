# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:36 jym Exp $

SUBDIR=			support

NOMAN=			# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/util/config

TESTS_SH=		t_config

CLEANFILES+=		t_config.sh
t_config.sh: t_config.awk t_config.in
	${TOOL_AWK} -f ${.CURDIR}/t_config.awk ${.CURDIR}/t_config.in >t_config.sh.tmp
	mv t_config.sh.tmp t_config.sh

FILESDIR=		${TESTSDIR}
FILES=			d_deffs_redef
FILES+=			d_loop
FILES+=			d_loop2
FILES+=			d_no_pseudo
FILES+=			d_postponed_orphan
FILES+=			d_pseudo_parent
FILES+=			d_shadow_instance
FILES+=			support/arch/regress/conf/Makefile.regress
FILES+=			support/arch/regress/conf/files.regress
FILES+=			support/arch/regress/conf/std.regress
FILES+=			support/conf/files

.include <bsd.test.mk>
.include <bsd.subdir.mk>
