#	$NetBSD: Makefile,v 1.1.2.1 2002/01/10 19:49:53 thorpej Exp $

SA_PROG= bootxx
SRCS=   bootxx.c conf.c

# Do not strip or remove a.out header for this one.
proginstall: ${SA_PROG}
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	${SA_PROG} ${DESTDIR}${MDEC_DIR}/${SA_PROG}

PROG=	${SA_PROG}
NOMAN= # defined

.include <bsd.prog.mk>

