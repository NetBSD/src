#	$NetBSD: Makefile,v 1.1.1.1 1995/06/09 22:02:39 gwr Exp $

SA_PROG= netboot
SRCS=   boot.c conf.c version.c dev_net.c

# XXX - These are temporary, until libsa merge is done...
DEFS= -DSTANDALONE -DSUN_BOOTPARAMS
INCL= -I. -I../libnet -I../libsa -I${S}/lib/libsa -I${S}
LIBS= ../libnet/libnet.a ../libsa/libsa.a

all:	${SA_PROG}.bin

.include <bsd.prog.mk>

install: ${SA_PROG}.bin
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	  ${SA_PROG}.bin ${DESTDIR}${MDEC_DIR}/${SA_PROG}

