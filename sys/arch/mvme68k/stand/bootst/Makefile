#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.9 2000/07/24 18:39:14 jdolecek Exp $

CLEANFILES+=stboot bootst bootst.bug

PROG=bootst.bug
SRCS=   boot.c conf.c dev_tape.c rawfs.c
LIBS=	${LIBSA} ${LIBBUG}
SRTOBJ=
DPADD=	${WRTVID}

bootst stboot: ${PROG} ${WRTVID}
	@size bootst.bug
	@${WRTVID} bootst.bug # creates bootst  and stboot

proginstall::
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	   -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} bootst ${DESTDIR}${MDEC_DIR}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	   -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} stboot ${DESTDIR}${MDEC_DIR}

.include "../Makefile.booters"

all: bootst
.include <bsd.prog.mk>
