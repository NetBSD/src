#	$NetBSD: Makefile,v 1.5.2.1 2009/05/13 19:19:54 jym Exp $

.include <bsd.own.mk>
CPPFLAGS+=	-DELFSIZE=32
LIB=	ldd_elf32

# XXX Force one member
SRCS=	dummy.c

LIBISPRIVATE=	yes
.PATH: ${.CURDIR}/..

# XXX
.include <bsd.own.mk>
.if ${MACHINE_ARCH} == "sparc64"
.include "${NETBSDSRCDIR}/libexec/ld.elf_so/arch/sparc/Makefile.ld32"
.elif ${MACHINE_ARCH} == "x86_64"
.include "${NETBSDSRCDIR}/libexec/ld.elf_so/arch/i386/Makefile.ld32"
.endif

.if ${MACHINE_ARCH} != "alpha"
.include "../Makefile.elf"
.endif

.include <bsd.lib.mk>
