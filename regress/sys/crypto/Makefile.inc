#	$NetBSD: Makefile.inc,v 1.7 2002/09/18 04:16:01 lukem Exp $
#	$KAME: Makefile.inc,v 1.3 2000/11/08 05:58:24 itojun Exp $

.if !target(regress)
.include <bsd.own.mk>

SYSDIR=	${NETBSDSRCDIR}/sys
WARNS?=	1

regress:
	@./${PROG}

BENCHROUNDS?=	10000

benchmark:
	@time ./${PROG} ${BENCHROUNDS}
.endif

# do not install regression test programs
proginstall::
