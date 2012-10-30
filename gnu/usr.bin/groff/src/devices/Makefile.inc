# $NetBSD: Makefile.inc,v 1.4.6.1 2012/10/30 18:58:29 yamt Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/Makefile.inc,v 1.1 2001/04/17 12:37:02 ru Exp $

DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm

.include "../../Makefile.inc"
