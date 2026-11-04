#	$NetBSD: Makefile,v 1.14.18.1 2026/11/04 22:41:00 perseant Exp $

.include <bsd.own.mk>

.if (${MKCVS} != "no")
SUBDIR+= xcvs
.endif
.if (${MKLVM} != "no")
SUBDIR+= lvm2
.endif

.if ${MKBSDDIFF} == "no"
SUBDIR+= diffutils
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

SUBDIR+= gettext texinfo rcs send-pr

.include <bsd.subdir.mk>
