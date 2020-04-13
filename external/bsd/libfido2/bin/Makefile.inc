#	$NetBSD: Makefile.inc,v 1.2.2.2 2020/04/13 07:46:06 martin Exp $

.include "${.PARSEDIR}/../Makefile.inc"

.PATH: ${DIST}/tools ${DIST}/man ${DIST}/openbsd-compat

LDADD+=-lfido2 -lcbor -lusbhid -lcrypto  -lm
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBCRYPTO} ${LIBM}
