#	$NetBSD: Makefile,v 1.114 2003/07/25 16:57:07 mrg Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils c89 cvs dc diffutils grep
SUBDIR+=	groff gzip rcs send-pr texinfo

SUBDIR+=	gawk

SUBDIR+=	gettext

SUBDIR+=	sim .WAIT gdb

.if ${USE_TOOLS_TOOLCHAIN} != "no"
SUBDIR+=	gcc
.else
SUBDIR+=	gcc3
.endif

.include <bsd.subdir.mk>
