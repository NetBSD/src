#	$NetBSD: Makefile,v 1.12 2003/03/05 07:19:13 mrg Exp $

PROG=		c++filt
CPPFLAGS+=	-DMAIN
CLEANFILES+=	c++filt.1

.include <bsd.own.mk>

.include "${.CURDIR}/../common/Makefile.prog"

.PATH: ${DIST}/libiberty

c++filt.1: ${DIST}/binutils/doc/cxxfilt.man
	sed -e "s/@PROGRAM@/${PROG}/" < $> > $@
