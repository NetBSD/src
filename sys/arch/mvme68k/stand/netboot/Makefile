#	$NetBSD: Makefile,v 1.13.2.2 2002/06/23 17:38:22 jdolecek Exp $

DEFS= -DSUN_BOOTPARAMS

SRCS=   boot.c conf.c devopen.c
SRCS+=	if_ie.c if_le.c
PROG=	netboot
LIBS=	${LIBSA} ${LIBBUG}
DPADD=	${LIBS}
CLEANFILES+= netboot.bin

netboot.bin: ${PROG}
	${OBJCOPY} -O binary ${.OBJDIR}/netboot ${.TARGET}

.include "../Makefile.booters"

all realall: netboot.bin

install: netboot.bin
	${INSTALL_FILE} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.OBJDIR}/netboot.bin ${DESTDIR}${MDEC_DIR}/netboot

.include <bsd.prog.mk>
