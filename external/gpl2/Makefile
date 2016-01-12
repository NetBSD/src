#	$NetBSD: Makefile,v 1.8 2016/01/12 22:58:52 christos Exp $

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

SUBDIR+= gettext

.include <bsd.subdir.mk>
