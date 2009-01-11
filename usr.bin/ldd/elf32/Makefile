#	$NetBSD: Makefile,v 1.4 2009/01/11 03:50:50 christos Exp $

.include <bsd.own.mk>
CPPFLAGS+=	-DELFSIZE=32
LIB=	ldd_elf32

# XXX Force one member
SRCS=	dummy.c

LIBISPRIVATE=	yes
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
