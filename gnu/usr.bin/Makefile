#	$NetBSD: Makefile,v 1.125 2006/11/01 13:18:03 apb Exp $

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
.if ${HAVE_GDB} == "5"
SUBDIR+=	gdb53
.else
SUBDIR+=	gdb6
.endif
.endif

.if ${MKGCCCMDS} != "no"
.if ${HAVE_GCC} == "3"
SUBDIR+=	gcc3
.else
SUBDIR+=	gcc4
.endif
.endif

.include <bsd.subdir.mk>
