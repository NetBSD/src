#	$NetBSD: Makefile.inc,v 1.4 2025/07/19 12:38:39 christos Exp $

.include "${.PARSEDIR}/../Makefile.inc"

.PATH: ${DIST}/tools ${DIST}/man ${DIST}/openbsd-compat

LDADD+=-lfido2 -lcbor -lusbhid -lcrypto -lz -lm -lpthread
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBCRYPTO} ${LIBZ} ${LIBM} \
    ${LIBPTHREAD}
