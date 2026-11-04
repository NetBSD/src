# $NetBSD: Makefile,v 1.4.22.2 2026/11/04 23:28:16 perseant Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/hash

TESTS_C+=	t_sha2
TESTS_C+=	t_hmac
LDADD.t_hmac+=	-lcrypto
DDADD.t_hmac+=	${LIBCRYPTO}

TESTS_SH+=	t_hash

BINDIR=		${TESTSDIR}
MKMAN=		no

PROGS+=		h_hash

FILESDIR=	${TESTSDIR}/data

FILES+=		data/md5test-in
FILES+=		data/md5test-out
FILES+=		data/sha1test-in
FILES+=		data/sha1test-out
FILES+=		data/sha1test2-out

.include <bsd.test.mk>
