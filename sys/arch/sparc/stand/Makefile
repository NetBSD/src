#	$NetBSD: Makefile,v 1.8 1995/09/30 21:43:35 pk Exp $

SUBDIR= boot bootxx installboot

### find out what to use for libkern and libsa
SAREL=
KERNREL=
.include "Makefile.inc"
.include "$S/lib/libsa/Makefile.inc"
.include "$S/lib/libkern/Makefile.inc"

all: ${SALIB} ${KERNLIB} _SUBDIRUSE

libdep:
	@echo  ${.OBJDIR}/${SALIB} ${.OBJDIR}/${KERNLIB}

clean:: _SUBDIRUSE

cleandir: _SUBDIRUSE

.include <bsd.subdir.mk>
