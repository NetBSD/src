#	$NetBSD: Makefile,v 1.3 1996/12/22 20:46:07 cgd Exp $
# Makefile for g++ library genclass 

BINDIR?= /usr/bin

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/genclass.sh ${DESTDIR}${BINDIR}/genclass

.include <bsd.prog.mk>
