#	$NetBSD: Makefile,v 1.2 1995/07/03 02:53:55 gwr Exp $

SA_PROG= netboot
SRCS=   boot.c conf.c version.c dev_net.c

DEFS= -DSTANDALONE -DSUN_BOOTPARAMS

all:	${SA_PROG}.bin

.include <bsd.prog.mk>

install: ${SA_PROG}.bin
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	  ${SA_PROG}.bin ${DESTDIR}${MDEC_DIR}/${SA_PROG}

