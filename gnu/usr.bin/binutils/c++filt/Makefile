#	$NetBSD: Makefile,v 1.11 2002/09/16 16:22:17 thorpej Exp $

PROG=		c++filt
CPPFLAGS+=	-DMAIN
CLEANFILES+=	c++filt.1

.include <bsd.own.mk>

.include "${.CURDIR}/../common/Makefile.prog"

.PATH: ${DIST}/libiberty

c++filt.1: ${DIST}/binutils/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < $> > $@
