#	$NetBSD: Makefile,v 1.14 1997/05/07 17:29:05 mycroft Exp $

SUBDIR= boot bootxx installboot

### find out what to use for libkern and libsa
.include "Makefile.inc"
.include "$S/lib/libsa/Makefile.inc"
.include "$S/lib/libkern/Makefile.inc"

all: ${SALIB} ${KERNLIB}

libdep:
	@echo  ${SALIB} ${KERNLIB}

sadep:
	@echo  ${SALIB}

kernlibdir:
	@echo  ${KERNDST}

salibdir:
	@echo  ${SADST}

BINDIR=/usr/mdec

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/binstall.sh ${DESTDIR}${BINDIR}/binstall

.include <bsd.subdir.mk>
