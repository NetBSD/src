#	$NetBSD: Makefile,v 1.9 2016/01/13 03:40:11 christos Exp $

.include <bsd.own.mk>

.if (${MKCVS} != "no")
SUBDIR+= xcvs
.endif
.if (${MKLVM} != "no")
SUBDIR+= lvm2
.endif

.if ${MKBSDGREP} == "no"
SUBDIR+= grep
.endif

SUBDIR+= gettext diffutils

.include <bsd.subdir.mk>
