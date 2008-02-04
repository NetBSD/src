#	$NetBSD: Makefile.landisk.inc,v 1.1.12.4 2008/02/04 09:22:10 yamt Exp $

.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
