# $NetBSD: Makefile.inc,v 1.10.8.1 2002/01/08 00:23:02 nathanw Exp $

BINDIR=/usr/share/keymaps/amiga
NOMAN=	# defined
CLEANFILES=${FILES}

realall: ${HOSTPROG} ${FILES}

${FILES}: ${HOSTPROG}
	./${HOSTPROG} > $@
