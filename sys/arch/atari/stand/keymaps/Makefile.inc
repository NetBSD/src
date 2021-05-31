# $NetBSD: Makefile.inc,v 1.7.220.1 2021/05/31 22:15:11 cjep Exp $

.include <bsd.hostinit.mk>

#HOST_CFLAGS+=	-Wall -Werror

realall: ${MAP}

${MAP}: ${HOSTPROG}
	./${HOSTPROG} > ${MAP}

CLEANFILES+=${MAP}
FILES=${MAP}
FILESDIR=/usr/share/keymaps/atari
