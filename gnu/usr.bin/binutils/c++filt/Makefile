#	$NetBSD: Makefile,v 1.5 1998/07/29 19:18:26 thorpej Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	c++filt
SRCS=	cplus-dem.c underscore.c
.endif

MAN=	c++filt.1

CPPFLAGS+=-DMAIN -DVERSION='"${VERSION}"'

CLEANFILES+=underscore.c ${PROG}.1

.include <bsd.prog.mk>

underscore.c: mkunderscore.sh ${dist}/bfd/config.bfd
	sh ${.CURDIR}/mkunderscore.sh ${MACHINE_GNU_ARCH}-unknown-netbsd \
		${dist}/bfd/config.bfd > ${.TARGET}

.PATH:	${dist}/libiberty

${PROG}.1: ${srcdir}/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < ${srcdir}/cxxfilt.man > ${.TARGET}
