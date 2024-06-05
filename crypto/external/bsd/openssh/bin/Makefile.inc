#	$NetBSD: Makefile.inc,v 1.5 2024/06/05 10:50:37 rin Exp $

CPPFLAGS+=-DWITH_OPENSSL

CRYPTOBSD=${NETBSDSRCDIR}/crypto/external/bsd

PROGDPLIBS+= \
    ssh ${CRYPTOBSD}/openssh/lib \
    crypto ${CRYPTOBSD}/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
    crypt ${NETBSDSRCDIR}/lib/libcrypt \
    z ${NETBSDSRCDIR}/lib/libz

.include "${.PARSEDIR}/../Makefile.inc"
