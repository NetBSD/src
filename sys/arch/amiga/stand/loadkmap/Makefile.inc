# $NetBSD: Makefile.inc,v 1.8.8.1 2000/11/20 19:58:52 bouyer Exp $

BINDIR=/usr/share/keymaps/amiga
MKMAN=	no
CLEANFILES=${FILES}

realall: ${HOSTPROG} ${FILES}

${FILES}: ${HOSTPROG}
	./${HOSTPROG} > $@
