#	$NetBSD: _i386.mk,v 1.2 1999/01/11 09:53:26 christos Exp $

EMULS=		i386nbsd elf_i386
.if ${OBJECT_FMT} == "ELF"
DEFAULT_EMUL=	elf_i386
.else
DEFAULT_EMUL=	i386nbsd
.endif
