#	$NetBSD: bsd.nls.mk,v 1.1 1995/04/20 05:09:34 jtc Exp $

.if !target(.MAIN)
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.MAIN: all
.endif

.SUFFIXES: .cat .msg

.msg.cat:
	@rm -f ${.TARGET}
	gencat ${.TARGET} ${.IMPSRC}

NLSALL= ${NLS:.msg=.cat}

.if !defined(NLSNAME)
.if defined(PROG)
NLSNAME=${PROG}
.else
NLSNAME=lib${LIB}
.endif
.endif

nlsinstall:
.if defined(NLSALL)
	@for msg in ${NLSALL}; do \
		NLSLANG=`basename $$msg .cat`; \
		dir=${DESTDIR}${NLSDIR}/$${NLSLANG}; \
		install -d $$dir; \
		install ${COPY} -o ${NLSOWN} -g ${NLSGRP} -m ${NLSMODE} $$msg $$dir/${NLSNAME}.cat; \
	done
.endif

.if defined(NLSALL)
all: ${NLSALL}

install:  nlsinstall

cleandir: cleannls
cleannls:
	rm -f ${NLSALL}
.endif
