# $NetBSD: Makefile.inc,v 1.7 2002/05/04 21:56:50 bjh21 Exp $

realall: ${MAP}

${MAP}: ${HOSTPROG}
	./${HOSTPROG} > ${MAP}

CLEANFILES+=${MAP}
FILES=${MAP}
FILESDIR=/usr/share/keymaps/atari
