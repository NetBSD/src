#	$NetBSD: Makefile,v 1.7 1995/09/16 23:20:22 pk Exp $

SUBDIR= boot bootxx installboot

.BEGIN:
	@([ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine)

### find out what to use for libkern and libsa
.include "Makefile.inc"
.include "$S/lib/libsa/Makefile.inc"
.include "$S/lib/libkern/Makefile.inc"

all: ${SALIB} ${KERNLIB} _SUBDIRUSE

libdep:
	@echo  ${SALIB} ${KERNLIB}

clean:: _SUBDIRUSE

cleandir: _SUBDIRUSE
	rm -rf lib machine

.include <bsd.subdir.mk>
