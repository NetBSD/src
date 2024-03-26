#	$NetBSD: Makefile,v 1.2 2024/03/26 16:52:03 christos Exp $

.include <bsd.hostinit.mk>

SRCDIR=	${.CURDIR}/../../../../external/bsd/elftoolchain/dist/common/sys

.PATH:	${SRCDIR}

INCS=		elfdefinitions.h
HOST_INCSDIR=	${TOOLDIR}/include/sys

elfdefinitions.h:	elfdefinitions.m4 elfconstants.m4
	${TOOL_M4} -I${SRCDIR} -D SRCDIR=${SRCDIR} ${M4FLAGS} \
		elfdefinitions.m4 > ${.TARGET}

.PHONY:	all clean clobber depend

install: .PHONY ${HOST_INCSDIR} .WAIT includes

${HOST_INCSDIR}:
	${_MKTARGET_INSTALL}
	${HOST_INSTALL_DIR} ${HOST_INCSDIR}

.for _f in ${INCS}
HOST_INCINSTFILES+= ${HOST_INCSDIR}/${_f}
${HOST_INCSDIR}/${_f}: ${_f}
	${_MKTARGET_INSTALL}
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}
.endfor

includes: .PHONY ${HOST_INCSDIR} .WAIT ${HOST_INCINSTFILES}

all dependall depend:	${INCS}

clean clobber:
	rm -f ${INCS}

cleandepend:
	rm -f ${.OBJDIR}/.depend

.include <bsd.obj.mk>
