#	$NetBSD: Makefile,v 1.4 2010/07/06 05:59:57 mrg Exp $

.include <bsd.own.mk>

# Don't install these, but make them first:
.ifnmake install
SUBDIR=txlt
SUBDIR+=elf2bb
.endif

# but these:
SUBDIR+=boot bootxx_ffs


.include <bsd.subdir.mk>
