#	$NetBSD: Makefile,v 1.2 2009/01/07 00:39:24 mrg Exp $

CPPFLAGS+=	-DELFSIZE=32
LIB=	ldd_elf32

# XXX Force one member
SRCS=	dummy.c

.PATH: ${.CURDIR}/..

# XXX
.if ${MACHINE} == "sparc64"
.include "${NETBSDSRCDIR}/libexec/ld.elf_so/arch/sparc/Makefile.ld32"
.elif ${MACHINE} == "amd64"
.include "${NETBSDSRCDIR}/libexec/ld.elf_so/arch/i386/Makefile.ld32"
.endif

.if ${MACHINE} != "alpha"
.include "../Makefile.elf"
.endif

.include <bsd.lib.mk>
