#	$NetBSD: Makefile,v 1.2 1998/05/22 02:40:27 tv Exp $

PROG=	c++filt
SRCS=	cplus-dem.c underscore.c

CPPFLAGS+=-DMAIN -DVERSION='"${VERSION}"'

CLEANFILES+=underscore.c

underscore.c: .PHONY
	sh ${.CURDIR}/mkunderscore.sh ${TARGET_ARCH}-unknown-netbsd ${TOP}/dist/bfd/config.bfd > \
	    ${.TARGET}

.include <bsd.prog.mk>

.PATH:	${dist}/libiberty

${PROG}.1: ${srcdir}/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < ${srcdir}/cxxfilt.man > ${.TARGET}
