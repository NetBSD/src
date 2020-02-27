#	$NetBSD: Makefile.inc,v 1.1 2020/02/27 00:24:40 christos Exp $

CPPFLAGS+=-DWITH_OPENSSL
LDADD+=	-lssh -lcrypto -lcrypt -lz
DPADD+=	${LIBSSH} ${LIBCRYPTO} ${LIBCRYPT} ${LIBZ}

.include "${.PARSEDIR}/../Makefile.inc"
