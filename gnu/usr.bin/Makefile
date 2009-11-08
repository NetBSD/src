#	$NetBSD: Makefile,v 1.131 2009/11/08 15:24:49 skrll Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc
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
