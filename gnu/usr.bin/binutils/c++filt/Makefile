#	$NetBSD: Makefile,v 1.9 1999/02/09 19:21:54 tv Exp $

PROG=		c++filt
SRCS=		cplus-dem.c underscore.c
MAN=		c++filt.1
CPPFLAGS+=	-DMAIN
CLEANFILES+=	${MAN}

.include <bsd.prog.mk>

.PATH: ${DIST}/libiberty

${MAN}: ${DIST}/binutils/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < $> > $@
