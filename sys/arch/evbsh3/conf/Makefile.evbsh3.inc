#       $NetBSD: Makefile.evbsh3.inc,v 1.5.94.1 2010/05/30 05:16:46 rmind Exp $

.if defined(BOARDMKFRAG)        # Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif
