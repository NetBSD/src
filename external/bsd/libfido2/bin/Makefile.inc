#	$NetBSD: Makefile.inc,v 1.3.4.1 2025/08/02 05:21:50 perseant Exp $

.include "${.PARSEDIR}/../Makefile.inc"

.PATH: ${DIST}/tools ${DIST}/man ${DIST}/openbsd-compat

LDADD+=-lfido2 -lcbor -lusbhid -lcrypto -lz -lm -lpthread
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBCRYPTO} ${LIBZ} ${LIBM} \
    ${LIBPTHREAD}
