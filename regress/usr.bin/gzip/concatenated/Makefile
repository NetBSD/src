#	$NetBSD: Makefile,v 1.1 2004/07/10 19:32:06 yamt Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

CLEANFILES=	ab1 ab2

regress:
	@echo -n aaaabbbb > ab1
	@(echo -n aaaa | ${GZIP} ; echo -n bbbb | ${GZIP}) | ${GZIP} -d > ab2
	@cmp ab1 ab2

.include <bsd.prog.mk>
