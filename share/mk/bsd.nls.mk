#	$NetBSD: bsd.nls.mk,v 1.24 2000/06/06 09:53:30 mycroft Exp $

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

nlsinstall:: ${DESTDIR}${NLSDIR}
.PRECIOUS:: ${DESTDIR}${NLSDIR}
.PHONY:: ${DESTDIR}${NLSDIR}

${DESTDIR}${NLSDIR}:
	@if [ ! -d ${.TARGET} ] || [ -h ${.TARGET} ] ; then \
		echo creating ${.TARGET}; \
		/bin/rm -rf ${.TARGET}; \
		${INSTALL} ${INSTPRIV} -d -o ${NLSOWN} -g ${NLSGRP} -m 755 \
		    ${.TARGET}; \
	fi

nlsinstall:: ${NLSALL:@F@${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat@}
.PRECIOUS: ${NLSALL:@F@${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat@}
.if !defined(UPDATE)
.PHONY: ${NLSALL:@F@${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat@}
.endif

__nlsinstall: .USE
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${INSTPRIV} -o ${NLSOWN} \
	    -g ${NLSGRP} -m ${NLSMODE} ${.ALLSRC} ${.TARGET}

.for F in ${NLSALL}
.if !defined(BUILD) && !make(all) && !make(${F})
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: .MADE
.endif
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: ${F} __nlsinstall
.endfor
.else
cleannls:
.endif

.if !target(nlsinstall)
nlsinstall::
.endif
