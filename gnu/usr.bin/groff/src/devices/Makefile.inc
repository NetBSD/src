# $NetBSD: Makefile.inc,v 1.5 2012/08/10 12:12:18 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/Makefile.inc,v 1.1 2001/04/17 12:37:02 ru Exp $

DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm

.include "../../Makefile.inc"
