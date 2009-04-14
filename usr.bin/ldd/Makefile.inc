#	$NetBSD: Makefile.inc,v 1.2 2009/04/14 22:15:22 lukem Exp $

WARNS?=	3	# XXX: -Wsign-compare issues ld.elf_so source

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
