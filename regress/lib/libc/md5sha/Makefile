#	$NetBSD: Makefile,v 1.3 2002/09/18 05:41:40 lukem Exp $

NOMAN=		# defined
LDSTATIC=	-static

PROG=		test
WARNS=		1

# MD5 tests from RFC1321
# SHA1 tests from FIPS-180-1

regress:
	./test -r < ${.CURDIR}/md5test-in | diff - ${.CURDIR}/md5test-out
	./test -rs < ${.CURDIR}/sha1test-in | diff - ${.CURDIR}/sha1test-out
	jot -s "" -b "a" -n 1000000 | ./test -rs | diff - ${.CURDIR}/sha1test2-out

.include <bsd.prog.mk>
