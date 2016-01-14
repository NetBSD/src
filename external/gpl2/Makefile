#	$NetBSD: Makefile,v 1.11 2016/01/14 00:38:30 christos Exp $

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

SUBDIR+= gettext diffutils texinfo

.include <bsd.subdir.mk>
