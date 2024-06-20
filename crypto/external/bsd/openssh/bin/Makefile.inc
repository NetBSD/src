#	$NetBSD: Makefile.inc,v 1.3.26.2 2024/06/20 18:18:44 martin Exp $

CPPFLAGS+=-DWITH_OPENSSL

CRYPTOBSD=${NETBSDSRCDIR}/crypto/external/bsd

PROGDPLIBS+= \
    ssh ${CRYPTOBSD}/openssh/lib \
    crypto ${CRYPTOBSD}/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
    crypt ${NETBSDSRCDIR}/lib/libcrypt \
    z ${NETBSDSRCDIR}/lib/libz

.include "${.PARSEDIR}/../Makefile.inc"
