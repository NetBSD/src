#	$NetBSD: Makefile,v 1.3.128.1 2010/08/17 06:43:57 uebayasi Exp $

.include <bsd.own.mk>

# Don't install these, but make them first:
.ifnmake install
SUBDIR=txlt
SUBDIR+=elf2bb
.endif

# but these:
SUBDIR+=boot bootxx_ffs


.include <bsd.subdir.mk>
