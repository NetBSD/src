#	$NetBSD: Makefile.inc,v 1.1.54.1 2009/05/13 19:19:54 jym Exp $

WARNS?=	3	# XXX: -Wsign-compare issues ld.elf_so source

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
