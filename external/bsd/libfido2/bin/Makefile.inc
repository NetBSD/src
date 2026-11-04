#	$NetBSD: Makefile.inc,v 1.3.4.2 2026/11/04 22:38:34 perseant Exp $

.include "${.PARSEDIR}/../Makefile.inc"

.PATH: ${DIST}/tools ${DIST}/man ${DIST}/openbsd-compat

LDADD+=-lfido2 -lcbor -lusbhid -lcrypto -lz -lm
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBCRYPTO} ${LIBZ} ${LIBM}
