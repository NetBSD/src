# $NetBSD: Makefile.inc,v 1.11 2001/12/12 01:49:36 tv Exp $

BINDIR=/usr/share/keymaps/amiga
NOMAN=	# defined
CLEANFILES=${FILES}

realall: ${HOSTPROG} ${FILES}

${FILES}: ${HOSTPROG}
	./${HOSTPROG} > $@
