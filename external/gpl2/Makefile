#	$NetBSD: Makefile,v 1.13.8.1 2017/07/18 17:32:50 snj Exp $

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

.if ${MKDTC} != "no"
SUBDIR+=	dtc
.endif

SUBDIR+= gettext diffutils texinfo rcs send-pr

.include <bsd.subdir.mk>
