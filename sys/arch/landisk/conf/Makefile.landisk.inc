#	$NetBSD: Makefile.landisk.inc,v 1.3 2008/02/02 03:14:03 uwe Exp $

TEXTADDR?=0x8c001000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
