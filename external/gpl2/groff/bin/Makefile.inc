# $NetBSD: Makefile.inc,v 1.1 2016/01/13 19:01:55 christos Exp $

.include "${.PARSEDIR}/../Makefile.inc"

DPADD+=		${LIBM}
LDADD+=		-lm
