#	$NetBSD: Makefile,v 1.121 2006/02/04 16:30:04 christos Exp $

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
.if ${HAVE_GCC3} == "no"
SUBDIR+=	gcc
.else
SUBDIR+=	gcc3
.endif
.endif

.include <bsd.subdir.mk>
