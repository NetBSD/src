#	$NetBSD: Makefile,v 1.3.108.1 2010/08/11 22:51:36 yamt Exp $

.include <bsd.own.mk>

# Don't install these, but make them first:
.ifnmake install
SUBDIR=txlt
SUBDIR+=elf2bb
.endif

# but these:
SUBDIR+=boot bootxx_ffs


.include <bsd.subdir.mk>
