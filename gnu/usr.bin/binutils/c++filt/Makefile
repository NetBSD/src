#	$NetBSD: Makefile,v 1.7 1999/02/02 22:05:59 tv Exp $

PROG=	c++filt
SRCS=	cplus-dem.c underscore.c
MAN=	c++filt.1

CPPFLAGS+=-DMAIN -DVERSION='"${VERSION}"'

CLEANFILES+=${PROG}.1

.include <bsd.prog.mk>

.PATH:	${dist}/libiberty

${PROG}.1: ${srcdir}/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < ${srcdir}/cxxfilt.man > ${.TARGET}
