# $NetBSD: Makefile.inc,v 1.7.4.1 1999/12/27 18:28:42 wrstuden Exp $
# utils one might want in a crunched binary

LDSTATIC=	-static		# only static compilation makes sense here

install: .NOTMAIN .MADE
