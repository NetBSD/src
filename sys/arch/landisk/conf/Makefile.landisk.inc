#	$NetBSD: Makefile.landisk.inc,v 1.2.30.1 2008/02/18 21:04:42 mjf Exp $

.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
