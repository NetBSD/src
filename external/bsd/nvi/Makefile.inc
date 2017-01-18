#	$NetBSD: Makefile.inc,v 1.1.12.1 2017/01/18 08:46:20 skrll Exp $

.include <bsd.own.mk>

DIST= ${NETBSDSRCDIR}/external/bsd/nvi/dist
.PATH: ${DIST}
BINDIR=/usr/bin

CWARNFLAGS.clang+=	-Wno-error=unused-const-variable
VERSION=1.81.6-2013-11-20nb3
