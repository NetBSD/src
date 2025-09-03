# $NetBSD: Makefile,v 1.5 2025/07/19 12:03:35 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/hash

TESTS_C+=	t_sha2
TESTS_C+=	t_hmac
LDADD.t_hmac+=	-lcrypto -lpthread
DDADD.t_hmac+=	${LIBCRYPTO} ${LIBPTHREAD}

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
