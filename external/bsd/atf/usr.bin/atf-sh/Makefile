# $NetBSD: Makefile,v 1.8 2013/02/19 15:35:43 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-sh

BINDIR=		/usr/bin
PROG_CXX=	atf-sh
SRCS=		atf-sh.cpp
MAN=		atf-sh.1 atf-sh-api.3

LDADD+=		-latf-c++ -latf-c
DPADD+=		${LIBATF_CXX} ${LIBATF_C}

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}

WARNS?=		2

.if ${MKSHARE} != "no"
FILESMODE=	444

FILES+=				libatf-sh.subr
FILESDIR_libatf-sh.subr=	/usr/share/atf

FILES+=			atf-sh.pc
FILESDIR_atf-sh.pc=	/usr/lib/pkgconfig

realall: atf-sh.pc
atf-sh.pc: Makefile atf-sh.pc.in
	${TOOL_SED} -e 's,__ATF_VERSION__,0.17,g' \
	    -e 's,__EXEC_PREFIX__,/usr,g' \
	    <${SRCDIR}/atf-sh/atf-sh.pc.in >atf-sh.pc
CLEANFILES+=	atf-sh.pc
.endif

.include <bsd.prog.mk>
