#	$NetBSD: Makefile,v 1.118 2003/11/27 10:52:02 mrg Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc
.if ${MKBINUTILS} != "no"
SUBDIR+=	binutils
.endif
SUBDIR+=	c89
.if ${MKCVS} != "no"
SUBDIR+=	cvs
.endif
SUBDIR+=	dc diffutils grep
SUBDIR+=	groff gzip rcs send-pr texinfo

SUBDIR+=	gawk

SUBDIR+=	gettext

.if ${MKGDB} != "no"
#SUBDIR+=	sim .WAIT
SUBDIR+=	gdb53
.endif

.if ${MKGCCCMDS} != "no"
.if ${HAVE_GCC3} == "no"
SUBDIR+=	gcc
.else
SUBDIR+=	gcc3
.endif
.endif

.include <bsd.subdir.mk>
