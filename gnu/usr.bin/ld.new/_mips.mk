#	$NetBSD: _mips.mk,v 1.1.2.2 1998/11/07 00:56:35 cgd Exp $

EMULS=		elf32lmip elf32bmip

#   Default emul depends on endian-ness
.if	(${MACHINE_GNU_ARCH} == "mipsel")
DEFAULT_EMUL=	elf32lmip
.else
DEFAULT_EMUL=	elf32bmip
.endif
