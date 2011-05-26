#	$NetBSD: Makefile.inc,v 1.2 2011/05/26 12:56:27 joerg Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server

WARNS?=	0

CWARNFLAGS.clang+=	-Wno-tautological-compare -Wno-parentheses \
			-Wno-format-security -Wno-format

BINDIR?= /usr/bin
IDIST=	${NETBSDSRCDIR}/external/gpl2/xcvs/dist

.if !defined(SUBDIR)
CPPFLAGS+= -DSETXID_SUPPORT -DHAVE_CONFIG_H \
	-I${.CURDIR}/../../include -I${IDIST}/lib \
	-I${IDIST}/diff -I${IDIST}/src

DOBJDIR!=cd $(.CURDIR)/../../lib/libdiff && ${PRINTOBJDIR}
COBJDIR!=cd $(.CURDIR)/../../lib/libcvs && ${PRINTOBJDIR}

LIBDIFF=	${DOBJDIR}/libdiff.a
LIBCVS=		${COBJDIR}/libcvs.a
.endif
