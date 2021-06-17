# $NetBSD: Makefile.inc,v 1.7.218.1 2021/06/17 04:46:18 thorpej Exp $

.include <bsd.hostinit.mk>

#HOST_CFLAGS+=	-Wall -Werror

realall: ${MAP}

${MAP}: ${HOSTPROG}
	./${HOSTPROG} > ${MAP}

CLEANFILES+=${MAP}
FILES=${MAP}
FILESDIR=/usr/share/keymaps/atari
