#	$NetBSD: Makefile,v 1.8 2000/07/24 18:39:47 jdolecek Exp $

DEFS= -DSUN_BOOTPARAMS

SRCS=   boot.c conf.c devopen.c dev_net.c
SRCS+=	if_ie.c if_le.c
PROG= netboot.tmp
LIBS= ${LIBSA} ${LIBBUG}

netboot.bin: ${PROG}
	@size ${PROG}
	dd ibs=32 skip=1 if=netboot.tmp of=$@

.include "../Makefile.booters"

all: netboot.bin

install: netboot.bin
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	   -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} netboot.bin ${DESTDIR}${MDEC_DIR}/netboot

.include <bsd.prog.mk>
