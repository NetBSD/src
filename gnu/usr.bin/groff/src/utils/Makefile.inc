# $NetBSD: Makefile.inc,v 1.4 2010/12/16 22:54:22 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/Makefile.inc,v 1.1 2001/04/17 12:37:15 ru Exp $

DPADD+=		${LIBGROFF}
LDADD+=		${LIBGROFF}
.if ${LIBGROFF_ONLY:Uno} != "yes"
DPADD+=		${LIBM} ${LIBSUPCXX}
LDADD+=		-lm -lsupc++
.endif

.include "../../Makefile.inc"
