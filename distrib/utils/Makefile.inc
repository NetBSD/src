# $NetBSD: Makefile.inc,v 1.6.2.1 2000/02/06 19:58:00 he Exp $
# utils one might want in a crunched binary

LDSTATIC=	-static		# only static compilation makes sense here

install: .NOTMAIN .MADE
