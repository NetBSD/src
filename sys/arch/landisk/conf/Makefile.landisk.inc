#	$NetBSD: Makefile.landisk.inc,v 1.4 2008/02/02 05:53:40 uwe Exp $

.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
