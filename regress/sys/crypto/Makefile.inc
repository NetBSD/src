#	$NetBSD: Makefile.inc,v 1.6 2002/08/18 14:59:34 itojun Exp $
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

# do not install regression test programs
proginstall::
