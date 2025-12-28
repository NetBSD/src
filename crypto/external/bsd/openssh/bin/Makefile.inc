#	$NetBSD: Makefile.inc,v 1.9 2025/12/28 09:39:35 nia Exp $

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
