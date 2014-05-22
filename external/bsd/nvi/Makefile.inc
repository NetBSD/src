#	$NetBSD: Makefile.inc,v 1.1.4.2 2014/05/22 15:50:32 yamt Exp $

.include <bsd.own.mk>

DIST= ${NETBSDSRCDIR}/external/bsd/nvi/dist
.PATH: ${DIST}
BINDIR=/usr/bin

CWARNFLAGS.clang+=	-Wno-error=unused-const-variable
VERSION=1.81.6-2013-11-20
