# $NetBSD: Makefile.inc,v 1.4.38.1 2002/04/17 00:02:43 nathanw Exp $

realall: ${MAP}

${MAP}: ${HOSTPROG}
	./${HOSTPROG} > ${MAP}

CLEANFILES+=${MAP}
FILES=${MAP}
FILESDIR=/usr/share/keymaps/atari

.include <bsd.files.mk>
