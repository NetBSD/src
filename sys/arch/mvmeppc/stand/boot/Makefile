#	$NetBSD: Makefile,v 1.1 2002/02/27 21:02:26 scw Exp $

SRCS=   boot.c conf.c devopen.c
PROG=	netboot.tmp
LIBS=	${LIBSA}
DPADD=	${LIBS}
CLEANFILES+= netboot.bin

netboot.bin: ${PROG}
	${OBJCOPY} -O binary ${.OBJDIR}/netboot.tmp $@

.include "../Makefile.booters"

all realall: netboot.bin

install: netboot.bin
	${INSTALL_FILE} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.OBJDIR}/netboot.bin ${DESTDIR}${MDEC_DIR}/netboot

.include <bsd.prog.mk>
