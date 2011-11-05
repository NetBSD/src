#	$NetBSD: Makefile,v 1.1 2011/11/05 23:32:51 christos Exp $

LDADD+=-ltre
DPADD+=${LIBTRE}
TESTSDIR=       ${TESTSBASE}/lib/libtre

LCREGEX=${.CURDIR}/../libc/regex
.PATH: ${LCREGEX}
.include "${LCREGEX}/Makefile"

