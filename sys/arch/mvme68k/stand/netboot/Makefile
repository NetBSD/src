#	$NetBSD: Makefile,v 1.11 2000/11/30 22:26:27 scw Exp $

DEFS= -DSUN_BOOTPARAMS

SRCS=   boot.c conf.c devopen.c dev_net.c
SRCS+=	if_ie.c if_le.c
PROG= netboot.tmp
LIBS= ${LIBSA} ${LIBBUG}
CLEANFILES+= netboot.bin


netboot.bin: ${PROG}
	${OBJCOPY} -O binary ${.OBJDIR}/netboot.tmp $@

.include "../Makefile.booters"

all realall: netboot.bin

install: netboot.bin
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	   -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.OBJDIR}/netboot.bin \
	   ${DESTDIR}${MDEC_DIR}/netboot

.include <bsd.prog.mk>
