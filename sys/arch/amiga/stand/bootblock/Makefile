#	$NetBSD: Makefile,v 1.3 2003/01/15 07:25:51 mhitch Exp $

.include <bsd.own.mk>

# Don't install these, but make them first:
.ifnmake install
SUBDIR=txlt
.if ${OBJECT_FMT} == "ELF"
SUBDIR+=elf2bb
.else
SUBDIR+=aout2bb
.endif
.endif

# but these:
SUBDIR+=boot bootxx_ffs


.include <bsd.subdir.mk>
