# $NetBSD: Makefile.inc,v 1.11 2004/06/09 16:03:37 abs Exp $
# utils one might want in a crunched binary

.if !defined(NOTINCRUNCHED)
LDSTATIC=	-static		# only static compilation makes sense here

install: .NOTMAIN .MADE
.endif
