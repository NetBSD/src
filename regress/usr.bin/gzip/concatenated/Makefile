#	$NetBSD: Makefile,v 1.2 2005/01/20 20:40:00 erh Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

CLEANFILES=	ab1 ab2 bc1 bc2

regress:
	@echo -n aaaabbbb > ab1
	@(echo -n aaaa | ${GZIP} ; echo -n bbbb | ${GZIP}) | ${GZIP} -d > ab2
	@cmp ab1 ab2
	@echo -n bbbb > bc1
	@(true | ${GZIP} ; cat bc1 | ${GZIP}) | ${GZIP} -d > bc2
	@cmp bc1 bc2

.include <bsd.prog.mk>
