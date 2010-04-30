#       $NetBSD: Makefile.evbsh3.inc,v 1.5.92.1 2010/04/30 14:39:19 uebayasi Exp $

.if defined(BOARDMKFRAG)        # Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif
