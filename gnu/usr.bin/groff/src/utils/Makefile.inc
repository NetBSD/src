# $NetBSD: Makefile.inc,v 1.4.6.1 2012/10/30 18:58:32 yamt Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/Makefile.inc,v 1.1 2001/04/17 12:37:15 ru Exp $

DPADD+=		${LIBGROFF}
LDADD+=		${LIBGROFF}
.if ${LIBGROFF_ONLY:Uno} != "yes"
DPADD+=		${LIBM}
LDADD+=		-lm
.endif

.include "../../Makefile.inc"
