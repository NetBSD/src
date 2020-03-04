#	$NetBSD: Makefile.inc,v 1.2 2020/03/04 17:31:08 christos Exp $

.include "${.PARSEDIR}/../Makefile.inc"

.PATH: ${DIST}/tools ${DIST}/man ${DIST}/openbsd-compat

LDADD+=-lfido2 -lcbor -lusbhid -lcrypto  -lm
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBCRYPTO} ${LIBM}
