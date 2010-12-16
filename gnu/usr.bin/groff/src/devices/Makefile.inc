# $NetBSD: Makefile.inc,v 1.4 2010/12/16 22:54:18 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/Makefile.inc,v 1.1 2001/04/17 12:37:02 ru Exp $

DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm -lsupc++

.include "../../Makefile.inc"
