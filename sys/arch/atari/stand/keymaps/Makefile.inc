# $NetBSD: Makefile.inc,v 1.4.34.1 2002/06/23 17:35:19 jdolecek Exp $

realall: ${MAP}

${MAP}: ${HOSTPROG}
	./${HOSTPROG} > ${MAP}

CLEANFILES+=${MAP}
FILES=${MAP}
FILESDIR=/usr/share/keymaps/atari
