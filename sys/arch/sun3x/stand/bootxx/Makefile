#	$NetBSD: Makefile,v 1.1.1.1 1997/03/13 16:27:27 gwr Exp $

SA_PROG= bootxx
SRCS=   bootxx.c conf.c

all:	${SA_PROG}

# Do not strip or remove a.out header for this one.
install: ${SA_PROG}
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	  ${SA_PROG} ${DESTDIR}${MDEC_DIR}/${SA_PROG}

.include <bsd.prog.mk>

