#	$NetBSD: Makefile,v 1.3 1998/05/28 20:52:26 cgd Exp $

PROG=	c++filt
SRCS=	cplus-dem.c underscore.c

CPPFLAGS+=-DMAIN -DVERSION='"${VERSION}"'

CLEANFILES+=underscore.c ${PROG}.1

underscore.c: .PHONY
	sh ${.CURDIR}/mkunderscore.sh ${TARGET_ARCH}-unknown-netbsd ${TOP}/dist/bfd/config.bfd > \
	    ${.TARGET}

.include <bsd.prog.mk>

.PATH:	${dist}/libiberty

${PROG}.1: ${srcdir}/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < ${srcdir}/cxxfilt.man > ${.TARGET}
