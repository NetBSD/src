# $NetBSD: Makefile.inc,v 1.6 1997/07/14 15:07:04 veego Exp $

BINDIR=/usr/share/keymaps/amiga
NOMAN=
CLEANFILES=${FILES}

all: ${PROG} ${FILES}

${FILES}: ${PROG}
	./${PROG} > ${FILES}

# don't install ${PROG}
proginstall::

