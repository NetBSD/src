# $NetBSD: Makefile,v 1.2.44.1 2009/08/26 03:34:33 matt Exp $

.include <bsd.own.mk>

SUBDIR+= dlopen
.if ${MKCXX} != "no"
SUBDIR+= testlib
.endif
SUBDIR+= dlerror-false dlerror-cleared

.include <bsd.subdir.mk>
