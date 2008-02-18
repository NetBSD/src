#	$NetBSD: Makefile.mmeye.inc,v 1.6.100.1 2008/02/18 21:04:46 mjf Exp $

.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
