#	$NetBSD: Makefile,v 1.123 2006/06/02 22:16:20 mrg Exp $

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
.if ${HAVE_GCC} == "3"
SUBDIR+=	gcc3
.else
SUBDIR+=	gcc4
.endif
.endif

.include <bsd.subdir.mk>
