#	$NetBSD: Makefile,v 1.3 2002/03/16 20:37:49 reinoud Exp $

.include <bsd.own.mk>

SUBDIR+= abort-fixup

.if ${OBJECT_FMT} == "ELF"
SUBDIR+= sigstackalign
.endif

.include <bsd.subdir.mk>
