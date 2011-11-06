#	$NetBSD: Makefile,v 1.2 2011/11/06 18:37:40 christos Exp $

LDADD+=-ltre
DPADD+=${LIBTRE}
TESTSDIR=       ${TESTSBASE}/lib/libtre
# XXX: tre eats copious amounts of memory even 9 does not work
IMPLEMENTATION=-DREGEX_TRE -DREGEX_MAXSIZE=2
SKIPASSOC=right

LCREGEX=${.CURDIR}/../libc/regex
.PATH: ${LCREGEX}
.include "${LCREGEX}/Makefile"

