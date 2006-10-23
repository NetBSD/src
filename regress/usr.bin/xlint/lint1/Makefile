# $NetBSD: Makefile,v 1.4 2006/10/23 00:08:54 christos Exp $

.include <bsd.sys.mk>
TESTS != echo test*.c
FAIL_TESTS != echo fail-test*.c
LINT1 = ${DESTDIR}/usr/libexec/lint1

regress:
.	for i in ${TESTS}
	${HOST_SH} -c "${LINT1} -g -S $i /dev/null"
	echo ok: $i
.	endfor
.	for i in ${FAIL_TESTS}
	${HOST_SH} -c "! ${LINT1} -g -S -w $i /dev/null"
	echo ok: $i '(should warn & fail)'
.	endfor

.include <bsd.subdir.mk>

