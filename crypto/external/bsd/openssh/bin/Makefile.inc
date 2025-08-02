#	$NetBSD: Makefile.inc,v 1.5.2.1 2025/08/02 05:18:44 perseant Exp $

CPPFLAGS+=-DWITH_OPENSSL

CRYPTOBSD=${NETBSDSRCDIR}/crypto/external/bsd

PROGDPLIBS+= \
    ssh ${CRYPTOBSD}/openssh/lib \
    crypto ${CRYPTOBSD:H}/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
    crypt ${NETBSDSRCDIR}/lib/libcrypt \
    z ${NETBSDSRCDIR}/lib/libz \
    pthread ${NETBSDSRCDIR}/lib/libpthread

.include "${.PARSEDIR}/../Makefile.inc"
