#	$NetBSD: Makefile.inc,v 1.4 2001/01/14 06:24:53 christos Exp $
#	$KAME: Makefile.inc,v 1.3 2000/11/08 05:58:24 itojun Exp $

.if !target(regress)
SYSDIR=	${.CURDIR}/../../../../sys
WARNS?=	1

regress:
	@${.OBJDIR}/${PROG}

BENCHROUNDS?=	10000

benchmark:
	@time ${.OBJDIR}/${PROG} ${BENCHROUNDS}
.endif
