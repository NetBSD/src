#	$NetBSD: Makefile,v 1.6 2004/03/19 08:07:28 jmc Exp $

NOMAN=  	# defined

UUDECODE_FILES=	ff_after_dnl.m4

regress: test-ff_after_dnl test-m4wrap test-m4wrap-P

test-ff_after_dnl: ff_after_dnl.m4
	m4 ff_after_dnl.m4 | diff ${.CURDIR}/ff_after_dnl.out -

test-m4wrap:
	m4 ${.CURDIR}/m4wrap.m4 | diff ${.CURDIR}/m4wrap.out -

test-m4wrap-P:
	m4 -P ${.CURDIR}/m4wrap-P.m4 | diff ${.CURDIR}/m4wrap-P.out -

.include <bsd.files.mk>
.include <bsd.prog.mk>
