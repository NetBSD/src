# $NetBSD: Makefile.inc,v 1.8 1999/03/03 19:54:05 tron Exp $

BINDIR=/usr/share/keymaps/amiga
MKMAN=	no
CLEANFILES=${FILES}

all: ${PROG} ${FILES}

${FILES}: ${PROG}
	./${PROG} > $@

# don't install ${PROG}
proginstall::

