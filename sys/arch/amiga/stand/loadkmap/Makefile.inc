# $NetBSD: Makefile.inc,v 1.10.4.1 2002/01/10 19:37:29 thorpej Exp $

BINDIR=/usr/share/keymaps/amiga
NOMAN=	# defined
CLEANFILES=${FILES}

realall: ${HOSTPROG} ${FILES}

${FILES}: ${HOSTPROG}
	./${HOSTPROG} > $@
