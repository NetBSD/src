#	$NetBSD: _sparc.mk,v 1.3 1999/01/31 21:16:46 christos Exp $

EMULS=		sparcnbsd elf32_sparc sun4
.if ${OBJECT_FMT} == "ELF"
DEFAULT_EMUL=	elf32_sparc
.else
DEFAULT_EMUL=	sparcnbsd
.endif
