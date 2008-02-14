#	$NetBSD: Makefile,v 1.126.4.1 2008/02/14 14:38:44 skrll Exp $

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
SUBDIR+=	gdb6
.endif

.if ${MKGCCCMDS} != "no"
SUBDIR+=	gcc4
.endif

.include <bsd.subdir.mk>
