#	$NetBSD: Makefile,v 1.1.2.3 2002/04/01 07:41:31 nathanw Exp $

SRCS=   boot.c conf.c devopen.c
PROG=	boot.tmp
LIBS=	${LIBSA}
DPADD=	${LIBS}
CLEANFILES+= boot.bin

boot.bin: ${PROG}
	${OBJCOPY} -O binary ${.OBJDIR}/boot.tmp $@

.include "../Makefile.booters"

all realall: boot.bin

install: boot.bin
	${INSTALL_FILE} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.OBJDIR}/boot.bin ${DESTDIR}${MDEC_DIR}/boot

.include <bsd.prog.mk>
