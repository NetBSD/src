#	$NetBSD: Makefile,v 1.10 2000/07/28 20:08:21 scw Exp $

DEFS= -DSUN_BOOTPARAMS

SRCS=   boot.c conf.c devopen.c dev_net.c
SRCS+=	if_ie.c if_le.c
PROG= netboot.tmp
LIBS= ${LIBSA} ${LIBBUG}
CLEANFILES+= netboot.bin

netboot.bin: ${PROG}
	@size ${PROG}
	dd ibs=32 skip=1 if=netboot.tmp of=$@

.include "../Makefile.booters"

all realall: netboot.bin

install: netboot.bin
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	   -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} netboot.bin ${DESTDIR}${MDEC_DIR}/netboot

.include <bsd.prog.mk>
