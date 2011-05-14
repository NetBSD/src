#	$NetBSD: Makefile.inc,v 1.8 2011/05/14 16:26:23 christos Exp $
#	$KAME: Makefile.inc,v 1.3 2000/11/08 05:58:24 itojun Exp $

.if !target(regress)
.include <bsd.own.mk>

SYSDIR=	${NETBSDSRCDIR}/sys
WARNS?=	4

regress:
	@./${PROG}

BENCHROUNDS?=	10000

benchmark:
	@time ./${PROG} ${BENCHROUNDS}
.endif

# do not install regression test programs
proginstall::
