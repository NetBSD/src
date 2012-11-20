#	$NetBSD: Makefile,v 1.135.6.1 2012/11/20 18:52:37 matt Exp $

.include <bsd.own.mk>

SUBDIR+=	bc
.if ${MKBINUTILS} != "no" && ${HAVE_BINUTILS} == "219"
SUBDIR+=	binutils
.endif
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

.if ${MKGCC} != "no"
.if ${HAVE_GCC} == 4
.if ${MKGCCCMDS} != "no"
SUBDIR+=	gcc4
.endif
.endif
.endif

.include <bsd.subdir.mk>
