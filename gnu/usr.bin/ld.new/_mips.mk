#	$NetBSD: _mips.mk,v 1.1 1998/09/30 16:47:48 jonathan Exp $

EMULS=		elf32lmip elf32bmip

#   Default emul depends on endian-ness
.if	(${MACHINE_GNU_ARCH} == "mipsel")
DEFAULT_EMUL=	elf32lmip
.else
DEFAULT_EMUL=	elf32bmip
.endif
