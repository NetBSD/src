# $NetBSD: Makefile.inc,v 1.1.2.2 2012/11/15 19:49:13 jkunz Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/external/bsd/elftosb/dist

.PATH:		${DIST}/common

LIBELFTOSBOBJDIR!=	cd ${DIST}/../lib && ${PRINTOBJDIR}

.ifndef HOSTPROG
DPADD+=		${LIBELFTOSBOBJDIR}/libelftosb.a
LDADD+=		-L${LIBELFTOSBOBJDIR} -lelftosb
.endif

CPPFLAGS+=	-I${DIST}/common
CPPFLAGS+=	-I${DIST}/elftosb2
CPPFLAGS+=	-DLinux

CWARNFLAGS+=	-Wno-multichar

LIBISCXX=	yes

BINDIR=		/usr/sbin

