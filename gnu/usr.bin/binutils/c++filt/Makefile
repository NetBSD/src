#	$NetBSD: Makefile,v 1.4 1998/07/25 18:58:43 tv Exp $

PROG=	c++filt
SRCS=	cplus-dem.c underscore.c

CPPFLAGS+=-DMAIN -DVERSION='"${VERSION}"'

CLEANFILES+=underscore.c ${PROG}.1

.include <bsd.prog.mk>

underscore.c: mkunderscore.sh ${dist}/bfd/config.bfd
	sh ${.CURDIR}/mkunderscore.sh ${MACHINE_GNU_ARCH}-unknown-netbsd \
		${dist}/bfd/config.bfd > ${.TARGET}

.PATH:	${dist}/libiberty

${PROG}.1: ${srcdir}/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < ${srcdir}/cxxfilt.man > ${.TARGET}
