#	$NetBSD: Makefile,v 1.115 2003/08/27 16:03:17 mrg Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils c89 cvs dc diffutils grep
SUBDIR+=	groff gzip rcs send-pr texinfo

SUBDIR+=	gawk

SUBDIR+=	gettext

SUBDIR+=	sim .WAIT gdb

.if ${HAVE_GCC3} == "no"
SUBDIR+=	gcc
.else
SUBDIR+=	gcc3
.endif

.include <bsd.subdir.mk>
