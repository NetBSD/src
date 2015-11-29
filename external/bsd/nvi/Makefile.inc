#	$NetBSD: Makefile.inc,v 1.4 2015/11/29 17:09:33 christos Exp $

.include <bsd.own.mk>

DIST= ${NETBSDSRCDIR}/external/bsd/nvi/dist
.PATH: ${DIST}
BINDIR=/usr/bin

CWARNFLAGS.clang+=	-Wno-error=unused-const-variable
VERSION=1.81.6-2013-11-20nb3
