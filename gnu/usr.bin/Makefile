#	$NetBSD: Makefile,v 1.117 2003/10/18 23:44:46 matt Exp $

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
.if ${HAVE_GDB53} == "no"
SUBDIR+=	sim .WAIT gdb
.else
SUBDIR+=	gdb53
.endif
.endif

.if ${MKGCCCMDS} != "no"
.if ${HAVE_GCC3} == "no"
SUBDIR+=	gcc
.else
SUBDIR+=	gcc3
.endif
.endif

.include <bsd.subdir.mk>
