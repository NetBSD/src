#       $NetBSD: Makefile.evbsh3.inc,v 1.5.78.1 2010/08/11 22:51:57 yamt Exp $

.if defined(BOARDMKFRAG)        # Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif
