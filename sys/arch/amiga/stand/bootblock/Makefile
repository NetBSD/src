#	$NetBSD: Makefile,v 1.3.130.1 2011/03/05 20:49:22 rmind Exp $

.include <bsd.own.mk>

# Don't install these, but make them first:
.ifnmake install
SUBDIR=txlt
SUBDIR+=elf2bb
.endif

# but these:
SUBDIR+=boot bootxx_ffs


.include <bsd.subdir.mk>
