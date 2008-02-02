#	$NetBSD: Makefile.mmeye.inc,v 1.7 2008/02/02 17:54:17 uwe Exp $

.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
