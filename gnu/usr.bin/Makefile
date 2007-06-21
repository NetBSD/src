#	$NetBSD: Makefile,v 1.127 2007/06/21 12:51:06 skrll Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc
.if ${MKBINUTILS} != "no"
SUBDIR+=	binutils
.endif
SUBDIR+=	c89
.if ${MKCVS} != "no"
SUBDIR+=	xcvs
.endif
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
