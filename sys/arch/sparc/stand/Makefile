#	$NetBSD: Makefile,v 1.13 1997/01/24 01:18:22 pk Exp $

SUBDIR= boot bootxx installboot

### find out what to use for libkern and libsa
.include "Makefile.inc"
.include "$S/lib/libsa/Makefile.inc"
.include "$S/lib/libkern/Makefile.inc"

all: ${SALIB} ${KERNLIB} _SUBDIRUSE

libdep:
	@echo  ${SALIB} ${KERNLIB}

sadep:
	@echo  ${SALIB}

kernlibdir:
	@echo  ${KERNDST}

salibdir:
	@echo  ${SADST}

clean: _SUBDIRUSE

cleandir: _SUBDIRUSE

BINDIR=/usr/mdec

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/binstall.sh ${DESTDIR}${BINDIR}/binstall

.include <bsd.subdir.mk>
