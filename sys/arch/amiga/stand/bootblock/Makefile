#	$NetBSD: Makefile,v 1.1.40.1 2002/01/08 00:22:59 nathanw Exp $

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
