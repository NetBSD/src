#	$NetBSD: bsd.nls.mk,v 1.17.4.1 1999/08/10 00:43:37 mcr Exp $

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.MAIN:		all
.endif
.PHONY:		cleannls nlsinstall
.if ${MKNLS} != "no"
realinstall:	nlsinstall
.endif
cleandir distclean: cleannls

.SUFFIXES: .cat .msg

.msg.cat:
	@rm -f ${.TARGET}
	gencat ${.TARGET} ${.IMPSRC}

.if defined(NLS) && !empty(NLS)
NLSALL= ${NLS:.msg=.cat}
.endif

.if !defined(NLSNAME)
.if defined(PROG)
NLSNAME=${PROG}
.else
NLSNAME=lib${LIB}
.endif
.endif

.if defined(NLSALL)
.if ${MKNLS} != "no"
all: ${NLSALL}
.endif

cleannls:
	rm -f ${NLSALL}

.for F in ${NLSALL}
nlsinstall:: ${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat
.endif
.if !defined(BUILD)
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: .MADE
.endif

.PRECIOUS: ${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: ${F}
	${INSTALL} -d ${.TARGET:H}
.if ${MORTALINSTALL} != "no"
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} \
		-m ${NLSMODE} ${.ALLSRC} ${.TARGET}
.else
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o ${NLSOWN} -g ${NLSGRP} \
		-m ${NLSMODE} ${.ALLSRC} ${.TARGET}
.endif
.endfor
.else
cleannls:
.endif

.if !target(nlsinstall)
nlsinstall::
.endif
