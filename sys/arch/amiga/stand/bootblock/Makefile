#	$NetBSD: Makefile,v 1.2 2001/12/17 05:48:09 mhitch Exp $

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
SUBDIR+=boot bootxx_ffs installboot


.include <bsd.subdir.mk>
