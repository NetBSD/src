# $NetBSD: Makefile.inc,v 1.5 2012/08/10 12:12:21 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/Makefile.inc,v 1.1 2001/04/17 12:37:15 ru Exp $

DPADD+=		${LIBGROFF}
LDADD+=		${LIBGROFF}
.if ${LIBGROFF_ONLY:Uno} != "yes"
DPADD+=		${LIBM}
LDADD+=		-lm
.endif

.include "../../Makefile.inc"
