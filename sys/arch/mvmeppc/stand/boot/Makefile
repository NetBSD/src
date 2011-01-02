#	$NetBSD: Makefile,v 1.5 2011/01/02 09:40:51 tsutsui Exp $

SRCS=   boot.c conf.c devopen.c
PROG=	boot.elf
LIBS=	${LIBSA} ${LIBKERN}
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
