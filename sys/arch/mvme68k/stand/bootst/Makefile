#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.12.4.2 2002/03/16 15:58:57 jdolecek Exp $

CLEANFILES+=stboot bootst bootst.bug

PROG=bootst.bug
SRCS=   boot.c conf.c dev_tape.c rawfs.c
LIBS=	${LIBSA} ${LIBBUG}
SRTOBJ=
DPADD=	${LIBS} ${WRTVID}

bootst stboot: ${PROG} ${WRTVID}
	@${SIZE} bootst.bug
	@${STRIP} bootst.bug
	@${OBJCOPY} --output-target=binary bootst.bug bootst
	@${WRTVID} bootst	# creates stboot

proginstall::
	${INSTALL_FILE} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		bootst ${DESTDIR}${MDEC_DIR}
	${INSTALL_FILE} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		stboot ${DESTDIR}${MDEC_DIR}

.include "../Makefile.booters"

all realall: bootst
.include <bsd.prog.mk>
