#	$NetBSD: Makefile,v 1.4 1996/10/18 06:09:43 thorpej Exp $

SA_PROG= bootxx
SRCS=   bootxx.c conf.c

all:	${SA_PROG}

# Do not strip or remove a.out header for this one.
install: ${SA_PROG}
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	  ${SA_PROG} ${DESTDIR}${MDEC_DIR}/${SA_PROG}

.include <bsd.prog.mk>

