#	$NetBSD: Makefile.inc,v 1.5 2001/10/31 15:57:55 tv Exp $
#	$KAME: Makefile.inc,v 1.3 2000/11/08 05:58:24 itojun Exp $

.if !target(regress)
SYSDIR=	${.CURDIR}/../../../../sys
WARNS?=	1

regress:
	@./${PROG}

BENCHROUNDS?=	10000

benchmark:
	@time ./${PROG} ${BENCHROUNDS}
.endif
