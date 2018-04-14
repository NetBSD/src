#	$NetBSD: Makefile,v 1.6 2018/04/14 17:53:09 christos Exp $

PROG=		c++filt
CPPFLAGS+=	-DMAIN
CLEANFILES+=	c++filt.1

.include "${.CURDIR}/../common/Makefile.prog"

.PATH: ${DIST}/libiberty

c++filt.1: ${DIST}/binutils/doc/cxxfilt.man
	${_MKTARGET_CREATE}
	${TOOL_SED} -e "s/@PROGRAM@/${PROG}/" < $> > $@
