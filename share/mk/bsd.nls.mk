#	$NetBSD: bsd.nls.mk,v 1.10 1997/10/11 08:16:28 mycroft Exp $

.if !target(.MAIN)
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.MAIN:		all
.endif
.PHONY:		cleannls nlsinstall
.if !defined(NONLS)
realinstall:	nlsinstall
.endif
cleandir:	cleannls

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
.if !defined(NONLS)
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
	${INSTALL} ${COPY} -o ${NLSOWN} -g ${NLSGRP} -m ${NLSMODE} ${.ALLSRC} \
		${.TARGET}
.endfor
.else
cleannls:
.endif

.if !target(nlsinstall)
nlsinstall::
.endif
