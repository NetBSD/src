#	$NetBSD: Makefile,v 1.13 2003/10/20 01:15:03 lukem Exp $

PROG=		c++filt
CPPFLAGS+=	-DMAIN
CLEANFILES+=	c++filt.1

.include <bsd.own.mk>

.include "${.CURDIR}/../common/Makefile.prog"

.PATH: ${DIST}/libiberty

c++filt.1: ${DIST}/binutils/doc/cxxfilt.man
	${_MKMSGCREATE}
	${_MKCMD}\
	sed -e "s/@PROGRAM@/${PROG}/" < $> > $@
