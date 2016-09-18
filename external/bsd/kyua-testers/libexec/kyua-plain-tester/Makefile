# $NetBSD: Makefile,v 1.1.14.1 2016/09/18 06:14:01 snj Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/kyua-testers/dist
.PATH:		${SRCDIR}

BINDIR=		/usr/libexec
PROG_CXX=	kyua-plain-tester
SRCS=		plain_main.c
MAN=		kyua-plain-tester.1
MAN+=		kyua-plain-interface.7

TESTER_LIBDIR!=		cd ${.CURDIR}/../../lib/libtester; ${PRINTOBJDIR}
LDADD+=		${TESTER_LIBDIR}/libtester.a
DPADD+=		${TESTER_LIBDIR}/libtester.a

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${.CURDIR}/../../lib/libtester
CPPFLAGS+=	-I${SRCDIR}

WARNS?=		3

CLEANFILES+=	kyua-plain-tester.1
kyua-plain-tester.1:	kyua-plain-tester.1.in
	sed -e 's,__PACKAGE__,kyua-testers,g' \
	    -e 's,__TMPDIR__,/tmp,g' \
	    <${.ALLSRC} >${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

.include <bsd.prog.mk>
