#	$NetBSD: Makefile,v 1.3 2006/05/29 16:47:07 simonb Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

CLEANFILES=	ab1 ab2 bc1 bc2

regress:
	@echo -n aaaabbbb > ab1
	@(echo -n aaaa | ${GZIPPROG} ; echo -n bbbb | ${GZIPPROG}) | \
	    ${GZIPPROG} -d > ab2
	@cmp ab1 ab2
	@echo -n bbbb > bc1
	@(true | ${GZIPPROG} ; cat bc1 | ${GZIPPROG}) | ${GZIPPROG} -d > bc2
	@cmp bc1 bc2

.include <bsd.prog.mk>
