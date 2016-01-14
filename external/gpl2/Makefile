#	$NetBSD: Makefile,v 1.12 2016/01/14 04:21:48 christos Exp $

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

SUBDIR+= gettext diffutils texinfo rcs

.include <bsd.subdir.mk>
