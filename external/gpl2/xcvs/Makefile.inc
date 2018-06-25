#	$NetBSD: Makefile.inc,v 1.5 2018/06/25 18:03:56 kamil Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server

WARNS?=	0

CWARNFLAGS.clang+=	-Wno-tautological-compare -Wno-parentheses \
			-Wno-format-security -Wno-format \
			-Wno-tautological-constant-out-of-range-compare

BINDIR?= /usr/bin
IDIST:=	${.PARSEDIR}/dist

.if !defined(SUBDIR)
CPPFLAGS+= -DSETXID_SUPPORT -DHAVE_CONFIG_H \
	-I${.CURDIR}/../../include -I${IDIST}/lib \
	-I${IDIST}/diff -I${IDIST}/src

DOBJDIR!=cd $(.CURDIR)/../../lib/libdiff && ${PRINTOBJDIR}
COBJDIR!=cd $(.CURDIR)/../../lib/libcvs && ${PRINTOBJDIR}

LIBDIFF=	${DOBJDIR}/libdiff.a
LIBCVS=		${COBJDIR}/libcvs.a
.endif

SANITIZER_RENAME_SYMBOL+=	regcomp regerror regexec regfree
