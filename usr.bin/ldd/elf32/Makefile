#	$NetBSD: Makefile,v 1.4.2.3 2009/01/22 23:17:34 snj Exp $

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
