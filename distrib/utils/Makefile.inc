# $NetBSD: Makefile.inc,v 1.10.2.1 2004/06/17 08:30:26 tron Exp $
# utils one might want in a crunched binary

.if !defined(NOTINCRUNCHED)
LDSTATIC=	-static		# only static compilation makes sense here

install: .NOTMAIN .MADE
.endif
