# $NetBSD: Makefile.inc,v 1.12 1998/08/27 12:20:51 tv Exp $

BINDIR=			/usr/mdec

HAVE_GCC28!=	${CC} --version | egrep "^(2\.8|egcs)" ; echo 
.if (${HAVE_GCC28} != "")
CWARNFLAGS+=	-Wno-main
.endif
