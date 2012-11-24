#	$NetBSD: Makefile,v 1.135.4.1 2012/11/24 19:45:16 riz Exp $

.include <bsd.own.mk>

SUBDIR+=	bc
SUBDIR+=	c89 c99
SUBDIR+=	dc diffutils gkermit
SUBDIR+=	rcs send-pr texinfo

.if ${MKBSDGREP} == "no"
SUBDIR+=	grep
.endif

.if ${MKGROFF} != "no"
SUBDIR+=	groff
.endif

SUBDIR+=	gettext

.if ${HAVE_GCC} == 4
.if ${MKGCCCMDS} != "no"
SUBDIR+=	gcc4
.endif
.endif

.include <bsd.subdir.mk>
