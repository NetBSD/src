#	$NetBSD: Makefile,v 1.6.2.2 1998/11/07 00:57:01 cgd Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	c++filt
SRCS=	cplus-dem.c underscore.c
.endif

MAN=	c++filt.1

CPPFLAGS+=-DMAIN -DVERSION='"${VERSION}"'

CLEANFILES+=${PROG}.1

.include <bsd.prog.mk>

.PATH:	${dist}/libiberty

${PROG}.1: ${srcdir}/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < ${srcdir}/cxxfilt.man > ${.TARGET}
