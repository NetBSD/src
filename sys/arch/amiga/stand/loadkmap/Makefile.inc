# $NetBSD: Makefile.inc,v 1.8.18.1 2000/09/19 18:15:12 is Exp $

BINDIR=/usr/share/keymaps/amiga
MKMAN=	no
CLEANFILES=${FILES}

realall: ${PROG} ${FILES}

${FILES}: ${PROG}
	./${PROG} > $@

# don't install ${PROG}
proginstall::

