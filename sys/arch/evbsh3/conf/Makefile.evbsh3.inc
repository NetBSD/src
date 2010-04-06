#       $NetBSD: Makefile.evbsh3.inc,v 1.6 2010/04/06 15:54:30 nonaka Exp $

.if defined(BOARDMKFRAG)        # Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif
