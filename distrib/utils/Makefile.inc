# $NetBSD: Makefile.inc,v 1.10 2003/09/19 16:29:21 martin Exp $
# utils one might want in a crunched binary

LDSTATIC=	-static		# only static compilation makes sense here

install: .NOTMAIN .MADE
