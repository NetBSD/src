#	$NetBSD: Makefile,v 1.14 2003/10/21 10:01:20 lukem Exp $

PROG=		c++filt
CPPFLAGS+=	-DMAIN
CLEANFILES+=	c++filt.1

.include <bsd.own.mk>

.include "${.CURDIR}/../common/Makefile.prog"

.PATH: ${DIST}/libiberty

c++filt.1: ${DIST}/binutils/doc/cxxfilt.man
	${_MKTARGET_CREATE}
	sed -e "s/@PROGRAM@/${PROG}/" < $> > $@
