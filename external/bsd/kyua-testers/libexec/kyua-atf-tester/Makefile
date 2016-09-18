# $NetBSD: Makefile,v 1.1.14.1 2016/09/18 06:14:00 snj Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/kyua-testers/dist
.PATH:		${SRCDIR}

BINDIR=		/usr/libexec
PROG_CXX=	kyua-atf-tester
SRCS=		atf_main.c
MAN=		kyua-atf-tester.1
MAN+=		kyua-atf-interface.7

ATFTESTER_LIBDIR!=	cd ${.CURDIR}/../../lib/libatf_tester; ${PRINTOBJDIR}
TESTER_LIBDIR!=		cd ${.CURDIR}/../../lib/libtester; ${PRINTOBJDIR}
LDADD+=		${ATFTESTER_LIBDIR}/libatf_tester.a
DPADD+=		${ATFTESTER_LIBDIR}/libatf_tester.a
LDADD+=		${TESTER_LIBDIR}/libtester.a
DPADD+=		${TESTER_LIBDIR}/libtester.a

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${.CURDIR}/../../lib/libtester
CPPFLAGS+=	-I${SRCDIR}

WARNS?=		3

CLEANFILES+=	kyua-atf-tester.1
kyua-atf-tester.1:	kyua-atf-tester.1.in
	sed -e 's,__PACKAGE__,kyua-testers,g' \
	    -e 's,__TMPDIR__,/tmp,g' \
	    <${.ALLSRC} >${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

.include <bsd.prog.mk>
