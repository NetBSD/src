# $NetBSD: Makefile.inc,v 1.8 1999/11/13 14:18:44 minoura Exp $
# utils one might want in a crunched binary

LDSTATIC=	-static		# only static compilation makes sense here

install: .NOTMAIN .MADE
