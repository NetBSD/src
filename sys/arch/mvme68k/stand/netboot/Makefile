#	$NetBSD: Makefile,v 1.4.16.2 2000/12/08 09:28:44 bouyer Exp $

DEFS= -DSUN_BOOTPARAMS

SRCS=   boot.c conf.c devopen.c dev_net.c
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
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	   -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.OBJDIR}/netboot.bin \
	   ${DESTDIR}${MDEC_DIR}/netboot

.include <bsd.prog.mk>
