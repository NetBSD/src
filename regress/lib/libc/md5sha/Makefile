#	$NetBSD: Makefile,v 1.4 2003/09/30 04:20:24 mycroft Exp $

NOMAN=		# defined
LDSTATIC=	-static

PROG=		test
WARNS=		1

# MD5 tests from RFC1321
# SHA1 tests from FIPS-180-1

regress:
	./test -r < ${.CURDIR}/md5test-in | diff ${.CURDIR}/md5test-out -
	./test -rs < ${.CURDIR}/sha1test-in | diff ${.CURDIR}/sha1test-out -
	jot -s "" -b "a" -n 1000000 | ./test -rs | diff ${.CURDIR}/sha1test2-out -

.include <bsd.prog.mk>
