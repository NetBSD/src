# $NetBSD: Makefile,v 1.3 2006/10/18 00:05:33 he Exp $

TESTS != echo test*.c
FAIL_TESTS != echo fail-test*.c
LINT1 = ${DESTDIR}/usr/libexec/lint1

regress:
.	for i in ${TESTS}
	${LINT1} -g -S $i /dev/null
	echo ok: $i
.	endfor
.	for i in ${FAIL_TESTS}
	! ${LINT1} -g -S -w $i /dev/null
	echo ok: $i '(should warn & fail)'
.	endfor

.include <bsd.subdir.mk>

