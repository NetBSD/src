# $NetBSD: Makefile.inc,v 1.9 2000/09/17 18:24:13 is Exp $

BINDIR=/usr/share/keymaps/amiga
MKMAN=	no
CLEANFILES=${FILES}

realall: ${PROG} ${FILES}

${FILES}: ${PROG}
	./${PROG} > $@

# don't install ${PROG}
proginstall::

