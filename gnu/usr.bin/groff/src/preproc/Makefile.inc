# $NetBSD: Makefile.inc,v 1.4 2010/12/16 22:54:20 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/Makefile.inc,v 1.1 2001/04/17 12:37:09 ru Exp $

DPADD+=		${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBGROFF} -lm -lsupc++

.include "../../Makefile.inc"
