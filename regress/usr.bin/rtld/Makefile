# $NetBSD: Makefile,v 1.3 2009/12/14 12:43:48 uebayasi Exp $

.include <bsd.own.mk>

SUBDIR+= dlopen
.if ${MKCXX} != "no"
SUBDIR+= testlib
.endif
SUBDIR+= dlerror-false dlerror-cleared

.include <bsd.subdir.mk>
