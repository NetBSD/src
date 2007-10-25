#	$NetBSD: Makefile,v 1.1 2007/10/25 14:51:19 he Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

CLEANFILES=	result

regress:
	env LANG=en_US.UTF-8 \
		${AWK} -f ${.CURDIR}/test.awk ${.CURDIR}/input > result
	@cmp ${.CURDIR}/expected result

.include <bsd.prog.mk>
