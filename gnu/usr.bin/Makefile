#	$NetBSD: Makefile,v 1.122 2006/04/07 19:38:58 mrg Exp $

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
SUBDIR+=	dc diffutils grep
SUBDIR+=	groff rcs send-pr texinfo

SUBDIR+=	gettext

.if ${MKGDB} != "no"
#SUBDIR+=	sim .WAIT
SUBDIR+=	gdb53
.endif

.if ${MKGCCCMDS} != "no"
.if ${HAVE_GCC} == "2"
SUBDIR+=	gcc
.elif ${HAVE_GCC} == "3"
SUBDIR+=	gcc3
.else
SUBDIR+=	gcc4
.endif
.endif

.include <bsd.subdir.mk>
