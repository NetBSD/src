#	$NetBSD: Makefile,v 1.134 2011/04/21 14:26:34 joerg Exp $

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

.if ${MKGDB} != "no"
.if ${HAVE_GDB} == "6"
SUBDIR+=	gdb6
.endif
.endif

.if ${MKGCCCMDS} != "no"
SUBDIR+=	gcc4
.endif

.include <bsd.subdir.mk>
