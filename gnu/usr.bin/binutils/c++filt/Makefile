#	$NetBSD: Makefile,v 1.10 2001/08/06 19:12:00 tv Exp $

PROG=		c++filt
CPPFLAGS+=	-DMAIN
CLEANFILES+=	c++filt.1

.include <bsd.own.mk>
.if defined(USE_NEW_TOOLCHAIN)
.include "${.CURDIR}/../common/Makefile.prog"
.else
SRCS=		cplus-dem.c underscore.c

.include <bsd.prog.mk>
.endif

.PATH: ${DIST}/libiberty

c++filt.1: ${DIST}/binutils/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < $> > $@
