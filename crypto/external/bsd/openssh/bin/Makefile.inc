#	$NetBSD: Makefile.inc,v 1.5.2.2 2026/11/04 22:36:36 perseant Exp $

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
