#	$NetBSD: Makefile,v 1.14 2002/02/09 09:36:00 lukem Exp $

DEFS= -DSUN_BOOTPARAMS

SRCS=   boot.c conf.c devopen.c
SRCS+=	if_ie.c if_le.c
PROG=	netboot.tmp
LIBS=	${LIBSA} ${LIBBUG}
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
