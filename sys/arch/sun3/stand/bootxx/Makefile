#	$NetBSD: Makefile,v 1.5 1997/09/05 04:31:52 gwr Exp $

SA_PROG= bootxx
SRCS=   bootxx.c conf.c

all:	${SA_PROG}

# Do not strip or remove a.out header for this one.
proginstall: ${SA_PROG}
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	  ${SA_PROG} ${DESTDIR}${MDEC_DIR}/${SA_PROG}

.include <bsd.prog.mk>

