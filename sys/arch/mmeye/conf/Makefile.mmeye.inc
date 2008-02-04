#	$NetBSD: Makefile.mmeye.inc,v 1.6.34.1 2008/02/04 09:22:16 yamt Exp $

.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
