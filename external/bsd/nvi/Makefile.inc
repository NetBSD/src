#	$NetBSD: Makefile.inc,v 1.3 2015/11/28 13:20:02 christos Exp $

.include <bsd.own.mk>

DIST= ${NETBSDSRCDIR}/external/bsd/nvi/dist
.PATH: ${DIST}
BINDIR=/usr/bin

CWARNFLAGS.clang+=	-Wno-error=unused-const-variable
VERSION=1.81.6-2013-11-20nb2
