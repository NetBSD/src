#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:18 tv Exp $

NOMAN=  	# defined
CLEANFILES+=	ff_after_dnl.m4

regress: test-ff_after_dnl test-m4wrap

test-ff_after_dnl: ff_after_dnl.m4
	m4 ff_after_dnl.m4 | diff - ${.CURDIR}/ff_after_dnl.out

ff_after_dnl.m4: ff_after_dnl.m4.uu
	uudecode ${.CURDIR}/ff_after_dnl.m4.uu

test-m4wrap:
	m4 ${.CURDIR}/m4wrap.m4 | diff - ${.CURDIR}/m4wrap.out

.include <bsd.prog.mk>
