# $NetBSD: Makefile.inc,v 1.9 2001/12/12 00:05:09 tv Exp $
# utils one might want in a crunched binary

LDSTATIC=	-static		# only static compilation makes sense here
.if ${MACHINE_ARCH} != "sparc64"
DBG=		-Os
.endif

install: .NOTMAIN .MADE
