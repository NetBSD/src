#	$NetBSD: Makefile.inc,v 1.2.14.1 2017/08/15 04:39:20 snj Exp $

CPPFLAGS+=-DWITH_OPENSSL
LDADD+=	-lssh -lcrypto -lcrypt -lz
DPADD+=	${LIBSSH} ${LIBCRYPTO} ${LIBCRYPT} ${LIBZ}

.include "${.PARSEDIR}/../Makefile.inc"
