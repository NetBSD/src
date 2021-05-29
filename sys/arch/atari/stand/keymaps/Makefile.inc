# $NetBSD: Makefile.inc,v 1.8 2021/05/29 09:50:04 tsutsui Exp $

.include <bsd.hostinit.mk>

#HOST_CFLAGS+=	-Wall -Werror

realall: ${MAP}

${MAP}: ${HOSTPROG}
	./${HOSTPROG} > ${MAP}

CLEANFILES+=${MAP}
FILES=${MAP}
FILESDIR=/usr/share/keymaps/atari
