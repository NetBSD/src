# $NetBSD: Makefile,v 1.13 2008/12/21 00:24:07 christos Exp $

.include <bsd.sys.mk>
TESTS != echo [a-z]*

regress:
.	for i in ${TESTS}
		${.CURDIR}/$i
.	endfor

.include <bsd.subdir.mk>

