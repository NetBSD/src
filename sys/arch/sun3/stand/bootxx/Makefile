#	$NetBSD: Makefile,v 1.8 2001/02/13 03:08:54 jmc Exp $

SA_PROG= bootxx
SRCS=   bootxx.c conf.c

# Do not strip or remove a.out header for this one.
proginstall: ${SA_PROG}
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	${SA_PROG} ${DESTDIR}${MDEC_DIR}/${SA_PROG}

PROG=	${SA_PROG}
MKMAN= no
NOMAN= yes

.include <bsd.prog.mk>

