#	$NetBSD: Makefile.inc,v 1.4.2.1 2025/11/20 18:46:46 martin Exp $

.include "${.PARSEDIR}/../Makefile.inc"

.PATH: ${DIST}/tools ${DIST}/man ${DIST}/openbsd-compat

LDADD+=-lfido2 -lcbor -lusbhid -lcrypto -lz -lm
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBCRYPTO} ${LIBZ} ${LIBM}
