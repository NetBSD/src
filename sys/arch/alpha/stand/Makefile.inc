# $NetBSD: Makefile.inc,v 1.13 1998/09/28 21:18:56 thorpej Exp $

BINDIR=			/usr/mdec

CWARNFLAGS+=	-Wall

HAVE_GCC28!=	${CC} --version | egrep "^(2\.8|egcs)" ; echo 
.if (${HAVE_GCC28} != "")
CWARNFLAGS+=	-Wno-main
.endif
