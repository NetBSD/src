#	$NetBSD: Makefile,v 1.5 2021/02/25 03:42:14 rin Exp $

.include <bsd.own.mk>

# Don't install these, but make them first:
.ifnmake install
SUBDIR=txlt
SUBDIR+=elf2bb
.endif

# but these:
SUBDIR+=boot bootxx_ffs bootxx_ffsv2


.include <bsd.subdir.mk>
