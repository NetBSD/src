#	$NetBSD: Makefile.inc,v 1.7.2.2 2026/02/02 18:07:58 martin Exp $

CRYPTOBSD=${NETBSDSRCDIR}/crypto/external/bsd

PROGDPLIBS+= \
    ssh ${CRYPTOBSD}/openssh/lib \
    crypt ${NETBSDSRCDIR}/lib/libcrypt \
    z ${NETBSDSRCDIR}/lib/libz

.include "${.PARSEDIR}/../Makefile.inc"

.ifdef WITH_OPENSSL
CPPFLAGS+=-DWITH_OPENSSL

PROGDPLIBS+= \
    crypto ${CRYPTOBSD:H}/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto
.endif
