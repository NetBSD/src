#	$NetBSD: Makefile,v 1.1 2009/01/06 03:59:56 mrg Exp $

.if ${MACHINE} != "alpha"

CPPFLAGS+=	-DELFSIZE=32
LIB=	ldd_elf32

# XXX
.if ${MACHINE} == "sparc64"
.include "../../../libexec/ld.elf_so/arch/sparc/Makefile.ld32"
.elif ${MACHINE} == "amd64"
.include "../../../libexec/ld.elf_so/arch/i386/Makefile.ld32"
.endif

.include "../Makefile.elf"

.endif
