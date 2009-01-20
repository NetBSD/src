#	$NetBSD: Makefile,v 1.5 2009/01/20 22:49:29 tron Exp $

.include <bsd.own.mk>
CPPFLAGS+=	-DELFSIZE=32
LIB=	ldd_elf32

# XXX Force one member
SRCS=	dummy.c

LIBISPRIVATE=	yes
.PATH: ${.CURDIR}/..

# XXX
.include <bsd.own.mk>
.if ${MACHINE} == "sparc64"
.include "${NETBSDSRCDIR}/libexec/ld.elf_so/arch/sparc/Makefile.ld32"
.elif ${MACHINE} == "amd64"
.include "${NETBSDSRCDIR}/libexec/ld.elf_so/arch/i386/Makefile.ld32"
.endif

.if ${MACHINE} != "alpha"
.include "../Makefile.elf"
.endif

.include <bsd.lib.mk>
