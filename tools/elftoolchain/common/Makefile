# $NetBSD: Makefile,v 1.3 2024/03/26 16:52:03 christos Exp $

.include <bsd.hostinit.mk>

SRCDIR=	${.CURDIR}/../../../external/bsd/elftoolchain/dist/common

.PATH:	${SRCDIR}

SUBDIR=		sys

INCS=		elfdefinitions.h
HOST_INCSDIR=	${TOOLDIR}/include

HOST_INCINSTFILES=
.for _f in ${INCS}
HOST_INCINSTFILES+= ${HOST_INCSDIR}/${_f}
${HOST_INCSDIR}/${_f}: ${_f}
	${_MKTARGET_INSTALL}
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}
.endfor

.PHONY:	all clean depend obj

install: .PHONY includes

includes: .PHONY ${HOST_INCSDIR} .WAIT ${HOST_INCINSTFILES}

all depend clean clobber:

cleandepend:
	rm -f ${.OBJDIR}/.depend

.include <bsd.subdir.mk>
