#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.5.16.1 2000/11/20 20:15:27 bouyer Exp $

CLEANFILES+=stboot bootst bootst.bug

PROG=bootst.bug
SRCS=   boot.c conf.c dev_tape.c rawfs.c
LIBS=	${LIBSA} ${LIBBUG}
SRTOBJ=
DPADD=	${WRTVID}

bootst stboot: ${PROG} ${WRTVID}
	@size bootst.bug
	@strip bootst.bug
	@${WRTVID} bootst.bug # creates bootst  and stboot

proginstall::
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	   -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} bootst ${DESTDIR}${MDEC_DIR}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	   -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} stboot ${DESTDIR}${MDEC_DIR}

.include "../Makefile.booters"

all realall: bootst
.include <bsd.prog.mk>
