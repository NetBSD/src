#	$NetBSD: Makefile,v 1.5 2003/09/30 04:20:24 mycroft Exp $

NOMAN=  	# defined
CLEANFILES+=	ff_after_dnl.m4

regress: test-ff_after_dnl test-m4wrap test-m4wrap-P

test-ff_after_dnl: ff_after_dnl.m4
	m4 ff_after_dnl.m4 | diff ${.CURDIR}/ff_after_dnl.out -

ff_after_dnl.m4: ff_after_dnl.m4.uu
	uudecode ${.CURDIR}/ff_after_dnl.m4.uu

test-m4wrap:
	m4 ${.CURDIR}/m4wrap.m4 | diff ${.CURDIR}/m4wrap.out -

test-m4wrap-P:
	m4 -P ${.CURDIR}/m4wrap-P.m4 | diff ${.CURDIR}/m4wrap-P.out -

.include <bsd.prog.mk>
