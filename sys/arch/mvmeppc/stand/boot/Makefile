#	$NetBSD: Makefile,v 1.2.16.2 2004/09/18 14:37:57 skrll Exp $

SRCS=   boot.c conf.c devopen.c
PROG=	boot.elf
LIBS=	${LIBSA}
DPADD=	${LIBS}
CLEANFILES+= boot.bin

boot.bin: ${PROG}
	${OBJCOPY} -O binary ${.OBJDIR}/boot.elf $@

.include "../Makefile.booters"

all realall: boot.bin

install: boot.bin
	${INSTALL_FILE} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.OBJDIR}/boot.bin ${DESTDIR}${MDEC_DIR}/boot

.include <bsd.prog.mk>
