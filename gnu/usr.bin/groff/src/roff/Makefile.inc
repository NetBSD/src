# $NetBSD: Makefile.inc,v 1.5 2012/08/10 12:12:21 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/roff/Makefile.inc,v 1.1 2001/04/17 12:37:12 ru Exp $

DPADD+=		${LIBGROFF} ${LIBM}
LDADD+=		${LIBGROFF} -lm

.include "../../Makefile.inc"
