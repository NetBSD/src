# $NetBSD: Makefile.inc,v 1.7 1999/02/13 02:54:38 lukem Exp $

BINDIR=/usr/share/keymaps/amiga
MKMAN=	no
CLEANFILES=${FILES}

all: ${PROG} ${FILES}

${FILES}: ${PROG}
	./${PROG} > ${FILES}

# don't install ${PROG}
proginstall::

