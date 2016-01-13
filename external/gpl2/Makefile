#	$NetBSD: Makefile,v 1.10 2016/01/13 19:02:29 christos Exp $

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

.if ${MKGROFF} != "no"
SUBDIR+=        groff
.endif

SUBDIR+= gettext diffutils

.include <bsd.subdir.mk>
