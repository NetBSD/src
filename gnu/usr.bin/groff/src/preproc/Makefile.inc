# $NetBSD: Makefile.inc,v 1.4.6.1 2012/10/30 18:58:30 yamt Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/Makefile.inc,v 1.1 2001/04/17 12:37:09 ru Exp $

DPADD+=		${LIBGROFF} ${LIBM}
LDADD+=		${LIBGROFF} -lm

.include "../../Makefile.inc"
