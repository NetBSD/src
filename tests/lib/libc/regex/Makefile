# $NetBSD: Makefile,v 1.2 2011/10/09 18:21:08 christos Exp $

MKMAN=	no

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/regex

BINDIR=		${TESTSDIR}
PROG=		h_regex
SRCS=		main.c split.c debug.c
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
FILES+=		data/nul.in
FILES+=		data/paren.in
FILES+=		data/regress.in
FILES+=		data/repet_bounded.in
FILES+=		data/repet_multi.in
FILES+=		data/repet_ordinary.in
FILES+=		data/startend.in
FILES+=		data/subexp.in
FILES+=		data/subtle.in
FILES+=		data/word_bound.in

t_regex.sh: t_regex.awk t_regex.in
	${TOOL_AWK} -f ${.CURDIR}/t_regex.awk ${.CURDIR}/t_regex.in > ${.TARGET}

CLEANFILES+= t_regex.sh

.include <bsd.test.mk>
