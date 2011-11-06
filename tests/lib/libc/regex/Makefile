# $NetBSD: Makefile,v 1.8 2011/11/06 19:45:37 christos Exp $

MKMAN=	no

.include <bsd.own.mk>

TESTSDIR?=	${TESTSBASE}/lib/libc/regex
IMPLEMENTATION?=	-DREGEX_SPENCER
SKIPASSOC?=	left

BINDIR=		${TESTSDIR}
PROGS?=		h_regex
PROGS+=		h_regex_att
SRCS.h_regex_att=	att.c
SRCS.h_regex=	main.c split.c debug.c
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/regex ${IMPLEMENTATION}

TESTS_SH?=	t_regex
TESTS_SH+=	t_regex_att
TESTS_C=	t_exhaust

FILESDIR=	${TESTSDIR}/data
FILES+=		README
FILES+=		data/anchor.in
FILES+=		data/backref.in
FILES+=		data/basic.in
FILES+=		data/bracket.in
FILES+=		data/c_comments.in
FILES+=		data/complex.in
FILES+=		data/error.in
FILES+=		data/meta.in
FILES+=		data/nospec.in
FILES+=		data/paren.in
FILES+=		data/regress.in
FILES+=		data/repet_bounded.in
FILES+=		data/repet_multi.in
FILES+=		data/repet_ordinary.in
FILES+=		data/startend.in
FILES+=		data/subexp.in
FILES+=		data/subtle.in
FILES+=		data/word_bound.in
FILES+=		data/zero.in
#FILES+=		data/att/README
FILES+=		data/att/basic.dat
FILES+=		data/att/categorization.dat
FILES+=		data/att/forcedassoc.dat
FILES+=		data/att/leftassoc.dat
FILES+=		data/att/nullsubexpr.dat
FILES+=		data/att/repetition.dat
FILES+=		data/att/rightassoc.dat

t_regex.sh: t_regex.awk t_regex.in
	${TOOL_AWK} -f ${.ALLSRC} > ${.TARGET}

t_regex_att.sh: t_regex.awk t_regex_att.in
	${TOOL_AWK} -v skipassoc=${SKIPASSOC} -f ${.ALLSRC} > ${.TARGET}

CLEANFILES+= t_regex.sh t_regex_att.sh

.include <bsd.test.mk>
