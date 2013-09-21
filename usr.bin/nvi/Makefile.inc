#	$NetBSD: Makefile.inc,v 1.3 2013/09/21 22:28:12 joerg Exp $

.include <bsd.own.mk>

DIST= ${NETBSDSRCDIR}/dist/nvi
.PATH: ${DIST}

CWARNFLAGS.clang+=	-Wno-error=unused-const-variable
