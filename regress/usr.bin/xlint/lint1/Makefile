# $NetBSD: Makefile,v 1.2 2006/10/14 23:38:40 dogcow Exp $

TESTS != echo test*.c
LINT1 = ${DESTDIR}/usr/libexec/lint1

regress:
.	for i in ${TESTS}
	${LINT1} -g -S $i /dev/null
.	endfor

.include <bsd.subdir.mk>

