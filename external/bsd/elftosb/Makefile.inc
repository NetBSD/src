# $NetBSD: Makefile.inc,v 1.2.26.1 2019/06/10 21:44:44 christos Exp $

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
CXXFLAGS+=	-std=gnu++03

CWARNFLAGS+=	-Wno-multichar
CWARNFLAGS.clang+=	-Wno-switch -Wno-error=delete-non-virtual-dtor \
			-Wno-bitwise-op-parentheses -Wno-parentheses \
			-Wno-tautological-compare

LIBISCXX=	yes

BINDIR=		/usr/sbin

