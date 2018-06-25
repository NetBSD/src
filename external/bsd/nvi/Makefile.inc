#	$NetBSD: Makefile.inc,v 1.6 2018/06/25 18:00:34 kamil Exp $

.include <bsd.own.mk>

SANITIZER_RENAME_SYMBOL+=	regcomp regerror regexec regfree

DIST= ${NETBSDSRCDIR}/external/bsd/nvi/dist
.PATH: ${DIST}
BINDIR=/usr/bin

CWARNFLAGS.clang+=	-Wno-error=unused-const-variable
VERSION=1.81.6-2013-11-20nb4
