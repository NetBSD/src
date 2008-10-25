#	$NetBSD: Makefile,v 1.15 2008/10/25 22:27:35 apb Exp $

PROG=		c++filt
CPPFLAGS+=	-DMAIN
CLEANFILES+=	c++filt.1

.include <bsd.own.mk>

.include "${.CURDIR}/../common/Makefile.prog"

.PATH: ${DIST}/libiberty

c++filt.1: ${DIST}/binutils/doc/cxxfilt.man
	${_MKTARGET_CREATE}
	${TOOL_SED} -e "s/@PROGRAM@/${PROG}/" < $> > $@
