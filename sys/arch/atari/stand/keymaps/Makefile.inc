# $NetBSD: Makefile.inc,v 1.4.38.2 2002/06/20 03:38:20 nathanw Exp $

realall: ${MAP}

${MAP}: ${HOSTPROG}
	./${HOSTPROG} > ${MAP}

CLEANFILES+=${MAP}
FILES=${MAP}
FILESDIR=/usr/share/keymaps/atari
