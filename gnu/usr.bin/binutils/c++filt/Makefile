#	$NetBSD: Makefile,v 1.1.1.1 1997/12/18 07:37:38 thorpej Exp $

PROG=	c++filt
SRCS=	cplus-dem.c underscore.c

CPPFLAGS+=-DMAIN -DVERSION='"${VERSION}"'

CLEANFILES+=underscore.c

underscore.c: .PHONY
	sh ${.CURDIR}/mkunderscore.sh ${TARGET} ${TOP}/dist/bfd/config.bfd > \
	    ${.TARGET}

.include <bsd.prog.mk>

.PATH:	${dist}/libiberty

${PROG}.1: ${srcdir}/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < ${srcdir}/cxxfilt.man > ${.TARGET}
