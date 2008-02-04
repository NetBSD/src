# $NetBSD: Makefile,v 1.2 2008/02/04 20:30:45 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/data ${SRCDIR}/subrs

.if ${MKSHARE} != "no"
# Files in ${SRCDIR}/data.
FILES=			atf.pc
FILESDIR_atf.pc=	/usr/lib/pkgconfig

# Files in ${SRCDIR}/subrs.
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES+=		atf.footer.subr \
		atf.header.subr \
		atf.init.subr
.endif

realall: atf.pc
atf.pc: atf.pc.in
	sed -e 's,__CXX__,${CXX},g' \
	    -e 's,__INCLUDEDIR__,/usr/include,g' \
	    -e 's,__LIBDIR__,/usr/lib,g' \
	    <${SRCDIR}/data/atf.pc.in >atf.pc
CLEANFILES+=	atf.pc

.include <bsd.prog.mk>
