# $NetBSD: Makefile.inc,v 1.5 2002/04/15 17:27:33 bjh21 Exp $

realall: ${MAP}

${MAP}: ${HOSTPROG}
	./${HOSTPROG} > ${MAP}

CLEANFILES+=${MAP}
FILES=${MAP}
FILESDIR=/usr/share/keymaps/atari

.include <bsd.files.mk>
