#	$NetBSD: bsd.nls.mk,v 1.22 2000/06/06 05:40:47 mycroft Exp $

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
.NOPATH: ${NLSALL}
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
realall: ${NLSALL}
.endif

cleannls:
	rm -f ${NLSALL}

nlsinstall:: ${NLSALL:@F@${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat@}
.if !defined(UPDATE)
.PHONY: ${NLSALL:@F@${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat@}
.endif
.PRECIOUS: ${NLSALL:@F@${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat@}

.for F in ${NLSALL}
.if !defined(BUILD) && !make(all) && !make(${F})
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: .MADE
.endif
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: ${F}
	${INSTALL} ${INSTPRIV} -d -o ${NLSOWN} -g ${NLSGRP} ${.TARGET:H}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${INSTPRIV} -o ${NLSOWN} \
	    -g ${NLSGRP} -m ${NLSMODE} ${.ALLSRC} ${.TARGET}
.endfor
.else
cleannls:
.endif

.if !target(nlsinstall)
nlsinstall::
.endif
