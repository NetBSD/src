# $NetBSD: Makefile.inc,v 1.6 2002/04/24 08:18:53 lukem Exp $

realall: ${MAP}

${MAP}: ${HOSTPROG}
	./${HOSTPROG} > ${MAP}

CLEANFILES+=${MAP}
FILES=${MAP}
FILESDIR=/usr/share/keymaps/atari

.include <bsd.prog.mk>
