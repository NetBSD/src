#	$NetBSD: bsd.nls.mk,v 1.32 2001/11/02 18:10:00 tv Exp $

.include <bsd.init.mk>

##### Basic targets
.PHONY:		cleannls nlsinstall
cleandir:	cleannls
realinstall:	nlsinstall

##### Default values
GENCAT?=	gencat
NLSNAME?=	${PROG:Ulib${LIB}}

NLS?=

##### Build rules
.if ${MKNLS} != "no"

NLSALL=		${NLS:.msg=.cat}

realall:	${NLSALL}
.NOPATH:	${NLSALL}

.SUFFIXES: .cat .msg

.msg.cat:
	@rm -f ${.TARGET}
	${GENCAT} ${.TARGET} ${.IMPSRC}

.endif # ${MKNLS} != "no"

##### Install rules
nlsinstall::	# ensure existence
.if ${MKNLS} != "no"

nlsinstall::	${DESTDIR}${NLSDIR}
.PRECIOUS:	${DESTDIR}${NLSDIR}
.PHONY:		${DESTDIR}${NLSDIR}

${DESTDIR}${NLSDIR}:
	@if [ ! -d ${.TARGET} ] || [ -h ${.TARGET} ] ; then \
		echo creating ${.TARGET}; \
		/bin/rm -rf ${.TARGET}; \
		${INSTALL_DIR} -o ${NLSOWN} -g ${NLSGRP} -m 755 ${.TARGET}; \
	fi

__nlsinstall: .USE
	${INSTALL_DIR} -o ${NLSOWN} -g ${NLSGRP} ${.TARGET:H}
	${INSTALL_FILE} -o ${NLSOWN} -g ${NLSGRP} -m ${NLSMODE} \
		${.ALLSRC} ${.TARGET}

.for F in ${NLSALL:O:u}
_F:=		${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat # installed path

${_F}:		${F} __nlsinstall			# install rule
nlsinstall::	${_F}
.PRECIOUS:	${_F}					# keep if install fails
.PHONY:		${UPDATE:U${_F}}			# noclobber install
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
.endfor

.undef _F
.endif # ${MKNLS} != "no"

##### Clean rules
cleannls:
.if !empty(NLS)
	rm -f ${NLSALL}
.endif
