#	$NetBSD: Makefile,v 1.1.2.2 2002/03/16 16:00:10 jdolecek Exp $

SA_PROG= bootxx
SRCS=   bootxx.c conf.c

# Do not strip or remove a.out header for this one.
proginstall: ${SA_PROG}
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
		${SA_PROG} ${DESTDIR}${MDEC_DIR}/${SA_PROG}

PROG=	${SA_PROG}
NOMAN= # defined

.include <bsd.prog.mk>

