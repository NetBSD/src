#	$NetBSD: Makefile.inc,v 1.7 2025/07/19 15:14:26 christos Exp $

CPPFLAGS+=-DWITH_OPENSSL

CRYPTOBSD=${NETBSDSRCDIR}/crypto/external/bsd

PROGDPLIBS+= \
    ssh ${CRYPTOBSD}/openssh/lib \
    crypto ${CRYPTOBSD:H}/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
    crypt ${NETBSDSRCDIR}/lib/libcrypt \
    z ${NETBSDSRCDIR}/lib/libz \
    pthread ${NETBSDSRCDIR}/lib/libpthread

.include "${.PARSEDIR}/../Makefile.inc"
