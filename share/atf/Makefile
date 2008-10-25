# $NetBSD: Makefile,v 1.4 2008/10/25 22:27:35 apb Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/data ${SRCDIR}/atf-sh

.if ${MKSHARE} != "no"
# Files in ${SRCDIR}/data.
FILES=			atf-c.pc \
			atf-c++.pc
FILESDIR_atf-c.pc=	/usr/lib/pkgconfig
FILESDIR_atf-c++.pc=	/usr/lib/pkgconfig

# Files in ${SRCDIR}/subrs.
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES+=		atf.footer.subr \
		atf.header.subr \
		atf.init.subr
.endif

realall: atf-c.pc
atf-c.pc: atf-c.pc.in
	${TOOL_SED} -e 's,__ATF_VERSION__,0.5,g' \
	    -e 's,__CC__,gcc,g' \
	    -e 's,__INCLUDEDIR__,/usr/include,g' \
	    -e 's,__LIBDIR__,/usr/lib,g' \
	    <${SRCDIR}/data/atf-c.pc.in >atf-c.pc
CLEANFILES+=	atf-c.pc

realall: atf-c++.pc
atf-c++.pc: atf-c++.pc.in
	${TOOL_SED} -e 's,__ATF_VERSION__,0.5,g' \
	    -e 's,__CXX__,g++,g' \
	    -e 's,__INCLUDEDIR__,/usr/include,g' \
	    -e 's,__LIBDIR__,/usr/lib,g' \
	    <${SRCDIR}/data/atf-c++.pc.in >atf-c++.pc
CLEANFILES+=	atf-c++.pc

.include <bsd.prog.mk>
