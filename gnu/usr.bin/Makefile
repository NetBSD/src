#	$NetBSD: Makefile,v 1.116 2003/09/18 23:17:55 mrg Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

SUBDIR+=	bc binutils c89 cvs dc diffutils grep
SUBDIR+=	groff gzip rcs send-pr texinfo

SUBDIR+=	gawk

SUBDIR+=	gettext

.if ${HAVE_GDB53} == "no"
SUBDIR+=	sim .WAIT gdb
.else
SUBDIR+=	gdb53
.endif

.if ${HAVE_GCC3} == "no"
SUBDIR+=	gcc
.else
SUBDIR+=	gcc3
.endif

.include <bsd.subdir.mk>
