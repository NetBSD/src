# $NetBSD: Makefile,v 1.5 2011/11/05 23:33:15 christos Exp $

MKMAN=	no

.include <bsd.own.mk>

TESTSDIR?=	${TESTSBASE}/lib/libc/regex

BINDIR=		${TESTSDIR}
PROGS+=		h_regex
SRCS.h_regex=	main.c split.c debug.c
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/regex

TESTS_SH=	t_regex
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

t_regex.sh: t_regex.awk t_regex.in
	${TOOL_AWK} -f ${.ALLSRC} > ${.TARGET}

CLEANFILES+= t_regex.sh

.include <bsd.test.mk>
