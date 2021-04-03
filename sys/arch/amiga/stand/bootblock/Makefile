#	$NetBSD: Makefile,v 1.4.72.1 2021/04/03 22:28:14 thorpej Exp $

.include <bsd.own.mk>

# Don't install these, but make them first:
.ifnmake install
SUBDIR=txlt
SUBDIR+=elf2bb
.endif

# but these:
SUBDIR+=boot bootxx_ffs bootxx_ffsv2


.include <bsd.subdir.mk>
