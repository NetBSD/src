#	$Id: Makefile,v 1.2 1993/05/22 07:10:44 cgd Exp $

PROG=	bitstring_test
NOMAN=	noman, no way, man

install:


regress:	test-8 test-27 test-32 test-49 test-64 test-67

test-8: bitstring_test
	bitstring_test 8 | diff - ${.CURDIR}/good/8

test-27: bitstring_test
	bitstring_test 27 | diff - ${.CURDIR}/good/27

test-32: bitstring_test
	bitstring_test 32 | diff - ${.CURDIR}/good/32

test-49: bitstring_test
	bitstring_test 49 | diff - ${.CURDIR}/good/49

test-64: bitstring_test
	bitstring_test 64 | diff - ${.CURDIR}/good/64

test-67: bitstring_test
	bitstring_test 67 | diff - ${.CURDIR}/good/67


.include <bsd.prog.mk>
