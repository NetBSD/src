#	$NetBSD: Makefile,v 1.1.40.2 2003/01/15 18:17:43 thorpej Exp $

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
