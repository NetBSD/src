#	$NetBSD: Makefile,v 1.133 2010/07/06 05:59:51 mrg Exp $

.include <bsd.own.mk>

SUBDIR+=	bc
SUBDIR+=	c89 c99
SUBDIR+=	dc diffutils gkermit grep
SUBDIR+=	rcs send-pr texinfo

.if ${MKGROFF} != "no"
SUBDIR+=	groff
.endif

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
