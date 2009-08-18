#	$NetBSD: Makefile,v 1.130 2009/08/18 15:11:25 skrll Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc
.if ${MKBINUTILS} != "no" && ${HAVE_BINUTILS} == "216"
SUBDIR+=	binutils
.endif
SUBDIR+=	c89 c99
SUBDIR+=	dc diffutils gkermit grep
SUBDIR+=	groff rcs send-pr texinfo

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
