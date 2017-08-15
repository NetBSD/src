#	$NetBSD: Makefile.inc,v 1.2.8.1 2017/08/15 05:27:20 snj Exp $

CPPFLAGS+=-DWITH_OPENSSL
LDADD+=	-lssh -lcrypto -lcrypt -lz
DPADD+=	${LIBSSH} ${LIBCRYPTO} ${LIBCRYPT} ${LIBZ}

.include "${.PARSEDIR}/../Makefile.inc"
