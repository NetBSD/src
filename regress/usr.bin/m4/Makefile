#	$NetBSD: Makefile,v 1.1 1997/12/30 23:30:48 cgd Exp $

NOMAN=  noman, no way, man

CLEANFILES+= ff_after_dnl.m4

regress: test-ff_after_dnl test-m4wrap

test-ff_after_dnl: ff_after_dnl.m4
	m4 ff_after_dnl.m4 | diff - ${.CURDIR}/ff_after_dnl.out

ff_after_dnl.m4: ff_after_dnl.m4.uu
	uudecode ${.CURDIR}/ff_after_dnl.m4.uu

test-m4wrap:
	m4 ${.CURDIR}/m4wrap.m4 | diff - ${.CURDIR}/m4wrap.out

.include <bsd.prog.mk>
