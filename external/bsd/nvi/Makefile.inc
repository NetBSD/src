#	$NetBSD: Makefile.inc,v 1.5.10.1 2018/07/28 04:37:14 pgoyette Exp $

.include <bsd.own.mk>

SANITIZER_RENAME_SYMBOL+=	regcomp regerror regexec regfree

DIST= ${NETBSDSRCDIR}/external/bsd/nvi/dist
.PATH: ${DIST}
BINDIR=/usr/bin

CWARNFLAGS.clang+=	-Wno-error=unused-const-variable
VERSION=1.81.6-2013-11-20nb4
