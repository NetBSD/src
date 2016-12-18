#	$NetBSD: Makefile.inc,v 1.1.6.1 2016/12/18 06:37:30 snj Exp $

.include <bsd.own.mk>

DIST= ${NETBSDSRCDIR}/external/bsd/nvi/dist
.PATH: ${DIST}
BINDIR=/usr/bin

CWARNFLAGS.clang+=	-Wno-error=unused-const-variable
VERSION=1.81.6-2013-11-20nb3
